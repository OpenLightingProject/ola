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
 * Copyright (C) 2010 Simon Newton
 */
//! [Tutorial Example] NOLINT(whitespace/comments)
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/client/ClientWrapper.h>
#include <string>

static const unsigned int UNIVERSE = 1;

// Called when universe registration completes.
void RegisterComplete(const ola::client::Result& result) {
  if (!result.Success()) {
    OLA_WARN << "Failed to register universe: " << result.Error();
  }
}

// Called when new DMX data arrives.
void NewDmx(const ola::client::DMXMetadata &metadata,
            const ola::DmxBuffer &data) {
  std::cout << "Received " << data.Size()
            << " channels for universe " << metadata.universe
            << ", priority " << static_cast<int>(metadata.priority)
            << std::endl;
}

int main() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  ola::client::OlaClientWrapper wrapper;
  if (!wrapper.Setup())
    exit(1);

  ola::client::OlaClient *client = wrapper.GetClient();
  // Set the callback and register our interest in this universe
  client->SetDMXCallback(ola::NewCallback(&NewDmx));
  client->RegisterUniverse(
      UNIVERSE, ola::client::REGISTER,
      ola::NewSingleCallback(&RegisterComplete));

  wrapper.GetSelectServer()->Run();
}
//! [Tutorial Example] NOLINT(whitespace/comments)
