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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
# __init__.py
# Copyright (C) 2018 Shenghao Yang

import logging

"""Initialization code for the ola python library"""

__author__ = 'me@shenghaoyang.info (Shenghao Yang)'


ola_handler = logging.StreamHandler()
ola_handler.setFormatter(logging.Formatter(logging.BASIC_FORMAT))
ola_logger = logging.getLogger(__name__)
ola_logger.addHandler(ola_handler)
ola_logger.propagate = False
