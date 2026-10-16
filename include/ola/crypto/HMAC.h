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
 * HMAC.h
 * HMAC Crypto functions.
 * Copyright (C) 2026 Peter Newman
 */

/**
 * @file ola/crypto/HMAC.h
 * @brief HMAC crypto functions
 */

#ifndef INCLUDE_OLA_CRYPTO_HMAC_H_
#define INCLUDE_OLA_CRYPTO_HMAC_H_

#include <stdint.h>

namespace ola {
namespace crypto {

/**
 * @brief Generate the SHA256 HMAC of some data.
 * @param[in] key pointer to the key
 * @param[in] key_length length of the key
 * @param[in] data pointer to the data to generate the HMAC of
 * @param[in] data_length length of the data to generate the HMAC of
 * @param[out] hmac pointer to where the HMAC will be stored
 * @param[out] hmac_length pointer to where the length of the generated HMAC
 *   will be stored
 */
bool HMACSHA256(const uint8_t *key,
                const unsigned int key_length,
                const uint8_t *data,
                const unsigned int data_length,
                uint8_t *hmac,
                unsigned int *hmac_length);
}  // namespace crypto
}  // namespace ola
#endif  // INCLUDE_OLA_CRYPTO_HMAC_H_
