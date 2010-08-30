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
 * The table that holds a list of available ports.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.Checkbox');
goog.require('goog.ui.Component');
goog.require('ola.LoggerWindow');
goog.require('ola.Server');

goog.provide('ola.AvailablePort');
goog.provide('ola.AvailablePortTable');


/**
 * A row in the available ports list.
 * @param {Object} data the data to build this row from.
 * @constructor
 */
ola.AvailablePort = function(data, opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
  this.data = data;
};
goog.inherits(ola.AvailablePort, goog.ui.Component);


/**
 * This component can't be used to decorate
 * @return {bool} false
 */
ola.AvailablePort.prototype.canDecorate = function() {
  return false;
};


/**
 * Create the dom for this component
 */
ola.AvailablePort.prototype.createDom = function() {
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
 * Get the port id for this item
 * @return {string} the id of this port
 */
ola.AvailablePort.prototype.portId = function() {
  return this.data['id'];
};


/**
 * Check is this row was selected
 * @return {boolean} true if selected, false otherwise.
 */
ola.AvailablePort.prototype.isSelected = function() {
  return this.checkbox.isChecked();
};


/**
 * An available port table component.
 * @constructor
 */
ola.AvailablePortTable = function(opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
};
goog.inherits(ola.AvailablePortTable, goog.ui.Component);


/**
 * Create the dom for the AvailablePortTable
 * @param {Element} element the dom element to decorate.
 */
ola.AvailablePortTable.prototype.createDom = function(container) {
  this.decorateInternal(this.dom_.createElement('tbody'));
};


/**
 * Decorate an existing element
 */
ola.AvailablePortTable.prototype.decorateInternal = function(element) {
  ola.AvailablePortTable.superClass_.decorateInternal.call(this, element);
};


/**
 * Check if we can decorate an element.
 * @param {Element} element the dom element to check
 * @return {boolean} true if this element can be decorated, false otherwise.
 */
ola.AvailablePortTable.prototype.canDecorate = function(element) {
  return element.tagName == 'TBODY';
};


/**
 * Returns the list of selected ports
 * @return {Array.<string>}
 */
ola.AvailablePortTable.prototype.getSelectedRows = function() {
  var selected_ports = new Array();
  var count = this.getChildCount();
  for (var i = 0; i < count; ++i) {
    var port = this.getChildAt(i);
    if (port.isSelected()) {
      selected_ports.push(port.portId());
    }
  }
  return selected_ports;
};


/**
 * Clear all rows from this table
 */
ola.AvailablePortTable.prototype.removeAllRows = function() {
  while (this.getChildCount()) {
    delete this.removeChildAt(0, true);
  }
};


/**
 * Update the list of available ports
 * @param {number} universe_id the id of the universe to fetch the available
 *   ports for.
 */
ola.AvailablePortTable.prototype.update = function(universe_id) {
  var table = this;
  ola.Server.getInstance().fetchAvailablePorts(
      universe_id,
      function(e) { table._updateCompleted(e); });
};


/**
 * Called when the list of available ports is returned
 */
ola.AvailablePortTable.prototype._updateCompleted = function(e) {
  if (e.target.getStatus() != 200) {
    ola.logger.info(e.target.getLastUri() + ' : ' + e.target.getLastError());
    return;
  }

  this.removeAllRows();
  var ports = e.target.getResponseJson();
  var port_length = ports.length;
  for (var i = 0; i < port_length; ++i) {
    var component = new ola.AvailablePort(ports[i]);
    this.addChild(component, true);
  }
};
