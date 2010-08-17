goog.require('goog.dom');
goog.require('goog.ui.Control');


goog.provide('ola.ControlList');


/**
 * The base class for a factory which produces control items
 * @class
 */
ola.ControlItemFactory = function() {
}

/**
 * @returns an instance of a ControlItem
 */
ola.ControlItemFactory.prototype.newItem = function(data) {
}


/**
 * The base class for an item in the control list
 */
ola.ControlItem = function(data) {
}


/**
 * Update this item with from new data
 */
ola.ControlItem.prototype.Update = function(new_data) {
}


/**
 *
 * @returns the dom element that corresponds to this ControlItem
 */
ola.ControlItem.prototype.getElement = function() {
}


/**
 * Represents a list on controls that are updated from a data model
 * @param container_id the id of the container to use as the control list.
 * @param item_factory a ControlItemFactory class to produce the ControlItems
 */
ola.ControlList = function(container_id, item_factory) {
  this.container_id = container_id;
  this.container = goog.dom.$(container_id);
  /*
  this.create_function = create_function;
  this.update_function = update_function;
  */

  this.item_factory = item_factory;
}


/**
 * Update this list from a new data set
 */
ola.ControlList.prototype.UpdateFromData = function(data_list) {
  var data_count = data_list.length;
  var index = 0;
  var child_element = goog.dom.getFirstElementChild(this.container);

  while (child_element != undefined && index != data_count) {
    var data_id = data_list[index].id;
    var span = goog.dom.getFirstElementChild(child_element);
    var span_id = child_element.id.split("_")[2];

    if (data_id < span_id) {
      var item = this.item_factory.newItem(data_list[index]);
      //var item = this.create_function(data_list[index]);
      var new_id = this.container_id + '_' + item.id;
      item.id = new_id;
      goog.dom.insertSiblingBefore(item, child_element);
      var control = new goog.ui.Control(item);
      control.decorate(item);
      index++;
    } else if (span_id == data_id) {
      this.update_function(child_element, data_list[index]);
      child_element = goog.dom.getNextElementSibling(child_element);
      index++;
    } else {
      // remove the old one
      var old_child = child_element;
      child_element = goog.dom.getNextElementSibling(child_element);
      goog.dom.removeNode(old_child);
    }
  }

  // remove any remaining nodes
  while (child_element != undefined) {
    var old_child = child_element;
    child_element = goog.dom.getNextElementSibling(child_element);
    goog.dom.removeNode(old_child);
  }

  // add any remaining plugins
  while (index != data_count) {
    var new_id = this.container_id + '_' + data_list[index].id;
    ///var item = this.create_function(data_list[index]);
    var item = this.item_factory.newItem(data_list[index]);
    item.id = new_id;
    goog.dom.appendChild(this.container, item);
    var control = new goog.ui.Control(item);
    control.decorate(item);
    index++;
  }
}
