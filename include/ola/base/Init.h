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
 * Init.h
 * A grab bag of functions useful for programs.
 * Copyright (C) 2012 Simon Newton
 */


#ifndef INCLUDE_OLA_BASE_INIT_H_
#define INCLUDE_OLA_BASE_INIT_H_

#include <ola/ExportMap.h>
#include <ola/Callback.h>

namespace ola {
// Call one of the following depending on if you're a Server or Client App
bool ServerInit(int argc, char *argv[], ExportMap *export_map);
bool AppInit(int argc, char *argv[]);

// Install a signal
bool InstallSignal(int signal, void(*fp)(int));

// Methods you probably don't need to use
bool InstallSEGVHandler();
void InitExportMap(int argc, char* argv[], ExportMap *export_map);
int Daemonise();
}  // ola
#endif  // INCLUDE_OLA_BASE_INIT_H_
