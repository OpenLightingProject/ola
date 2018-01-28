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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * SPIOutput.h
 * An RDM-controllable SPI device. Takes up to one universe of DMX.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_SPI_SPIOUTPUT_H_
#define PLUGINS_SPI_SPIOUTPUT_H_

#include <memory>
#include <string>
#include "common/rdm/NetworkManager.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/stl/STLUtils.h"
#include "ola/rdm/ResponderOps.h"
#include "ola/rdm/ResponderPersonality.h"
#include "ola/rdm/ResponderSensor.h"

namespace ola {
namespace plugin {
namespace spi {

class SPIOutput: public ola::rdm::DiscoverableRDMControllerInterface {
 public:
  // definitions for all SPI Personalities
  // keep personality order
  // - it is important for backwards compatibility
  // the integer representation is used to store the configuration in files
  // and RDM Personality IDs should also be stable.
  // new ones can be added at end.
  // remember to increment RDM-Version in
  // SPIOutput.cpp SPIOutput::GetDeviceInfo()
  enum SPI_PERSONALITY {
    PERS_WS2801_INDIVIDUAL = 1,
    PERS_WS2801_COMBINED = 2,
    PERS_LDP8806_INDIVIDUAL = 3,
    PERS_LDP8806_COMBINED = 4,
    PERS_P9813_INDIVIDUAL = 5,
    PERS_P9813_COMBINED = 6,
    PERS_APA102_INDIVIDUAL = 7,
    PERS_APA102_COMBINED = 8,
    PERS_APA102_PB_INDIVIDUAL,
    PERS_APA102_PB_COMBINED,
    PERS_TLC5971_INDIVIDUAL,
  };

  struct Options {
    std::string device_label;
    uint8_t pixel_count;
    uint8_t output_number;

    explicit Options(uint8_t output_number, const std::string &spi_device_name)
        : device_label("SPI Device - " + spi_device_name),
          pixel_count(25),  // For the https://www.adafruit.com/products/738
          output_number(output_number) {
    }
  };

  SPIOutput(const ola::rdm::UID &uid,
            class SPIBackendInterface *backend,
            const Options &options);
  ~SPIOutput();

  std::string GetDeviceLabel() const;
  bool SetDeviceLabel(const std::string &device_label);
  uint8_t GetPersonality() const;
  bool SetPersonality(uint16_t personality);
  uint16_t GetStartAddress() const;
  bool SetStartAddress(uint16_t start_address);
  unsigned int PixelCount() const { return m_pixel_count; }

  std::string Description() const;
  bool WriteDMX(const DmxBuffer &buffer);

  void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback);
  void SendRDMRequest(ola::rdm::RDMRequest *request,
                      ola::rdm::RDMCallback *callback);

 private:
  /**
   * The RDM Operations for the MovingLightResponder.
   */
  class RDMOps : public ola::rdm::ResponderOps<SPIOutput> {
   public:
    static RDMOps *Instance() {
      if (!instance)
        instance = new RDMOps();
      return instance;
    }

   private:
    RDMOps() : ola::rdm::ResponderOps<SPIOutput>(PARAM_HANDLERS) {}

    static RDMOps *instance;
  };

  class SPIBackendInterface *m_backend;
  const uint8_t m_output_number;
  std::string m_spi_device_name;
  const ola::rdm::UID m_uid;
  const unsigned int m_pixel_count;
  std::string m_device_label;
  uint16_t m_start_address;  // starts from 1
  bool m_identify_mode;
  std::auto_ptr<ola::rdm::PersonalityCollection> m_personality_collection;
  std::auto_ptr<ola::rdm::PersonalityManager> m_personality_manager;
  ola::rdm::Sensors m_sensors;
  std::auto_ptr<ola::rdm::NetworkManagerInterface> m_network_manager;

  // DMX methods
  bool InternalWriteDMX(const DmxBuffer &buffer);

  void IndividualWS2801Control(const DmxBuffer &buffer);
  void CombinedWS2801Control(const DmxBuffer &buffer);
  void IndividualLPD8806Control(const DmxBuffer &buffer);
  void CombinedLPD8806Control(const DmxBuffer &buffer);
  void IndividualP9813Control(const DmxBuffer &buffer);
  void CombinedP9813Control(const DmxBuffer &buffer);
  void IndividualAPA102Control(const DmxBuffer &buffer);
  void CombinedAPA102Control(const DmxBuffer &buffer);
  void IndividualAPA102ControlPixelBrightness(const DmxBuffer &buffer);
  void CombinedAPA102ControlPixelBrightness(const DmxBuffer &buffer);
  void IndividualTLC5971Control(const DmxBuffer &buffer);


  unsigned int LPD8806BufferSize() const;
  void WriteSPIData(const uint8_t *data, unsigned int length);

