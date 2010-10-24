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

goog.require('goog.events');
goog.require('goog.ui.Control');

goog.provide('ola.common.DataItem');
goog.provide('ola.common.GenericControl');
goog.provide('ola.common.SortedList');

/**
 * The base data object that represents an item in a sorted list
 * @constructor
 */
ola.common.DataItem = function() {};

/**
 * Get the id of this node.
 * @return {number|string} the id of this element.
 */
ola.common.DataItem.prototype.id = goog.nullFunction;


/**
 * Compare one item to another.
 * @param {ola.common.DataItem} other the other item to compare to.
 * @return {number} -1 if less than, 1 if greater than, 0 if equal.
 */
ola.common.DataItem.prototype.compare = function(other) {
  if (this.id() > other.id()) {
    return 1;
  } else if (this.id() < other.id()) {
    return -1;
  }
  return 0;
}


/**
 * An Generic navigation control element.
 * @constructor
 */
ola.common.GenericControl = function(item, callback, opt_renderer, opt_domHelper) {
  goog.ui.Control.call(this, '', opt_renderer, opt_domHelper);
  this._item = item;
  this.callback = callback;
};
goog.inherits(ola.common.GenericControl, goog.ui.Control);


/**
 * Return the underlying GenericItem
 * @return {ola.GenericItem}
 */
ola.common.GenericControl.prototype.item = function() { return this._item; };


/**
 * This component can't be used to decorate
 */
ola.common.GenericControl.prototype.canDecorate = function() { return false; };


/**
 * Setup the event handler for this object.
 */
ola.common.GenericControl.prototype.enterDocument = function() {
  ola.common.GenericControl.superClass_.enterDocument.call(this);
  goog.events.listen(this.getElement(),
                     goog.events.EventType.CLICK,
                     function() { this.callback(this._item); },
                     false,
                     this);
};


/**
 * Update this item with from new data
 * @param {ola.GenericItem} the new item to update from.
 */
ola.common.GenericControl.prototype.update = function(item) {
  this.setContent(item.name());
};



/**
 * Represents a list on controls that are updated from a data model
 * @param {string} container_id the id of the container to use as the control
 *   list.
 * @param {ola.common.SortedListComponentFactory} component_factory a
 *   SortedListComponentFactory class to produce the SortedListComponents.
 * @constructor
 */
ola.common.SortedList = function(container_id, component_factory) {
  this.container = container_id;
  this.component_factory = component_factory;
};


/**
 * Update this list from a new list of data items.
 * @param {Array.<ola.common.DataItem>} item_list the new set of data items.
 */
ola.common.SortedList.prototype.updateFromData = function(item_list) {
  var component_index = 0;
  var item_index = 0;
  var item_count = item_list.length;

  item_list.sort(function(a, b) { return a.compare(b); });

  while (component_index != this.container.getChildCount() &&
         item_index != item_count) {
    var item = item_list[item_index];

    var current_component = this.container.getChildAt(component_index);
    var component_item = current_component.item();

    var comparison = item.compare(component_item);
    if (comparison == -1) {
      var component = this.component_factory.newComponent(item);
      this.container.addChildAt(component, component_index, true);
      item_index++;
      component_index++;
    } else if (comparison == 0) {
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
