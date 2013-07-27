/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ResponderSettings.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERSETTINGS_H_
#define INCLUDE_OLA_RDM_RESPONDERSETTINGS_H_

#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/ResponderHelper.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

using std::string;

/**
 * @brief The base class all Settings inherit from.
 */
class SettingInterface {
  public:
    virtual ~SettingInterface() {}

    /**
     * @brief The text description of this setting
     * @returns the string description of the setting.
     */
    virtual string Description() const = 0;

    /**
     * @brief Return the size of the _DESCRIPTION parameter data.
     */
    virtual unsigned int DescriptionResponseSize() const = 0;

    /**
     * @brief Populate the _DESCRIPTION parameter data.
     * @param index the index for this setting
     * @param data the RDM parameter data to write to.
     */
    virtual unsigned int GenerateDescriptionResponse(uint8_t index,
                                                     uint8_t *data) const = 0;
};

/**
 * @brief A Setting which has a description and no other properties.
 */
class BasicSetting : SettingInterface {
  public:
    typedef const char* ArgType;

    /**
     * @brief Construct a new BasicSetting
     * @param description the description for this setting.
     */
    explicit BasicSetting(const ArgType description);

    /**
     * @brief The text description of this setting
     * @returns the string description of the setting.
     */
    string Description() const { return m_description; }

    unsigned int DescriptionResponseSize() const {
      return sizeof(description_s);
    }

    unsigned int GenerateDescriptionResponse(uint8_t index,
                                             uint8_t *data) const;

  private:
    struct description_s {
      uint8_t setting;
      char description[MAX_RDM_STRING_LENGTH];
    } __attribute__((packed));

    string m_description;
};


/**
 * @brief A PWM Frequency Setting.
 * See Section 4.10 of E1.37-1.
 */
class FrequencyModulationSetting : SettingInterface {
  public:
    /**
     * @brief The constructor argument for the FrequencyModulationSetting
     */
    struct FrequencyModulationArg {
      uint32_t frequency;  /**< The frequency */
      const char *description;  /**< The description */
    };

    typedef FrequencyModulationArg ArgType;

    /**
     * @brief Construct a new FrequencyModulationSetting.
     * @param arg the FrequencyModulationArg for this setting.
     */
    explicit FrequencyModulationSetting(const ArgType &arg);

    /**
     * @brief The text description of this setting
     * @returns the string description of the setting.
     */
    string Description() const { return m_description; }

    /**
     * @brief returns the frequency for this setting.
     */
    uint32_t Frequency() const { return m_frequency; }

    unsigned int DescriptionResponseSize() const {
      return sizeof(description_s);
    }

    unsigned int GenerateDescriptionResponse(uint8_t index,
                                             uint8_t *data) const;

  private:
    struct description_s {
      uint8_t setting;
      uint32_t frequency;
      char description[MAX_RDM_STRING_LENGTH];
    } __attribute__((packed));

    uint32_t m_frequency;
    string m_description;
};


/**
 * @brief Holds the list of settings for a class of responder. A single instance
 * is shared between all responders of the same type. Subclass this and use a
 * singleton.
 *
 * @note Settings are indexed from zero. SettingManager responsible for
 * reporting correct indicies with correct offset.
 */
template <class SettingType>
class SettingCollection {
  public:
    SettingCollection(const typename SettingType::ArgType args[],
                      unsigned int arg_count,
                      bool zero_offset = false)
        : m_zero_offset(zero_offset) {
      for (unsigned int i = 0; i < arg_count; i++) {
        m_settings.push_back(SettingType(args[i]));
      }
    }

    uint8_t Count() const { return m_settings.size(); }

    const SettingType *Lookup(uint8_t index) const {
      if (index > m_settings.size()) {
        return NULL;
      }
      return &m_settings[index];
    }

    unsigned int Offset() const {
      return m_zero_offset ? 0 : 1;
    }

  protected:
    SettingCollection() {}

  private:
    std::vector<SettingType> m_settings;
    const bool m_zero_offset;
};


/**
 * Manages the settings for a single responder.
 */
template <class SettingType>
class SettingManager {
  public:
    explicit SettingManager(const SettingCollection<SettingType> *settings)
        : m_settings(settings),
          m_current_setting(settings->Offset()) {
    }

    virtual ~SettingManager() {}

    const RDMResponse *Get(const RDMRequest *request) const;
    virtual const RDMResponse *Set(const RDMRequest *request);
    const RDMResponse *GetDescription(const RDMRequest *request) const;

    uint8_t Count() const {
      return m_settings->Count();
    }

    uint8_t CurrentSetting() const {
      return m_current_setting + m_settings->Offset();
    }

    bool ChangeSetting(uint8_t state);

  private:
    const SettingCollection<SettingType> *m_settings;
    uint8_t m_current_setting; /**< Index to m_settings, including zero*/
};

typedef SettingCollection<BasicSetting> BasicSettingCollection;
typedef SettingManager<BasicSetting> BasicSettingManager;

template <class SettingType>
const RDMResponse *SettingManager<SettingType>::Get(
    const RDMRequest *request) const {
  uint16_t data = ((m_current_setting + m_settings->Offset()) << 8 |
      m_settings->Count());
  return ResponderHelper::GetUInt16Value(request, data);
}

template <class SettingType>
const RDMResponse *SettingManager<SettingType>::Set(
    const RDMRequest *request) {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  unsigned int offset = m_settings->Offset();
  if (arg < offset || arg >= m_settings->Count() + offset) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    m_current_setting = arg - offset;
    return ResponderHelper::EmptySetResponse(request);
  }
}

template <class SettingType>
const RDMResponse *SettingManager<SettingType>::GetDescription(
    const RDMRequest *request) const {
  uint8_t arg;
  if (!ResponderHelper::ExtractUInt8(request, &arg)) {
    return NackWithReason(request, NR_FORMAT_ERROR);
  }

  unsigned int offset = m_settings->Offset();
  if (arg < offset || arg >= m_settings->Count() + offset) {
    return NackWithReason(request, NR_DATA_OUT_OF_RANGE);
  } else {
    const SettingType *setting = m_settings->Lookup(arg - offset);
    uint8_t output[setting->DescriptionResponseSize()]; // NOLINT
    unsigned int size = setting->GenerateDescriptionResponse(arg, output);
    return GetResponseFromData(request, output, size, RDM_ACK);
  }
}

template <class SettingType>
bool SettingManager<SettingType>::ChangeSetting(uint8_t new_setting) {
  uint8_t offset = m_settings->Offset();

  if (new_setting < offset || new_setting  >= m_settings->Count() + offset)
    return false;

  m_current_setting = new_setting - offset;
  return true;
}
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERSETTINGS_H_
