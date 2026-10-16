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
 * HMAC.cpp
 * HMAC Crypto functions.
 * Copyright (C) 2026 Peter Newman
 */

#include "ola/crypto/HMAC.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include "ola/Logging.h"

namespace ola {
namespace crypto {

bool HMACSHA256(const uint8_t *key,
                const unsigned int key_length,
                const uint8_t *data,
                const unsigned int data_length,
                uint8_t *hmac,
                unsigned int *hmac_length) {
  /*HMAC_CTX ctx;
  HMAC_Init(&ctx, key, key_length, );
  HMAC_Update(&ctx, data, data_length);
  HMAC_Final(&ctx, hmac, hmac_length);
  HMAC_cleanup(&ctx);*/
  HMAC(EVP_sha256(), key, key_length, data, data_length, hmac, hmac_length);

  return true;
}
}  // namespace crypto
}  // namespace ola
