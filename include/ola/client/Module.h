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
 * Module.h
 * Contains the information required to load the StreamingClient as a plugin.
 * Copyright (C) 2014 Simon Newton
 */
/**
 * @file
 * @brief Information required to use the StreamingClient as a plugin.
 *
 * Sometimes it's useful for client applications to avoid linking against
 * libola, say for instance if they install separately from OLA. By deferring
 * the linking and using libola as a plugin, clients can use OLA if it's
 * installed on the system or if not, take some other action like displaying a
 * message or using another output mechanism.
 *
 * This file provides plugin interfaces so that a client code can load
 * a small subset of libola at runtime. For each function we \#define the symbol
 * name and then provide the function type signature and the actual function
 * itself.
 *
 * http://www.tldp.org/HOWTO/html_single/C++-dlopen provides some good
 * background.
 *
 * @snippet streaming_client_plugin.cpp Streaming Client Plugin
 */

#ifndef INCLUDE_OLA_CLIENT_MODULE_H_
#define INCLUDE_OLA_CLIENT_MODULE_H_

#include <ola/client/StreamingClient.h>

namespace ola {

namespace client { class StreamingClientInterface; }
class DmxBuffer;

}  // namespace ola

/**
 * @brief The symbol for the function to create a new StreamingClient.
 */
#define OLA_NEW_STREAMING_CLIENT "ola_new_streaming_client"

/**
 * @brief The symbol for the function to delete a StreamingClient.
 */
#define OLA_DELETE_STREAMING_CLIENT "ola_delete_streaming_client"

/**
 * @brief A function pointer to create a new StreamingClient.
 */
typedef ola::client::StreamingClientInterface *ola_new_streaming_client_t(
    const ola::client::StreamingClient::Options &options);

/**
 * @brief A function pointer to delete a StreamingClient.
 */
typedef void ola_delete_streaming_client_t(
    ola::client::StreamingClientInterface *client);

extern "C" {
  ola::client::StreamingClientInterface *ola_new_streaming_client(
      const ola::client::StreamingClient::Options &options);
  void ola_delete_streaming_client(
      ola::client::StreamingClientInterface *client);
}

/**
 * @brief The symbol for the function to create a new DmxBuffer.
 */
#define OLA_NEW_DMXBUFFER "ola_new_dmxbuffer"

/**
 * @brief The symbol for the function to delete a DmxBuffer.
 */
#define OLA_DELETE_DMXBUFFER "ola_delete_dmxbuffer"

/**
 * @brief The symbol for the function to set the contents of a DmxBuffer.
 */
#define OLA_SET_DMXBUFFER "ola_set_dmxbuffer"

/**
 * @brief A function pointer to create a new DmxBuffer.
 */
typedef ola::DmxBuffer *ola_new_dmxbuffer_t();

/**
 * @brief A function pointer to delete a DmxBuffer.
 */
typedef void ola_delete_dmxbuffer_t(ola::DmxBuffer *buffer);

/**
 * @brief A function pointer to set the contents of a DmxBuffer.
 *
 * To avoid the vtable penalty with DmxBuffer we expose a function to call
 * Set(). If you want to access other methods in DmxBuffer please send a patch.
 */
typedef void ola_set_dmxbuffer_t(ola::DmxBuffer *buffer, const uint8_t *data,
                                 unsigned int size);

extern "C" {
  ola::DmxBuffer *ola_new_dmxbuffer();
  void ola_delete_dmxbuffer(ola::DmxBuffer *buffer);
  void ola_set_dmxbuffer(ola::DmxBuffer *buffer, const uint8_t *data,
                         unsigned int size);
}

#endif  // INCLUDE_OLA_CLIENT_MODULE_H_
