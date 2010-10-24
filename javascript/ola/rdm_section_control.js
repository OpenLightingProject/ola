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
 * An item in the RDM section list
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.events');
goog.require('goog.ui.Control');

goog.provide('ola.RdmSectionItem');
goog.provide('ola.RdmSectionControl');
goog.provide('ola.RdmSectionControlFactory');


/**
 * An object which represents a section in a list.
 * @param {Object} data the data to use to construct this item.
 * @constructor
 */
ola.RdmSectionItem = function(data) {
  this._id = data['id'];
  this._name = data['name'];
  this._hint = data['hint'];
};
goog.inherits(ola.RdmSectionItem, ola.common.DataItem);


/**
 * Get the id of this universe.
 * @return {number} the uid id.
 */
ola.RdmSectionItem.prototype.id = function() { return this._id; };
ola.RdmSectionItem.prototype.hint = function() { return this._hint; };

/**
 * Return the uid as a string
 * @return {number} the uid as a string
 */
ola.RdmSectionItem.prototype.toString = function() {
  return this._name;
};


/**
 * Compare one uid to another.
 * @param {ola.common.DataItem} other the other item to compare to.
 * @return {number} -1 if less than, 1 if greater than, 0 if equal.
 */
ola.RdmSectionItem.prototype.compare = function(other) {
  if (this._name < other._name) {
    return -1;
  } else if (this._name > other._name) {
    return 1;
  } else {
    return 0;
  }
};


/**
 * An section navigation control element.
 * @constructor
 */
ola.RdmSectionControl = function(item, callback, opt_renderer, opt_domHelper) {
  ola.common.GenericControl.call(this, item, callback, opt_renderer, opt_domHelper);
  this.setContent(item.toString());
};
goog.inherits(ola.RdmSectionControl, ola.common.GenericControl);


/**
 * Setup the event handler for this object.
 */
ola.RdmSectionControl.prototype.enterDocument = function() {
  ola.UniverseControl.superClass_.enterDocument.call(this);
  this.getElement().title = this.item().toString();
};


/**
 * Update this item with from new data.
 */
ola.RdmSectionControl.prototype.update = function(item) {
  this.setContent(item.toString());
};


/**
 * The base class for a factory which produces control items
 * @param {function()} callback the function to call when an item is clicked.
 *   The * first arg is the item id.
 * @constructor
 */
ola.RdmSectionControlFactory = function(callback) {
  this.callback = callback;
};


/**
 * Create a new RdmSectionControl object from some data
 * @param {Object} data the data to use for the control.
 * @return {ola.RdmSectionControl}
 */
ola.RdmSectionControlFactory.prototype.newComponent = function(data) {
  return new ola.RdmSectionControl(data, this.callback);
};
