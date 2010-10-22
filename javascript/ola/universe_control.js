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
 * The universe controls
 * Copyright (C) 2010 Simon Newton
 */

goog.require('ola.GenericControl');

goog.provide('ola.UniverseControl');


/**
 * An Universe navigation control element.
 * @constructor
 */
ola.UniverseControl = function(item, callback, opt_renderer, opt_domHelper) {
  ola.GenericControl.call(this, item, callback, opt_renderer, opt_domHelper);
  this.setContent(item.name());
};
goog.inherits(ola.UniverseControl, ola.GenericControl);


/**
 * Setup the event handler for this object.
 */
ola.UniverseControl.prototype.enterDocument = function() {
  ola.UniverseControl.superClass_.enterDocument.call(this);
  this.getElement().title = 'Universe ' + this._item.id();
};


/**
 * A factory which produces UniverseControls
 * @param {function()} the funtion called when the control is clicked.
 * @constructor
 */
ola.UniverseControlFactory = function(callback) {
  this.callback = callback;
};


/**
 * @return {ola.UniverseControl} an instance of a UniverseRow
 */
ola.UniverseControlFactory.prototype.newComponent = function(data) {
  return new ola.UniverseControl(data, this.callback);
};
