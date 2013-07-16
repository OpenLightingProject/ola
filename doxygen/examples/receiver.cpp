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
 * Copyright (C) 2010 Simon Newton
 */
//! [Tutorial Example]
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/OlaClientWrapper.h>
#include <string>

static const unsigned int UNIVERSE = 1;

// Called when universe registration completes.
void RegisterComplete(const std::string& error) {
  if (!error.empty()) {
    OLA_WARN << "Failed to register universe";
  }
}

// Called when new DMX data arrives.
void NewDmx(unsigned int universe,
            const ola::DmxBuffer &data,
            const std::string &error) {
  if (error.empty()) {
    OLA_INFO << "Received " << data.Size()
             << " channels for universe " << universe;
  } else {
    OLA_WARN << "Receive failed: " << error;
  }
}

int main() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  ola::OlaCallbackClientWrapper wrapper;
  if (!wrapper.Setup())
    exit(1);

  ola::OlaCallbackClient *client = wrapper.GetClient();
  // Set the callback and register our interest in this universe
  client->SetDmxCallback(ola::NewCallback(&NewDmx));
  client->RegisterUniverse(
      UNIVERSE, ola::REGISTER, ola::NewSingleCallback(&RegisterComplete));

  wrapper.GetSelectServer()->Run();
}
//! [Tutorial Example]
