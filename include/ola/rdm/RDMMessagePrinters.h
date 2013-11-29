/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  A command line based RDM controller
 *  Copyright (C) 2010 Simon Newton
 */

/**
 * @addtogroup rdm_helpers
 * @{
 * @file RDMMessagePrinters.h
 * @brief Used for displaying the RDM data to the command line
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RDMMESSAGEPRINTERS_H_
#define INCLUDE_OLA_RDM_RDMMESSAGEPRINTERS_H_

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/messaging/MessagePrinter.h>
#include <ola/rdm/PidStore.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/UID.h>
#include <iomanip>
#include <set>
#include <string>
#include <vector>


namespace ola {
namespace rdm {

using ola::messaging::BoolMessageField;
using ola::messaging::GenericMessagePrinter;
using ola::messaging::GroupMessageField;
using ola::messaging::Int16MessageField;
using ola::messaging::Int32MessageField;
using ola::messaging::Int8MessageField;
using ola::messaging::MessagePrinter;
using ola::messaging::StringMessageField;
using ola::messaging::UInt16MessageField;
using ola::messaging::UInt32MessageField;
using ola::messaging::UInt8MessageField;
using ola::messaging::UIDMessageField;
using std::endl;
using std::set;
using std::string;


/**
 * A RDM specific printer that transforms field names
 */
class RDMMessagePrinter: public GenericMessagePrinter {
 public:
    explicit RDMMessagePrinter(unsigned int initial_ident = 0)
      : GenericMessagePrinter(GenericMessagePrinter::DEFAULT_INDENT,
                              initial_ident) {
    }
 protected:
    string TransformLabel(const string &label) {
      string new_label = label;
      ola::CustomCapitalizeLabel(&new_label);
      return new_label;
    }
};


/**
 * Print a list of proxied UIDs
 */
class ProxiedDevicesPrinter: public MessagePrinter {
 public:
    void Visit(const UIDMessageField *field) {
      Stream() << field->Value() << endl;
    }
};


/**
 * Print a status message.
 */
class StatusMessagePrinter: public MessagePrinter {
 public:
    void Visit(const UInt8MessageField *field) {
      if (m_messages.empty())
        return;
      m_messages.back().status_type = field->Value();
      m_messages.back().status_type_defined = true;
    }

    void Visit(const Int16MessageField *field) {
      if (m_messages.empty())
        return;
      status_message &message = m_messages.back();
      if (message.int_offset < MAX_INT_FIELDS)
        message.int16_fields[message.int_offset++] = field->Value();
    }

    void Visit(const UInt16MessageField *field) {
      if (m_messages.empty())
        return;
      status_message &message = m_messages.back();
      if (message.uint_offset < MAX_UINT_FIELDS)
        message.uint16_fields[message.uint_offset++] = field->Value();
    }

    void Visit(const GroupMessageField*) {
      status_message message;
      m_messages.push_back(message);
    }

 protected:
    void PostStringHook() {
      vector<status_message>::const_iterator iter = m_messages.begin();
      for (; iter != m_messages.end(); ++iter) {
        if (!iter->status_type_defined ||
            iter->uint_offset != MAX_UINT_FIELDS ||
            iter->int_offset != MAX_INT_FIELDS) {
          OLA_WARN << "Invalid status message";
          continue;
        }

        const string message = StatusMessageIdToString(
            iter->uint16_fields[1],
            iter->int16_fields[0],
            iter->int16_fields[1]);

        Stream() << StatusTypeToString(iter->status_type) << ": ";
        if (iter->uint16_fields[0])
          Stream() << "Sub-device " << iter->uint16_fields[0] << ": ";

        if (message.empty()) {
          Stream() << " message-id: " <<
            iter->uint16_fields[1] << ", data1: " << iter->int16_fields[0] <<
            ", data2: " << iter->int16_fields[1] << endl;
        } else {
          Stream() << message << endl;
        }
      }
    }

