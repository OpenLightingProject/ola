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
  this._id = data['id'];
  this._uid = data['uid'];
};
goog.inherits(ola.UidItem, ola.DataItem);


/**
 * Get the id of this universe.
 * @return {number} the universe id.
 */
ola.UidItem.prototype.id = function() { return this._id; };


/**
 * Get the sort key of this universe.
 * @return {number} the unvierse id.
 */
ola.UidItem.prototype.sortKey = function() { return this._id; };


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
};
goog.inherits(ola.UidControl, ola.GenericControl);


/**
 * Setup the event handler for this object.
 */
ola.UidControl.prototype.enterDocument = function() {
  ola.UniverseControl.superClass_.enterDocument.call(this);
  this.getElement().title = this._item.uid();
};


/**
 * Update this item with from new data.
 */
ola.UidControl.prototype.update = function(item) {
  // We don't expect the uid to change here.
  this.setContent(item.uid());
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
