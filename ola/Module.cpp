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
 * Module.cpp
 * Contains the information required to load the StreamingClient as a plugin.
 * Copyright (C) 2014 Simon Newton
 */

#include <ola/client/Module.h>
#include <ola/DmxBuffer.h>
#include <ola/client/StreamingClient.h>

extern "C" {
ola::client::StreamingClientInterface *ola_new_streaming_client(
    const ola::client::StreamingClient::Options &options) {
  return new ola::client::StreamingClient(options);
}

void ola_delete_streaming_client(
    ola::client::StreamingClientInterface *client) {
  delete client;
}

ola::DmxBuffer *ola_new_dmxbuffer() {
  return new ola::DmxBuffer();
}

void ola_delete_dmxbuffer(ola::DmxBuffer *buffer) {
  delete buffer;
}

void ola_set_dmxbuffer(ola::DmxBuffer *buffer, const uint8_t *data,
                       unsigned int size) {
  buffer->Set(data, size);
}
}
