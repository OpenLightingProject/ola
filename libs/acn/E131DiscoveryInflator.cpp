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
 * E131DiscoveryInflator.cpp
 * An inflator for E1.31 discovery messages.
 * Copyright (C) 2014 Simon Newton
 */

#include <vector>
#include "ola/Logging.h"
#include "ola/base/Macro.h"
#include "libs/acn/E131DiscoveryInflator.h"

namespace ola {
namespace acn {

using std::vector;

unsigned int E131DiscoveryInflator::InflatePDUBlock(HeaderSet *headers,
                                                    const uint8_t *data,
                                                    unsigned int len) {
  if (!m_page_callback.get()) {
    return len;
  }

  PACK(
  struct page_header {
    uint8_t page_number;
    uint8_t last_page;
  });
  STATIC_ASSERT(sizeof(page_header) == 2);

  page_header header;
  if (len < sizeof(header)) {
    OLA_WARN << "Universe Discovery packet is too small: " << len;
    return len;
  }
  memcpy(reinterpret_cast<uint8_t*>(&header), data, sizeof(header));

  DiscoveryPage page(header.page_number, header.last_page);

  for (const uint8_t *ptr = data + sizeof(header); ptr != data + len;
       ptr += 2) {
    uint16_t universe;
    memcpy(reinterpret_cast<uint8_t*>(&universe), ptr, sizeof(universe));
    page.universes.push_back(ola::network::NetworkToHost(universe));
  }
  m_page_callback->Run(*headers, page);
  return len;
}
}  // namespace acn
}  // namespace ola
