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

goog.provide('ola.UidControl');
goog.provide('ola.UidControlFactory');

var ola = ola || {}


/**
 * The class for an item in the uid list.
 * @param data {Object} the data to build this UID control from
 * @param callback {function} the function to call when clicked
 * @constructor
 */
ola.UidControl = function(data, callback, opt_renderer, opt_domHelper) {
  goog.ui.Control.call(this, data['uid'], opt_renderer, opt_domHelper);
  // id is a float in the form manufacturer.device
  this.id = data['id'];
  // the actual UID of this item
  this.uid = data['uid'];
  this.callback = callback;
};
goog.inherits(ola.UidControl, goog.ui.Control);


/**
 * Return the ID of this item. The ID is a float in the form
 * manufactuer_id.device_id.
 * @return {number}
 */
ola.UidControl.prototype.Id = function() {
  return this.id
};


/**
 * Setup the event handlers for this control
 */
ola.UidControl.prototype.enterDocument = function() {
  goog.ui.Control.superClass_.enterDocument.call(this);
  this.getElement().title = 'UID ' + this.uid;
  goog.events.listen(this.getElement(),
                     goog.events.EventType.CLICK,
                     function() { this.callback(this.id); },
                     false,
                     this);
};


/**
 * Update this item with from new data.
 */
ola.UidControl.prototype.Update = function(new_data) {
  // We don't expect the uid to change here.
  this.setContent(new_data['uid']);
}


/**
 * The base class for a factory which produces control items
 * @param callback {function} the function to call when an item is clicked. The
 * first arg is the item id.
 * @constructor
 */
ola.UidControlFactory = function(callback) {
  this.callback = callback;
};


/**
 * Create a new UidControl object from some data
 * @param {Object} the data to use for the control.
 * @returns {ola.UidControl}
 */
ola.UidControlFactory.prototype.newComponent = function(data) {
  return new ola.UidControl(data, this.callback);
};
