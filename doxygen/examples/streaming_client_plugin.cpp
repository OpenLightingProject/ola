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
 * Copyright (C) 2014 Simon Newton
 */
//! [Streaming Client Plugin] NOLINT(whitespace/comments)
#include <dlfcn.h>
#include <ola/client/StreamingClient.h>
#include <ola/DmxBuffer.h>
#include <ola/client/Module.h>

#include <iostream>

using std::cout;
using std::endl;

/**
 * @brief Load a symbol.
 */
template <typename FunctionType>
FunctionType LoadSymbol(void *module, const char *symbol) {
  FunctionType ptr = reinterpret_cast<FunctionType>(dlsym(module, symbol));
  if (!ptr) {
    cout << "Failed to find " << symbol << " " << dlerror() << endl;
  }
  return ptr;
}

int main() {
  ola::client::StreamingClientInterface *client = NULL;

  // Adjust to suit.
  void *module = dlopen("/usr/local/lib/libola.dylib", RTLD_NOW);
  // void *module = dlopen("/usr/local/lib/libola.so", RTLD_NOW);
  if (!module) {
    cout << "Failed to load" << dlerror() << endl;
    return -1;
  }

  ola_new_streaming_client_t *new_client_ptr =
      LoadSymbol<ola_new_streaming_client_t*>(module, OLA_NEW_STREAMING_CLIENT);
  ola_delete_streaming_client_t *delete_client_ptr =
      LoadSymbol<ola_delete_streaming_client_t*>(
         module, OLA_DELETE_STREAMING_CLIENT);
  ola_new_dmxbuffer_t *new_buffer_ptr =
      LoadSymbol<ola_new_dmxbuffer_t*>(module, OLA_NEW_DMXBUFFER);
  ola_delete_dmxbuffer_t *delete_buffer_ptr =
      LoadSymbol<ola_delete_dmxbuffer_t*>(module, OLA_DELETE_DMXBUFFER);
  ola_set_dmxbuffer_t *set_buffer_ptr =
      LoadSymbol<ola_set_dmxbuffer_t*>(module, OLA_SET_DMXBUFFER);

  if (!(new_client_ptr && delete_client_ptr && new_buffer_ptr &&
        delete_buffer_ptr && set_buffer_ptr)) {
    return -1;
  }

  ola::client::StreamingClient::Options options;
  client = new_client_ptr(options);
  cout << "Setup() returned: " << client->Setup() << endl;

  ola::DmxBuffer *buffer = new_buffer_ptr();

  // Now actually send the DMX
  uint8_t data[] = {1, 2, 3};
  set_buffer_ptr(buffer, data, sizeof(data));
  const unsigned int universe = 1;
  client->SendDMX(universe, *buffer, ola::client::StreamingClient::SendArgs());

  client->Stop();

  delete_buffer_ptr(buffer);
  delete_client_ptr(client);

  if (module) {
    dlclose(module);
  }
}
//! [Streaming Client Plugin] NOLINT(whitespace/comments)
