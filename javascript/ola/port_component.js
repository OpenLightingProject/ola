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
 * The new universe frame.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.Checkbox');
goog.require('goog.ui.Component');
goog.require('goog.ui.MenuItem');
goog.require('goog.ui.Select');

goog.provide('ola.AvailablePortComponent');
goog.provide('ola.AvailablePortComponentFactory');
goog.provide('ola.PortComponent');
goog.provide('ola.PortComponentFactory');

var ola = ola || {};


/**
 * A port bound to a universe
 * @constructor
 */
ola.PortComponent = function(data, opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
  this.data = data;
};
goog.inherits(ola.PortComponent, goog.ui.Component);


/**
 * This component can't be used to decorate
 */
ola.PortComponent.prototype.canDecorate = function() {
  return false;
};


/**
 * Create the dom for this component
 */
ola.PortComponent.prototype.createDom = function() {
  this.tr = this.dom_.createDom('tr', {});
  this.tr.style.cursor = 'pointer';
  var td = goog.dom.createDom('td', {}, '');
  this.dom_.appendChild(this.tr, td);
  this.checkbox = new goog.ui.Checkbox();
  this.checkbox.setChecked(true);
  this.checkbox.render(td);
  this.dom_.appendChild(
      this.tr, goog.dom.createDom('td', {}, this.data['device']));
  this.dom_.appendChild(this.tr,
      goog.dom.createDom('td', {}, this.data['description']));

  var priority = this.data['priority'];

  if (priority == undefined) {
    // this port doesn't support priorities at all
    this.dom_.appendChild(
      this.tr, goog.dom.createDom('td', {}, 'Not supported'));
  } else {
    this.priority_input = goog.dom.createElement('input');
    this.priority_input.value = priority['value'];
    this.priority_input.maxLength = 3;
    this.priority_input.size = 3;
    if (priority['current_mode'] == undefined) {
      // this port only supports static priorities
      var td = goog.dom.createDom('td', {}, this.priority_input);
      this.dom_.appendChild(this.tr, td);
    } else {
      // this port supports both modes
      this.priority_select = new goog.ui.Select();
      this.priority_select.addItem(new goog.ui.MenuItem('Inherit'));
      this.priority_select.addItem(new goog.ui.MenuItem('Override'));
      this.priority_select.setSelectedIndex(
        priority['current_mode'] == 'inherit' ? 0 : 1);
      this._prioritySelectChanged();

      var td = goog.dom.createElement('td');
      this.priority_select.render(td);
      this.dom_.appendChild(td, this.priority_input);
      this.dom_.appendChild(this.tr, td);
    }
  }
  this.setElementInternal(this.tr);

};


/**
 * Setup the event handlers
 */
ola.PortComponent.prototype.enterDocument = function() {
  if (this.priority_select != undefined) {
    goog.events.listen(
        this.priority_select,
        goog.ui.Component.EventType.ACTION,
        this._prioritySelectChanged,
        false, this);

    // don't toggle the check box if we're changing priorities
    goog.events.listen(
        this.priority_select.getElement(),
        goog.events.EventType.CLICK,
        function(e) {
          e.stopPropagation();
        });
  }

  if (this.priority_input != undefined) {
    // don't toggle the check box if we're changing priorities
    goog.events.listen(
        this.priority_input,
        goog.events.EventType.CLICK,
          function(e) {
            e.stopPropagation();
          });
  }

  goog.events.listen(this.tr,
                     goog.events.EventType.CLICK,
                     function() { this.checkbox.toggle(); },
                     false, this);
};


/**
 * Get the id of this item
 */
ola.PortComponent.prototype.Id = function() {
  return this.data['id'];
};


/**
 * Get the port id for this item
 */
ola.PortComponent.prototype.PortId = function() {
  return this.data['port_id'];
};


/**
 * Get the priority value for this port
 * @return {number|undefined} the priority value or undefined if this port
 * doesn't support priorities.
 */
