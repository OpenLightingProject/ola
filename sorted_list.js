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

goog.require('goog.dom');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');

goog.provide('ola.SortedList');

var ola = ola || {}

/**
 * The base class for a factory which produces control items
 * @class
 */
ola.SortedListComponentFactory = function() {
}

/**
 * @returns an instance of a SortedListComponent
 */
ola.SortedListComponentFactory.prototype.newComponent = function(data) {
}


/**
 * The base class for an item in the control list
 */
ola.SortedListComponent = function(data, opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
  this.data = data;
};
goog.inherits(ola.SortedListComponent, goog.ui.Component);


/**
 * Update this item with from new data
 */
ola.SortedListComponent.prototype.Id = function() {
  return this.data['id'];
}


/**
 * Update this item with from new data
 */
ola.SortedListComponent.prototype.Update = function(new_data) {
  this.data = new_data;
}


/**
 * Represents a list on controls that are updated from a data model
 * @param container_id the id of the container to use as the control list.
 * @param component_factory a SortedListComponentFactory class to produce the
 *   SortedListComponents
 */
ola.SortedList = function(container_id, component_factory) {
  this.container_id = container_id;
  this.container = new goog.ui.Container();
  this.container.decorate(goog.dom.$(container_id));
  this.component_factory = component_factory;
}


/**
 * Update this list from a new data set
 */
ola.SortedList.prototype.UpdateFromData = function(data_list) {
  var component_index = 0;
  var data_index = 0;
  var data_count = data_list.length;

  while (component_index != this.container.getChildCount() &&
         data_index != data_count) {
    var data_id = data_list[data_index]['id'];
    var current_component = this.container.getChildAt(component_index);
    var component_id = current_component.Id();

    if (data_id < component_id) {
      var component = this.component_factory.newComponent(
          data_list[data_index]);
      this.container.addChildAt(component, component_index, true);
      data_index++;
      component_index++;
    } else if (component_id == data_id) {
      current_component.Update(data_list[data_index]);
      component_index++;
      data_index++;
    } else {
      delete this.container.removeChild(current_component, true);
    }
  }

  // remove any remaining nodes
  while (component_index < this.container.getChildCount()) {
    delete this.container.removeChildAt(component_index, true);
  }

  // add any remaining items
  for (; data_index < data_count; data_index++) {
    var component = this.component_factory.newComponent(data_list[data_index]);
    this.container.addChild(component, true);
  }
}


/**
 * Clear the entire list
 */
ola.SortedList.prototype.Clear = function() {
  while (this.container.getChildCount()) {
    delete this.container.removeChildAt(0, true);
  }
}
