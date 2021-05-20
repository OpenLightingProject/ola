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
 * BaseSnifferReader.h
 * Common RDM Sniffer code for logic analyzer based sniffers.
 * Copyright (C) 2021 Peter Newman
 */

#ifndef TOOLS_LOGIC_BASESNIFFERREADER_H_
#define TOOLS_LOGIC_BASESNIFFERREADER_H_

#include <ola/io/SelectServer.h>
#include <ola/rdm/CommandPrinter.h>
#include <ola/rdm/PidStoreHelper.h>

#include "libs/sniffer/DMXSignalProcessor.h"

class BaseSnifferReader {
 public:
    BaseSnifferReader(ola::io::SelectServer *ss, unsigned int sample_rate);
    ~BaseSnifferReader();

    void FrameReceived(const uint8_t *data, unsigned int length);

    virtual void Stop() = 0;

    virtual bool IsConnected() const = 0;

 protected:
    ola::io::SelectServer *m_ss;
    DMXSignalProcessor m_signal_processor;
    ola::rdm::PidStoreHelper m_pid_helper;
    ola::rdm::CommandPrinter m_command_printer;

    void DisplayDMXFrame(const uint8_t *data, unsigned int length);
    void DisplayRDMFrame(const uint8_t *data, unsigned int length);
    void DisplayAlternateFrame(const uint8_t *data, unsigned int length);
    void DisplayRawData(const uint8_t *data, unsigned int length);
};
#endif  // TOOLS_LOGIC_BASESNIFFERREADER_H_
