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
 * E131DiscoveryInflator.h
 * An inflator for E1.31 discovery messages.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef LIBS_ACN_E131DISCOVERYINFLATOR_H_
#define LIBS_ACN_E131DISCOVERYINFLATOR_H_

#include <memory>
#include <vector>
#include "ola/Callback.h"
#include "ola/acn/ACNVectors.h"
#include "libs/acn/BaseInflator.h"

namespace ola {
namespace acn {

class E131DiscoveryInflator: public InflatorInterface {
 public:
  struct DiscoveryPage {
   public:
    const uint8_t page_number;
    const uint8_t last_page;
    const uint32_t page_sequence;
    std::vector<uint16_t> universes;

    DiscoveryPage(uint8_t page_number, uint8_t last_page)
        : page_number(page_number),
          last_page(last_page),
          page_sequence(0) {  // not yet part of the standard
    }
  };
  typedef ola::Callback2<void, const HeaderSet&, const DiscoveryPage&>
      PageCallback;

  explicit E131DiscoveryInflator(PageCallback *callback)
      : m_page_callback(callback) {}

  uint32_t Id() const { return acn::VECTOR_E131_DISCOVERY; }

  unsigned int InflatePDUBlock(HeaderSet *headers,
                               const uint8_t *data,
                               unsigned int len);


 private:
  std::auto_ptr<PageCallback> m_page_callback;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_E131DISCOVERYINFLATOR_H_
