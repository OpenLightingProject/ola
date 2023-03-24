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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * UniverseChannelAddress.h
 * Represents a universe-channel address pair.
 * Copyright (C) 2023 Peter Newman
 */

/**
 * @addtogroup dmx
 * @{
 * @file UniverseChannelAddress.h
 * @brief Represents a universe-channel address pair
 * @}
 */

#ifndef INCLUDE_OLA_DMX_UNIVERSECHANNELADDRESS_H_
#define INCLUDE_OLA_DMX_UNIVERSECHANNELADDRESS_H_

#include <ola/base/Macro.h>
#include <stdint.h>
#include <sstream>
#include <string>

namespace ola {
namespace dmx {

/**
 * @addtogroup dmx
 * @{
 */

/**
 * @brief The UniverseChannelAddress class.
 **/
class UniverseChannelAddress {
 public:
  UniverseChannelAddress()
      : m_universe(0),
        m_channel(0) {
  }

  UniverseChannelAddress(unsigned int universe, uint16_t channel)
      : m_universe(universe),
        m_channel(channel) {
  }
  UniverseChannelAddress(const UniverseChannelAddress &other)
      : m_universe(other.m_universe),
        m_channel(other.m_channel) {
  }

  ~UniverseChannelAddress() {}

  UniverseChannelAddress& operator=(const UniverseChannelAddress &other) {
    if (this != &other) {
      m_universe = other.m_universe;
      m_channel = other.m_channel;
   }
    return *this;
  }

  bool operator==(const UniverseChannelAddress &other) const {
    return m_universe == other.m_universe && m_channel == other.m_channel;
  }

  bool operator!=(const UniverseChannelAddress &other) const {
    return !(*this == other);
  }

  /**
   * @brief Less than operator for partial ordering.
   *
   * Sorts by universe, then channel.
   */
  bool operator<(const UniverseChannelAddress &other) const {
    if (m_universe == other.m_universe) {
      return m_channel < other.m_channel;
    } else {
      return m_universe < other.m_universe;
    }
  }

  /**
   * @brief Greater than operator.
   *
   * Sorts by universe, then channel.
   */
  bool operator>(const UniverseChannelAddress &other) const {
    if (m_universe == other.m_universe) {
      return m_channel > other.m_channel;
    } else {
      return m_universe > other.m_universe;
    }
  }

  unsigned int Universe() const { return m_universe; }
  void Universe(const unsigned int universe) { m_universe = universe; }
  uint16_t Channel() const { return m_channel; }
  void Channel(uint16_t channel) { m_channel = channel; }

  std::string ToString() const;

  static bool FromString(const std::string &str,
                         UniverseChannelAddress *universe_channel_address);

  // useful for testing
  static UniverseChannelAddress FromStringOrDie(
      const std::string &universe_channel_address);

  friend std::ostream& operator<<(std::ostream &out,
                                  const UniverseChannelAddress &address) {
    return out << address.ToString();
  }

 private:
  unsigned int m_universe;
  unsigned int m_channel;
};
/**
 * @}
 */
}  // namespace dmx
}  // namespace ola
#endif  // INCLUDE_OLA_DMX_UNIVERSECHANNELADDRESS_H_
