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
 * An order preserving, list of UIDs
 * Copyright (C) 2010 Simon Newton
 */

goog.require('ola.common.DataItem');

goog.provide('ola.common.UidItem');
goog.provide('ola.common.UidControl');
goog.provide('ola.common.UidControlFactory');


/**
 * An object which represents a UID in a list.
 * @param {Object} data the data to use to construct this item.
 * @constructor
 */
ola.common.UidItem = function(data) {
  this._device_id = data['device_id'];
  this._manufacturer_id = data['manufacturer_id'];
  this._device = data['device'];
  this._manufacturer = data['manufacturer'];
};
goog.inherits(ola.common.UidItem, ola.common.DataItem);


/**
 * Get the id of this universe.
 * @return {number} the uid id.
 */
ola.common.UidItem.prototype.id = function() { return this.asString(); };


/**
 * Convert a number to the hex representation
 * @param {number} n the number to convert.
 * @param {number} padding the length to pad to
 * @return {string} the hex representation of the number.
 */
ola.common.UidItem.prototype._toHex = function(n, padding) {
  if (n < 0) {
    n = 0xffffffff + n + 1;
  }
  var s = n.toString(16);
  while (s.length < padding) {
    s = '0' + s;
  }
  return s;
};


/**
 * Return the string representation of the uid.
 * @return {string} the uid
 */
ola.common.UidItem.prototype.asString = function() {
  return (this._toHex(this._manufacturer_id, 4) + ':' +
          this._toHex(this._device_id, 8));
};


/**
 * Return the uid as a string
 * @return {number} the uid as a string
 */
ola.common.UidItem.prototype.toString = function() {
  var uid = "";
  if (this._manufacturer) {
    uid += this._manufacturer;
  }
  if (this._manufacturer && this._device) {
    uid += ", ";
  }
  if (this._device) {
    uid += this._device;
  }
  if (this._manufacturer || this._device) {
    uid += " [";
  }

  uid += this.asString();

  if (this._manufacturer || this._device) {
    uid += "]";
  }
  return uid;
};


/**
 * Compare one uid to another.
 * @param {ola.common.DataItem} other the other item to compare to.
 * @return {number} -1 if less than, 1 if greater than, 0 if equal.
 */
ola.common.UidItem.prototype.compare = function(other) {
  if (this._manufacturer_id > other._manufacturer_id) {
    return 1;
  } else if (this._manufacturer_id < other._manufacturer_id) {
    return -1;
  }
  return this._device_id - other._device_id;
};


/**
 * An UID navigation control element.
 * @constructor
 */
ola.common.UidControl = function(item, callback, opt_renderer, opt_domHelper) {
  ola.common.GenericControl.call(this, item, callback, opt_renderer, opt_domHelper);
  this.setContent(item.toString());
};
goog.inherits(ola.common.UidControl, ola.common.GenericControl);


/**
 * Setup the event handler for this object.
 */
ola.common.UidControl.prototype.enterDocument = function() {
  ola.UniverseControl.superClass_.enterDocument.call(this);
  this.getElement().title = this.item().toString();
};


/**
 * Update this item with from new data.
 */
ola.common.UidControl.prototype.update = function(item) {
  // We don't expect the uid to change here.
  this.setContent(item.toString());
};


/**
 * The base class for a factory which produces control items
 * @param {function()} callback the function to call when an item is clicked.
 *   The * first arg is the item id.
 * @constructor
 */
ola.common.UidControlFactory = function(callback) {
  this.callback = callback;
};


/**
 * Create a new UidControl object from some data
 * @param {Object} data the data to use for the control.
 * @return {ola.common.UidControl}
 */
ola.common.UidControlFactory.prototype.newComponent = function(data) {
  return new ola.common.UidControl(data, this.callback);
};
