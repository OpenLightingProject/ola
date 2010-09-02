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
 * An item in the UID list
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.events');
goog.require('goog.ui.Control');

goog.provide('ola.UidItem');
goog.provide('ola.UidControl');
goog.provide('ola.UidControlFactory');

var ola = ola || {};


/**
 * An object which represents a UID in a list.
 * @param {Object} data the data to use to construct this item.
 * @constructor
 */
ola.UidItem = function(data) {
  this._device_id = data['device_id'];
  this._manufacturer_id = data['manufacturer_id'];
};
goog.inherits(ola.UidItem, ola.DataItem);


/**
 * Get the id of this universe.
 * @return {number} the universe id.
 */
ola.UidItem.prototype.id = function() { return this._id; };


/**
 * Convert a number to the hex representation
 * @param {number} n the number to convert.
 * @return {string} the hex representation of the number.
 */
ola.UidItem.prototype._toHex = function(n) {
  if (n < 0) {
    n = 0xffffffff + n + 1;
  }
  var s = n.toString(16);
  ola.logger.info(s);
  return s;
};


/**
 * Return the device id
 * @return {number} the device id
 */
ola.UidItem.prototype.toString = function() {
  return (this._toHex(this._manufacturer_id) + ':' +
          this._toHex(this._device_id));
};


/**
 * Compare one uid to another.
 * @param {ola.DataItem} other the other item to compare to.
 * @return {number} -1 if less than, 1 if greater than, 0 if equal.
 */
ola.UidItem.prototype.compare = function(other) {
  if (this._manufacturer_id > other._manufacturer_id) {
    return 1;
  } else if (this._manufacturer_id < other._manufacturer_id) {
    return -1;
  }
  return this.device_id - other.device_id;
};


/**
 * Return the UID string
 * @return {string} the UID.
 */
ola.UidItem.prototype.uid = function() { return this._uid; };


/**
 * An UID navigation control element.
 * @constructor
 */
ola.UidControl = function(item, callback, opt_renderer, opt_domHelper) {
  ola.GenericControl.call(this, item, callback, opt_renderer, opt_domHelper);
  this.setContent(item.toString());
};
goog.inherits(ola.UidControl, ola.GenericControl);


/**
 * Setup the event handler for this object.
 */
ola.UidControl.prototype.enterDocument = function() {
  ola.UniverseControl.superClass_.enterDocument.call(this);
  this.getElement().title = this.item().toString();
};


/**
 * Update this item with from new data.
 */
ola.UidControl.prototype.update = function(item) {
  // We don't expect the uid to change here.
  this.setContent(item.toString());
};


/**
 * The base class for a factory which produces control items
 * @param {function()} callback the function to call when an item is clicked.
 *   The * first arg is the item id.
 * @constructor
 */
ola.UidControlFactory = function(callback) {
  this.callback = callback;
};


/**
 * Create a new UidControl object from some data
 * @param {Object} data the data to use for the control.
 * @return {ola.UidControl}
 */
ola.UidControlFactory.prototype.newComponent = function(data) {
  return new ola.UidControl(data, this.callback);
};
