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
 * The table that holds a list of available ports.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.Checkbox');
goog.require('goog.ui.Component');
goog.require('goog.ui.MenuItem');
goog.require('goog.ui.Select');
goog.require('ola.LoggerWindow');
goog.require('ola.common.Server');

goog.provide('ola.Port');
goog.provide('ola.PortTable');


/**
 * A row in the available ports list.
 * @param {Object} data the data to build this row from.
 * @constructor
 * @param {goog.dom.DomHelper=} opt_domHelper An optional DOM helper.
 */
ola.Port = function(data, opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
  this.data = data;
};
goog.inherits(ola.Port, goog.ui.Component);

/**
 * This component can't be used to decorate
 * @return {bool} false.
 */
ola.Port.prototype.canDecorate = function() {
  return false;
};


/**
 * Create the dom for this component
 */
ola.Port.prototype.createDom = function() {
  var tr = this.dom_.createDom('tr', {});
  tr.style.cursor = 'pointer';
  var td = goog.dom.createDom('td', {}, '');
  this.checkbox = new goog.ui.Checkbox();
  this.checkbox.setChecked(true);
  this.checkbox.render(td);
  this.dom_.appendChild(tr, td);
  this.dom_.appendChild(
      tr, goog.dom.createDom('td', {}, this.data['device']));
  this.dom_.appendChild(tr,
      goog.dom.createDom('td', {}, this.data['description']));

  var priority = this.data['priority'];

  if (priority['priority_capability'] == undefined) {
    // this port doesn't support priorities at all
    this.dom_.appendChild(
      tr, goog.dom.createDom('td', {}, 'Not supported'));
  } else {
    // Now we know it supports priorities, lets create the common UI elements
    // for them
    this.priority_input = goog.dom.createElement('input');
    this.priority_input.value = priority['value'];
    this.priority_input.maxLength = 3;
    this.priority_input.size = 3;
    if (priority['priority_capability'] == 'full') {
      // this port supports both modes
      this.priority_select = new goog.ui.Select();
      this.priority_select.addItem(new goog.ui.MenuItem('Inherit'));
      this.priority_select.addItem(new goog.ui.MenuItem('Static'));
      this.priority_select.setSelectedIndex(
        priority['current_mode'] == 'inherit' ? 0 : 1);
      this.prioritySelectChanged_();

      var td = goog.dom.createElement('td');
      this.priority_select.render(td);
      this.dom_.appendChild(td, this.priority_input);
      this.dom_.appendChild(tr, td);
    } else if (priority['priority_capability'] == 'static') {
      // this port only supports Static priorities
      this.dom_.appendChild(tr, this.priority_input);
    }
  }
  this.setElementInternal(tr);
};


/**
 * Setup the event handlers
 */
ola.Port.prototype.enterDocument = function() {
  ola.Port.superClass_.enterDocument.call(this);

  if (this.priority_select != undefined) {
    goog.events.listen(
        this.priority_select,
        goog.ui.Component.EventType.ACTION,
        this.prioritySelectChanged_,
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

  goog.events.listen(this.getElement(),
                     goog.events.EventType.CLICK,
                     function() { this.checkbox.toggle(); },
                     false, this);
};


/**
 * Clean up this object.
 */
ola.Port.prototype.exitDocument = function() {
  ola.AvailablePort.superClass_.exitDocument.call(this);

  this.checkbox.exitDocument();
  if (this.priority_input) {
    goog.events.removeAll(this.priority_input);
  }
  if (this.priority_select) {
    goog.events.removeAll(this.priority_select.getElement());
    goog.events.removeAll(this.priority_select);
    this.priority_select.exitDocument();
  }
  goog.events.removeAll(this.getElement());
};


/**
 * Dispose of this object.
 */
ola.Port.prototype.dispose = function() {
  if (!this.getDisposed()) {
    ola.Port.superClass_.dispose.call(this);
    this.checkbox.dispose();
    this.checkbox = undefined;
    if (this.priority_select) {
      this.priority_select.dispose();
      this.priority_select = undefined;
    }
    this.priority_input = undefined;
  }
};


/**
 * Get the port id for this item
 * @return {string} the id of this port.
 */
ola.Port.prototype.portId = function() {
  return this.data['id'];
};


/**
 * Check is this row was selected
 * @return {boolean} true if selected, false otherwise.
 */
ola.Port.prototype.isSelected = function() {
  return this.checkbox.isChecked();
};


/**
 * Get the priority value for this port
 * @return {number|undefined} the priority value or undefined if this port
 * doesn't support priorities.
 */
ola.Port.prototype.priority = function() {
  var priority_capability = this.data['priority']['priority_capability'];
  if (priority_capability != undefined) {
    return this.priority_input.value;
  } else {
    return undefined;
  }
};


/**
 * Get the priority mode for this port
 * @return {string|undefined} the priority mode (inherit|static) or undefined
 *   if this port doesn't support priority modes.
 */
ola.Port.prototype.priorityMode = function() {
  var priority_capability = this.data['priority']['priority_capability'];
  if (priority_capability == 'full') {
    if (this.priority_select.getValue() == 'Inherit') {
      return 'inherit';
    } else {
      return 'static';
    }
  } else if (priority_capability == 'static') {
    return 'static';
  } else {
    return undefined;
  }
};


/**
 * Called when the port priority changes
 * @param {Object} e the event object.
 * @private
 */
ola.Port.prototype.prioritySelectChanged_ = function(e) {
  var item = this.priority_select.getSelectedItem();
  if (item.getCaption() == 'Static') {
    // static mode
    this.priority_input.style.visibility = 'visible';
  } else {
    // inherit mode
    this.priority_input.style.visibility = 'hidden';

  }
};


/**
 * An available port table component.
 * @constructor
 * @param {goog.dom.DomHelper=} opt_domHelper An optional DOM helper.
 */
ola.PortTable = function(opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
};
goog.inherits(ola.PortTable, goog.ui.Component);


/**
 * Create the dom for the PortTable.
 */
ola.PortTable.prototype.createDom = function() {
  this.decorateInternal(this.dom_.createElement('tbody'));
};


/**
 * Decorate an existing element
 * @param {Element} element the dom element to decorate.
 */
ola.PortTable.prototype.decorateInternal = function(element) {
  ola.PortTable.superClass_.decorateInternal.call(this, element);
};


/**
 * Check if we can decorate an element.
 * @param {Element} element the dom element to check.
 * @return {boolean} true if this element can be decorated, false otherwise.
 */
ola.PortTable.prototype.canDecorate = function(element) {
  return element.tagName == 'TBODY';
};


/**
 * Clear all rows from this table
 */
ola.PortTable.prototype.removeAllRows = function() {
  while (this.getChildCount()) {
    var row = this.removeChildAt(0, true);
    row.dispose();
  }
};


/**
 * Update the list of available ports
 * @param {Array.<Object>} ports the new list of ports.
 */
ola.PortTable.prototype.update = function(ports) {
  this.removeAllRows();
  var port_length = ports.length;
  for (var i = 0; i < port_length; ++i) {
    var component = new ola.Port(ports[i]);
    this.addChild(component, true);
  }
};
