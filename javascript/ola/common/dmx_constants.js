/**
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
 * DMX constants.
 * Copyright (C) 2013 Peter Newman
 */

goog.provide('ola.common.DmxConstants');

/** The first channel number @type {number} */
ola.common.DmxConstants.MIN_CHANNEL_NUMBER = 1;
/** The number of channels and max channel number @type {number} */
ola.common.DmxConstants.MAX_CHANNEL_NUMBER = 512;
/** The minimum value of a channel @type {number} */
ola.common.DmxConstants.MIN_CHANNEL_VALUE = 0;
/** The maximum value of a channel @type {number} */
ola.common.DmxConstants.MAX_CHANNEL_VALUE = 255;
/** The channel level at which to change the background colour @type {number} */
ola.common.DmxConstants.BACKGROUND_CHANGE_CHANNEL_LEVEL = 90;
