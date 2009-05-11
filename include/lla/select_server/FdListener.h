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
 * FdListener.h
 * The interface for the fdlistener class
 * Copyright (C) 2005  Simon Newton
 */

#ifndef FDLISTENER_H
#define FDLISTENER_H

namespace lla {
namespace select_server {

class FDListener {

  public :
    FDListener() {};
    virtual ~FDListener() {};
    virtual int FDReady() = 0;

  private:
    FDListener(const FDListener&);
    FDListener& operator=(const FDListener&);
};

} // select_server
} // lla
#endif