  // RDM methods
  ola::rdm::RDMResponse *GetDeviceInfo(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetProductDetailList(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDeviceModelDescription(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetManufacturerLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDeviceLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetDeviceLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetSoftwareVersionLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDmxPersonality(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetDmxPersonality(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetPersonalityDescription(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetSlotInfo(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDmxStartAddress(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetDmxStartAddress(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetIdentify(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetIdentify(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetSensorDefinition(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetSensorValue(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *SetSensorValue(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *RecordSensor(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetListInterfaces(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetInterfaceLabel(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetInterfaceHardwareAddressType1(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetIPV4CurrentAddress(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetIPV4DefaultRoute(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDNSHostname(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDNSDomainName(
      const ola::rdm::RDMRequest *request);
  ola::rdm::RDMResponse *GetDNSNameServer(
      const ola::rdm::RDMRequest *request);

  // Helpers
  uint8_t P9813CreateFlag(uint8_t red, uint8_t green, uint8_t blue);
  static uint8_t CalculateAPA102LatchBytes(uint16_t pixel_count);
  static uint8_t CalculateAPA102PixelBrightness(uint8_t brightness);

  static const uint8_t SPI_MODE;
  static const uint8_t SPI_BITS_PER_WORD;
  static const uint16_t SPI_DELAY;
  static const uint32_t SPI_SPEED;
  static const uint16_t WS2801_SLOTS_PER_PIXEL;
  static const uint16_t LPD8806_SLOTS_PER_PIXEL;
  static const uint16_t P9813_SLOTS_PER_PIXEL;
  static const uint16_t P9813_SPI_BYTES_PER_PIXEL;
  static const uint16_t APA102_SLOTS_PER_PIXEL;
  static const uint16_t APA102_PB_SLOTS_PER_PIXEL;
  static const uint16_t APA102_SPI_BYTES_PER_PIXEL;
  static const uint16_t APA102_START_FRAME_BYTES;
  static const uint8_t  APA102_LEDFRAME_START_MARK;
  static const uint16_t TLC5971_SLOTS_PER_DEVICE;
  static const uint16_t TLC5971_SPI_BYTES_PER_DEVICE;

  // TLC5971 data structure
  PACK(
  struct TLC5971_packet_config_fields_t{
    //  Write Command (6Bit)
    uint8_t WRCMD : 6;
    //  Function Control Data (5 x 1Bit = 5Bit)
    uint8_t OUTTMG : 1;
    uint8_t EXTGCK : 1;
    uint8_t TMGRST : 1;
    uint8_t DSPRPT : 1;
    uint8_t BLANK : 1;
    //  BC-Data (3 x 7Bits = 21Bit)
    uint8_t BCB : 7;
    uint8_t BCG : 7;
    uint8_t BCR : 7;
  });

  union TLC5971_packet_config_t {
    uint8_t bytes[4];
    // 6 + 5 + 21 = 4byte
     TLC5971_packet_config_fields_t fields;
  };

  // //  Write Command (6Bit)
  // static const uint8_t TLC5971_PACKET_CONFIG_MASKS_WRCMD = 0b00111111;
  // //  Function Control Data (5 x 1Bit = 5Bit)
  // static const uint8_t TLC5971_PACKET_CONFIG_MASKS_OUTTMG = 0b00000001;
  // static const uint8_t TLC5971_PACKET_CONFIG_MASKS_EXTGCK = 0b00000001;
  // static const uint8_t TLC5971_PACKET_CONFIG_MASKS_TMGRST = 0b00000001;
  // static const uint8_t TLC5971_PACKET_CONFIG_MASKS_DSPRPT = 0b00000001;
  // static const uint8_t TLC5971_PACKET_CONFIG_MASKS_BLANK  = 0b00000001;
  // //  BC-Data (3 x 7Bits = 21Bit)
  // static const uint8_t TLC5971_PACKET_CONFIG_MASKS_BCB = 0b01111111;
  // static const uint8_t TLC5971_PACKET_CONFIG_MASKS_BCG = 0b01111111;
  // static const uint8_t TLC5971_PACKET_CONFIG_MASKS_BCR = 0b01111111;
  //
  // //  Write Command (6Bit)
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_WRCMD  = 2;
  // //  Function Control Data (5 x 1Bit = 5Bit)
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_OUTTMG = 1;
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_EXTGCK = 0;
  // // byte border ------------------------------------------
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_TMGRST = 7;
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_DSPRPT = 6;
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_BLANK  = 5;
  // //  BC-Data (3 x 7Bits = 21Bit)
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_BCB_RS = 2;
  // // byte border ------------------------------------------
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_BCB_LS = 6;
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_BCG_RS = 1;
  // // byte border ------------------------------------------
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_BCG_LS = 7;
  // static const uint8_t TLC5971_PACKET_CONFIG_LSHIFT_BCR = 0;

  union TLC5971_packet_gsdata_t {
    uint8_t bytes[24];
    // the uint16_t will not work everywhere because of endianess problems..
    // 12ch @16bit = 24byte
    struct {
      uint16_t GSB3;
      uint16_t GSG3;
      uint16_t GSR3;
      uint16_t GSB2;
      uint16_t GSG2;
      uint16_t GSR2;
      uint16_t GSB1;
      uint16_t GSG1;
      uint16_t GSR1;
      uint16_t GSB0;
      uint16_t GSG0;
      uint16_t GSR0;
    } fields;
  };

  union TLC5971_packet_t {
    uint8_t bytes[28];
    struct {
      TLC5971_packet_config_t config;
      // uint8_t config[4];
      TLC5971_packet_gsdata_t gsdata;
    } fields;
  };

  static const ola::rdm::ResponderOps<SPIOutput>::ParamHandler
      PARAM_HANDLERS[];
};
}  // namespace spi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPI_SPIOUTPUT_H_
