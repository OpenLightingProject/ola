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
 * Object that represent a plugin item.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('ola.DataItem');

goog.provide('ola.PluginItem');
goog.provide('ola.PluginControlFactory');


/**
 * An object which represents a plugin in the list
 * @param {Object} data the data to use to construct this item.
 * @constructor
 */
ola.PluginItem = function(data) {
  this._id = data['id'];
  this._name = data['name'];
};
goog.inherits(ola.PluginItem, ola.DataItem);


/**
 * Get the id of this universe.
 * @return {number} the universe id.
 */
ola.PluginItem.prototype.id = function() { return this._id; };


/**
 * Return the universe name
 * @return {string} the name.
 */
ola.PluginItem.prototype.name = function() { return this._name; };

/**
 * Compare one item to another.
 * @param {ola.DataItem} other the other item to compare to.
 * @return {number} -1 if less than, 1 if greater than, 0 if equal.
 */
ola.PluginItem.prototype.compare = function(other) {
  if (this.name() > other.name()) {
    return 1;
  } else if (this.name() < other.name()) {
    return -1;
  }
  return 0;
}

/**
 * An Plugin navigation control element.
 * @constructor
 */
ola.PluginControl = function(item, callback, opt_renderer, opt_domHelper) {
  ola.GenericControl.call(this, item, callback, opt_renderer, opt_domHelper);
  this.setContent(item.name());
};
goog.inherits(ola.PluginControl, ola.GenericControl);


/**
 * Setup the event handler for this object.
 */
ola.PluginControl.prototype.enterDocument = function() {
  ola.PluginControl.superClass_.enterDocument.call(this);
  this.getElement().title = this._item.name() + ' Plugin';
};


/**
 * A factory which produces PluginControls
 * @param {function()} the funtion called when the control is clicked.
 * @constructor
 */
ola.PluginControlFactory = function(callback) {
  this.callback = callback;
};


/**
 * @return {ola.PluginControl} an instance of a PluginRow
 */
ola.PluginControlFactory.prototype.newComponent = function(data) {
  return new ola.PluginControl(data, this.callback);
};



