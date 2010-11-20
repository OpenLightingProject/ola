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
 * The base class for the tabs used in the universe pane.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.math');
goog.require('goog.style');
goog.provide('ola.common.BaseUniverseTab');


/**
 * The base tab for the universe view.
 * @param {element} the element to use for the tab
 * @constructor
 */
ola.common.BaseUniverseTab = function(element) {
  this.element = goog.dom.$(element);
  this.active = false;
  this.universe_id = undefined;
};


/**
 * Called when the universe changes.
 */
ola.common.BaseUniverseTab.prototype.setUniverse = function(universe_id) {
  this.universe_id = universe_id;
};


/**
 * Get the current universe;
 */
ola.common.BaseUniverseTab.prototype.getUniverse = function() {
  return this.universe_id;
};


/**
 * Called when the view port size changes
 */
ola.common.BaseUniverseTab.prototype.sizeChanged = function(frame_size) {
  goog.style.setBorderBoxSize(
      this.element,
      new goog.math.Size(frame_size.width - 7, frame_size.height - 34));
};


/**
 * Called when the tab becomes visible.
 */
ola.common.BaseUniverseTab.prototype.setActive = function(state) {
  this.active = state;
};


/**
 * Returns true if this tab is active (visible);
 */
ola.common.BaseUniverseTab.prototype.isActive = function() {
  return this.active;
};
