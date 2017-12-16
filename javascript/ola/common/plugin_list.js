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
 * Object that represent a plugin item.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('ola.common.DataItem');

goog.provide('ola.common.PluginControl');
goog.provide('ola.common.PluginControlFactory');
goog.provide('ola.common.PluginItem');


/**
 * An object which represents a plugin in the list
 * @param {Object} data the data to use to construct this item.
 * @constructor
 */
ola.common.PluginItem = function(data) {
  this._id = data['id'];
  this._name = data['name'];
};
goog.inherits(ola.common.PluginItem, ola.common.DataItem);


/**
 * Get the id of this universe.
 * @return {number} the universe id.
 */
ola.common.PluginItem.prototype.id = function() { return this._id; };


/**
 * Return the universe name
 * @return {string} the name.
 */
ola.common.PluginItem.prototype.name = function() { return this._name; };

/**
 * Compare one item to another.
 * @param {ola.common.DataItem} other the other item to compare to.
 * @return {number} -1 if less than, 1 if greater than, 0 if equal.
 */
ola.common.PluginItem.prototype.compare = function(other) {
  if (this.name() > other.name()) {
    return 1;
  } else if (this.name() < other.name()) {
    return -1;
  }
  return 0;
};

/**
 * An Plugin navigation control element.
 * @constructor
 * @param {Object} item the item to add.
 * @param {function()} callback the function to run when the item is clicked.
 * @param {goog.ui.ControlRenderer=} opt_renderer Renderer used to render or
 *     decorate the component; defaults to {@link goog.ui.ControlRenderer}.
 * @param {goog.dom.DomHelper=} opt_domHelper An optional DOM helper.
 */
ola.PluginControl = function(item, callback, opt_renderer, opt_domHelper) {
  ola.common.GenericControl.call(this,
                                 item,
                                 callback,
                                 opt_renderer,
                                 opt_domHelper);
  this.setContent(item.name());
};
goog.inherits(ola.PluginControl, ola.common.GenericControl);


/**
 * Setup the event handler for this object.
 */
ola.PluginControl.prototype.enterDocument = function() {
  ola.PluginControl.superClass_.enterDocument.call(this);
  this.getElement().title = this._item.name() + ' Plugin';
};


/**
 * A factory which produces PluginControls
 * @param {function()} callback the function called when the control is clicked.
 * @constructor
 */
ola.common.PluginControlFactory = function(callback) {
  this.callback = callback;
};


/**
 * @param {Object} data The new data for the row.
 * @return {ola.PluginControl} an instance of a PluginRow.
 */
ola.common.PluginControlFactory.prototype.newComponent = function(data) {
  return new ola.PluginControl(data, this.callback);
};



