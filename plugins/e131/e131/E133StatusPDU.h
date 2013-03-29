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
 * E133StatusPDU.cpp
 * The E133StatusPDU
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E133STATUSPDU_H_
#define PLUGINS_E131_E131_E133STATUSPDU_H_

#include <ola/io/IOStack.h>
#include <string>
#include "plugins/e131/e131/E133Enums.h"
#include "plugins/e131/e131/PDU.h"

namespace ola {
namespace plugin {
namespace e131 {

class E133StatusPDU : private PDU {
  public:
    static void PrependPDU(ola::io::IOStack *stack, E133StatusCode status_code,
                           const std::string &status);

  private:
    static const size_t MAX_STATUS_STRING_SIZE = 64;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_E133STATUSPDU_H_