ola.PortComponent.prototype.priority = function() {
  if (this.priority_input) {
    return this.priority_input.value;
  } else {
    return undefined;
  }
};


/**
 * Get the priority mode for this port
 * @return {string|undefined} the priority mode (inherit|override) or undefined
 *   if this port doesn't support priority modes.
 */
ola.PortComponent.prototype.priorityMode = function() {
  if (this.priority_select) {
    return this.priority_select.getValue();
  } else {
    return undefined;
  }
};


/**
 * Check is this was selected
 */
ola.PortComponent.prototype.IsSelected = function() {
  return this.checkbox.isChecked();
};


/**
 * Uncheck this box.
 */
ola.PortComponent.prototype.uncheck = function() {
  return this.checkbox.setChecked(goog.ui.Checkbox.State.UNCHECKED);
};


/**
 * Update this item with from new data
 */
ola.PortComponent.prototype.Update = function(new_data) {
  var element = this.getElement();
  var td = goog.dom.getFirstElementChild(element);
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['device'];
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['description'];
};


/**
 * Called when the port priority changes
 */
ola.PortComponent.prototype._prioritySelectChanged = function(e) {
  if (this.priority_select.getSelectedIndex()) {
    // override mode
    this.priority_input.style.visibility = 'visible';
  } else {
    // inherit mode
    this.priority_input.style.visibility = 'hidden';

  }
};


/**
 * The base class for a factory which produces PortComponents
 * @constructor
 */
ola.PortComponentFactory = function() {
};


/**
 * @return {ola.PortComponent} an instance of a PortComponent.
 */
ola.PortComponentFactory.prototype.newComponent = function(data) {
  return new ola.PortComponent(data);
};


/**
 * A line in the available ports list.
 * @constructor
 */
ola.AvailablePortComponent = function(data, opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
  this.data = data;
};
goog.inherits(ola.AvailablePortComponent, goog.ui.Component);


/**
 * This component can't be used to decorate
 */
ola.AvailablePortComponent.prototype.canDecorate = function() {
  return false;
};


/**
 * Create the dom for this component
 */
ola.AvailablePortComponent.prototype.createDom = function() {
  var tr = this.dom_.createDom('tr', {});
  tr.style.cursor = 'pointer';
  var td = goog.dom.createDom('td', {}, '');
  this.dom_.appendChild(tr, td);
  this.checkbox = new goog.ui.Checkbox();
  this.checkbox.render(td);
  this.dom_.appendChild(tr, goog.dom.createDom('td', {}, this.data['device']));
  this.dom_.appendChild(tr, goog.dom.createDom('td', {},
      this.data['is_output'] ? 'Output' : 'Input'));
  this.dom_.appendChild(tr, goog.dom.createDom('td', {},
      this.data['description']));
  this.setElementInternal(tr);

  goog.events.listen(tr,
                     goog.events.EventType.CLICK,
                     function() { this.checkbox.toggle(); },
                     false, this);
};


/**
 * Get the id of this item
 */
ola.AvailablePortComponent.prototype.Id = function() {
  return this.data['id'];
};


/**
 * Get the port id for this item
 */
ola.AvailablePortComponent.prototype.PortId = function() {
  return this.data['port_id'];
};

/**
 * Check is this was selected
 */
ola.AvailablePortComponent.prototype.IsSelected = function() {
  return this.checkbox.isChecked();
};


/**
 * Update this item with from new data
 */
ola.AvailablePortComponent.prototype.Update = function(new_data) {
  var element = this.getElement();
  var td = goog.dom.getFirstElementChild(element);
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['device'];
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['is_output'] ? 'Output' : 'Input';
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['description'];
};


/**
 * The base class for a factory which produces AvailablePortComponents
 * @constructor
 */
ola.AvailablePortComponentFactory = function() {
};


/**
 * @return {ola.AvailablePortComponent} an instance of a AvailablePortComponent.
 */
ola.AvailablePortComponentFactory.prototype.newComponent = function(data) {
  return new ola.AvailablePortComponent(data);
};
