/**
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
 * The base class that all other frames inherit from.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');

goog.provide('ola.BaseFrame');


/**
 * The base frame class
 * @param {string} element_id the id of the div to use for the home frame.
 * @constructor
 */
ola.BaseFrame = function(element_id) {
  this.element = goog.dom.$(element_id);
};


/**
 * Check if this frame is visible.
 * @return {boolean} true if visible, false otherwise.
 */
ola.BaseFrame.prototype.IsVisible = function() {
  return this.element.style.display == 'block';
};


/**
 * Show this frame
 */
ola.BaseFrame.prototype.Show = function() {
  this.element.style.display = 'block';
};


/**
 * Hide this frame
 */
ola.BaseFrame.prototype.Hide = function() {
  this.element.style.display = 'none';
};


/**
 * Make this frame show the spinner
 */
ola.BaseFrame.prototype.setAsBusy = function() {
  this.element.innerHTML = (
      '<div align="center"><img src="/loader.gif"></div>');
};


/**
 * Make this frame show the spinner
 */
ola.BaseFrame.prototype.Clear = function() {
  this.element.innerHTML = '';
};