 private:
    enum { MAX_INT_FIELDS = 2 };
    enum { MAX_UINT_FIELDS = 2 };
    struct status_message {
      public:
        uint16_t uint16_fields[MAX_UINT_FIELDS];
        int16_t int16_fields[MAX_INT_FIELDS];
        uint8_t uint_offset;
        uint8_t int_offset;
        uint8_t status_type;
        bool status_type_defined;

        status_message() : uint_offset(0), int_offset(0), status_type(0),
            status_type_defined(false) {}
    };
    vector<status_message> m_messages;
};


/**
 * Print a list of supported params with their canonical names
 */
class SupportedParamsPrinter: public MessagePrinter {
 public:
    SupportedParamsPrinter(uint16_t manufacturer_id,
                           const RootPidStore *root_store)
        : m_manufacturer_id(manufacturer_id),
          m_root_store(root_store) {}

    void Visit(const UInt16MessageField *message) {
      m_pids.insert(message->Value());
    }

 protected:
    void PostStringHook() {
      set<uint16_t>::const_iterator iter = m_pids.begin();
      for (; iter != m_pids.end(); ++iter) {
        Stream() << "  0x" << std::hex << *iter;
        const PidDescriptor *descriptor = m_root_store->GetDescriptor(
            *iter, m_manufacturer_id);
        if (descriptor) {
          string name = descriptor->Name();
          ola::ToLower(&name);
          Stream() << " (" << name << ")";
        }
        Stream() << endl;
      }
    }

 private:
    set<uint16_t> m_pids;
    uint16_t m_manufacturer_id;
    const RootPidStore *m_root_store;
};


/**
 * Print the device info message.
 */
class DeviceInfoPrinter: public GenericMessagePrinter {
 public:
    void Visit(const UInt16MessageField *message) {
      const string name = message->GetDescriptor()->Name();
      if (name == "product_category")
        Stream() << TransformLabel(name) << ": " <<
          ProductCategoryToString(message->Value()) << endl;
      else
        GenericMessagePrinter::Visit(message);
    }

 protected:
    string TransformLabel(const string &label) {
      string new_label = label;
      ola::CustomCapitalizeLabel(&new_label);
      return new_label;
    }
};


/**
 * Print the string fields of a message
 */
class LabelPrinter: public MessagePrinter {
 public:
    void Visit(const StringMessageField *message) {
      Stream() << message->Value() << endl;
    }
};


/**
 * Print the list of product detail ids
 */
class ProductIdPrinter: public MessagePrinter {
 public:
    void Visit(const UInt16MessageField *message) {
      m_product_ids.insert(message->Value());
    }

    void PostStringHook() {
      set<uint16_t>::const_iterator iter = m_product_ids.begin();
      for (; iter != m_product_ids.end(); ++iter) {
        Stream() << ProductDetailToString(*iter) << endl;
      }
    }
 private:
    set<uint16_t> m_product_ids;
};


/**
 * Print the list of supported languages.
 */
class LanguageCapabilityPrinter: public MessagePrinter {
 public:
    void Visit(const StringMessageField *message) {
      m_languages.insert(message->Value());
    }

    void PostStringHook() {
      set<string>::const_iterator iter = m_languages.begin();
      for (; iter != m_languages.end(); ++iter) {
        Stream() << *iter << endl;
      }
    }
 private:
    set<string> m_languages;
};


/**
 * Print the real time clock info
 */
class ClockPrinter: public MessagePrinter {
 public:
    ClockPrinter() : MessagePrinter(), m_offset(0) {}
    void Visit(const UInt16MessageField *message) {
      m_year = message->Value();
    }

    void Visit(const UInt8MessageField *message) {
      if (m_offset < CLOCK_FIELDS)
        m_fields[m_offset] = message->Value();
      m_offset++;
    }

