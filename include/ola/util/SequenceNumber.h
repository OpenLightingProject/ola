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
 * SequenceNumber.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_UTIL_SEQUENCENUMBER_H_
#define INCLUDE_OLA_UTIL_SEQUENCENUMBER_H_

namespace ola {

/**
 * SequenceNumber, this ensures that we increment the sequence number whenver
 * we go to use it.
 */
template<typename sequence_type>
class SequenceNumber {
 public:
    SequenceNumber(): m_sequence_number(0) { }
    explicit SequenceNumber(sequence_type initial_value)
      : m_sequence_number(initial_value) {
    }

    sequence_type Next() { return m_sequence_number++; }

 private:
    sequence_type m_sequence_number;
};
}  // namespace ola
#endif  // INCLUDE_OLA_UTIL_SEQUENCENUMBER_H_
