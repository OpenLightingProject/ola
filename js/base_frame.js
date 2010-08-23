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
 * The base class that all other frames inherit from.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');

goog.provide('ola.BaseFrame');

var ola = ola || {}

/**
 * The base frame class
 * @param element_id the id of the div to use for the home frame
 * @constructor
 */
ola.BaseFrame = function(element_id) {
  this.element = goog.dom.$(element_id);
}


/**
 * Is this frame visible?
 */
ola.BaseFrame.prototype.IsVisible = function() {
  return this.element.style.display == 'block';
}


/**
 * Show this frame
 */
ola.BaseFrame.prototype.Show = function() {
  this.element.style.display = 'block';
}


/**
 * Hide this frame
 */
ola.BaseFrame.prototype.Hide = function() {
  this.element.style.display = 'none';
}