    void PostStringHook() {
      if (m_offset != CLOCK_FIELDS) {
        Stream() << "Malformed packet";
      }
      Stream() << std::setfill('0') << std::setw(2) <<
        static_cast<int>(m_fields[1]) << "/" <<
        static_cast<int>(m_fields[0]) << "/" <<
        m_year << " " <<
        static_cast<int>(m_fields[2]) << ":" <<
        static_cast<int>(m_fields[3]) << ":" <<
        static_cast<int>(m_fields[4]) << endl;
    }

 private:
    enum { CLOCK_FIELDS = 5};
    uint16_t m_year;
    uint8_t m_fields[CLOCK_FIELDS];
    unsigned int m_offset;
};

/**
 * Print slot info.
 */
class SlotInfoPrinter: public MessagePrinter {
 public:
    void Visit(const UInt8MessageField *field) {
      if (m_slot_info.empty())
        return;
      m_slot_info.back().type = field->Value();
      m_slot_info.back().type_defined = true;
    }

    void Visit(const UInt16MessageField *field) {
      if (m_slot_info.empty())
        return;
      if (!m_slot_info.back().offset_defined) {
        m_slot_info.back().offset = field->Value();
        m_slot_info.back().offset_defined = true;
      } else {
        m_slot_info.back().label = field->Value();
        m_slot_info.back().label_defined = true;
      }
    }

    void Visit(const GroupMessageField*) {
      slot_info slot;
      m_slot_info.push_back(slot);
    }

 protected:
    void PostStringHook() {
      vector<slot_info>::const_iterator iter = m_slot_info.begin();
      for (; iter != m_slot_info.end(); ++iter) {
        if (!iter->offset_defined ||
            !iter->type_defined ||
            !iter->label_defined) {
          OLA_WARN << "Invalid slot info";
          continue;
        }

        const string slot = SlotInfoToString(iter->type, iter->label);

        if (slot.empty()) {
          Stream() << " offset: " <<
            iter->offset << ", type: " << iter->type <<
            ", label: " << iter->label << endl;
        } else {
          Stream() << "Slot offset " << iter->offset << ": " << slot << endl;
        }
      }
    }

 private:
    struct slot_info {
      public:
        uint16_t offset;
        bool offset_defined;
        uint8_t type;
        bool type_defined;
        uint16_t label;
        bool label_defined;

        slot_info() : offset(0), offset_defined(false), type(0),
            type_defined(false), label(0), label_defined(false) {}
    };
    vector<slot_info> m_slot_info;
};


/**
 * Print sensor definition.
 */
class SensorDefinitionPrinter: public GenericMessagePrinter {
 public:
    void Visit(const UInt8MessageField *message) {
      const string name = message->GetDescriptor()->Name();
      if (name == "type") {
        Stream() << TransformLabel(name) << ": " <<
          SensorTypeToString(message->Value()) << endl;
      } else if (name == "unit") {
        Stream() << TransformLabel(name) << ": ";
        if (message->Value() == UNITS_NONE) {
          Stream() << "None";
        } else {
          Stream() << UnitToString(message->Value());
        }
        Stream() << endl;
      } else if (name == "prefix") {
        Stream() << TransformLabel(name) << ": ";
        if (message->Value() == PREFIX_NONE) {
          Stream() << "None";
        } else {
          Stream() << PrefixToString(message->Value());
        }
        Stream() << endl;
      } else if (name == "supports_recording") {
        Stream() << TransformLabel(name) << ": ";
        string supports_recording =
            SensorSupportsRecordingToString(message->Value());
        if (supports_recording.empty()) {
          Stream() << "None";
        } else {
          Stream() << supports_recording;
        }
        Stream() << endl;
      } else {
        GenericMessagePrinter::Visit(message);
      }
    }

 protected:
    string TransformLabel(const string &label) {
      string new_label = label;
      ola::CustomCapitalizeLabel(&new_label);
      return new_label;
    }
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMMESSAGEPRINTERS_H_
