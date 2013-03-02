#!/bin/sh
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# SimpleRpcControllerTest.sh
# Copyright (C) 2013 Simon Newton

# I can't find a way to execute an automake test with the particular Python
# interpreter found by the configure script. To work around this we wrap the
# test in a shell script and use the interpreter in $PYTHON. If this is empty
# we'll execute the script as normal.
${PYTHON} ./SimpleRpcControllerTest.py
exit $?
