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
 * A sorted list implementation that can be updated from a data model
 * Copyright (C) 2010 Simon Newton
 */

goog.provide('ola.DataItem');
goog.provide('ola.GenericControl');
goog.provide('ola.SortedList');

/**
 * The base data object that represents an item in a sorted list
 * @constructor
 */
ola.DataItem = function() {};

/**
 * Get the id of this node.
 * @return {number|string} the id of this element.
 */
ola.DataItem.prototype.id = goog.nullFunction;

/**
 * Get the sort key of this node
 * @return {number|string} the sort key for this element.
 */
ola.DataItem.prototype.sortKey = goog.nullFunction;


/**
 * An Generic navigation control element.
 * @constructor
 */
ola.GenericControl = function(item, callback, opt_renderer, opt_domHelper) {
  goog.ui.Control.call(this, '', opt_renderer, opt_domHelper);
  this._item = item;
  this.callback = callback;
};
goog.inherits(ola.GenericControl, goog.ui.Control);


/**
 * Return the underlying GenericItem
 * @return {ola.GenericItem}
 */
ola.GenericControl.prototype.item = function() { return this._item; };


/**
 * This component can't be used to decorate
 */
ola.GenericControl.prototype.canDecorate = function() { return false; };


/**
 * Setup the event handler for this object.
 */
ola.GenericControl.prototype.enterDocument = function() {
  ola.GenericControl.superClass_.enterDocument.call(this);
  goog.events.listen(this.getElement(),
                     goog.events.EventType.CLICK,
                     function() { this.callback(this._item.id()); },
                     false,
                     this);
};


/**
 * Update this item with from new data
 * @param {ola.GenericItem} the new item to update from.
 */
ola.GenericControl.prototype.update = function(item) {
  this.setContent(item.name());
};



/**
 * Represents a list on controls that are updated from a data model
 * @param {string} container_id the id of the container to use as the control
 *   list.
 * @param {ola.SortedListComponentFactory} component_factory a
 *   SortedListComponentFactory class to produce the SortedListComponents.
 * @constructor
 */
ola.SortedList = function(container_id, component_factory) {
  this.container = container_id;
  this.component_factory = component_factory;
};


/**
 * Update this list from a new list of data items.
 * @param {Array.<ola.DataItem>} item_list the new set of data items.
 */
ola.SortedList.prototype.updateFromData = function(item_list) {
  var component_index = 0;
  var item_index = 0;
  var item_count = item_list.length;

  while (component_index != this.container.getChildCount() &&
         item_index != item_count) {
    var item_id = item_list[item_index].id();
    var current_component = this.container.getChildAt(component_index);
    var component_id = current_component.item().id();

    if (item_id < component_id) {
      var component = this.component_factory.newComponent(
          item_list[item_index]);
      this.container.addChildAt(component, component_index, true);
      item_index++;
      component_index++;
    } else if (component_id == item_id) {
      current_component.update(item_list[item_index]);
      component_index++;
      item_index++;
    } else {
      delete this.container.removeChild(current_component, true);
    }
  }

  // remove any remaining nodes
  while (component_index < this.container.getChildCount()) {
    delete this.container.removeChildAt(component_index, true);
  }

  // add any remaining items
  for (; item_index < item_count; item_index++) {
    var component = this.component_factory.newComponent(item_list[item_index]);
    this.container.addChild(component, true);
  }
};
