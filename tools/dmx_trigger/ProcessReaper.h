/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ProcessReaper.h
 * Copyright (C) 2011 Simon Newton
 */


#ifndef TOOLS_DMX_TRIGGER_PROCESSREAPER_H_
#define TOOLS_DMX_TRIGGER_PROCESSREAPER_H_

#include <unistd.h>
#include <vector>

/**
 * The ProcessReaper cleans up after child processes.
 */
class ProcessReaper {
  public:
    ProcessReaper() {}
    ~ProcessReaper() {}

    void AddPid(pid_t pid) {
      m_pids.push_back(pid);
    }

  private:
    typedef std::vector<pid_t> PidVector;
    PidVector m_pids;
};
#endif  // TOOLS_DMX_TRIGGER_PROCESSREAPER_H_
