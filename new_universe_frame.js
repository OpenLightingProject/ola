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
goog.require('goog.ui.Component');
goog.require('ola.BaseFrame');
goog.require('ola.LoggerWindow');
goog.require('ola.Server');
goog.require('ola.Server.EventType');
goog.require('ola.SortedList');
goog.require('goog.ui.Checkbox');

goog.provide('ola.NewUniverseFrame');

var ola = ola || {}


/**
 * A line in the active universe list.
 * @class
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
      this.data['is_output'] ?  'Output' : 'Input'));
  this.dom_.appendChild(tr, goog.dom.createDom('td', {},
      this.data['description']));
  this.setElementInternal(tr);

  goog.events.listen(tr,
                     goog.events.EventType.CLICK,
                     function () { this.checkbox.toggle(); },
                     false, this);
};


/**
 * Get the id of this item
 */
ola.AvailablePortComponent.prototype.Id = function() {
  return this.data['id'];
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
 * @class
 */
ola.AvailablePortComponentFactory = function() {
};


/**
 * @returns an instance of a AvailablePortComponent
 */
ola.AvailablePortComponentFactory.prototype.newComponent = function(data) {
  return new ola.AvailablePortComponent(data);
};


/**
 * The class representing the Universe frame
 * @constructor
 */
ola.NewUniverseFrame = function(element_id, ola_ui) {
  ola.BaseFrame.call(this, element_id);

  var cancel_button = goog.dom.$('cancel_new_universe_button');
  goog.ui.decorate(cancel_button);
  goog.events.listen(cancel_button,
                     goog.events.EventType.CLICK,
                     ola_ui.ShowHome,
                     false, ola_ui);

  var confirm_button = goog.dom.$('confirm_new_universe_button');
  goog.ui.decorate(confirm_button);
  goog.events.listen(confirm_button,
                     goog.events.EventType.CLICK,
                     this._AddButtonClicked,
                     false, this);

  this.table_container = new ola.TableContainer();
  this.table_container.decorate(goog.dom.$('available_ports'));
  this.port_list = new ola.SortedList(
      this.table_container,
      new ola.AvailablePortComponentFactory());
}
goog.inherits(ola.NewUniverseFrame, ola.BaseFrame);


/**
 * Show this frame. We extend the base method so we can populate the ports.
 */
ola.NewUniverseFrame.prototype.Show = function() {
  // get available ports
  var ola_server = ola.Server.getInstance();
  goog.events.listen(ola_server, ola.Server.EventType.AVAILBLE_PORTS_EVENT,
                     this._UpdateAvailablePorts,
                     false, this);
  ola_server.FetchAvailablePorts();

  ola.UniverseFrame.superClass_.Show.call(this);
}


/**
 * Called when the available ports are updated
 */
ola.NewUniverseFrame.prototype._UpdateAvailablePorts = function(e) {
  this.port_list.UpdateFromData(e.ports);
  goog.events.unlisten(
      ola.Server.getInstance(),
      ola.Server.EventType.AVAILBLE_PORTS_EVENT,
      this._UpdateAvailablePorts,
      false, this);
}


/**
 * Called when the add universe button is clicked
 */
ola.NewUniverseFrame.prototype._AddButtonClicked = function(e) {
  var dialog = ola.Dialog.getInstance();
  var universe_id_input = goog.dom.$('new_universe_id');
  var universe_id = parseInt(universe_id_input.value);
  if (isNaN(universe_id) || universe_id < 0 || universe_id > 65535) {
    dialog.setTitle('Invalid Universe Number');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent('The universe number must be between 0 and 65535');
    dialog.setVisible(true);
    return;
  }

  var ola_server = ola.Server.getInstance();
  // check if we already know about this universe
  if (ola_server.CheckIfUniverseExists(universe_id)) {
    dialog.setTitle('Universe already exists');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent('Universe ' + universe_id + ' already exists');
    dialog.setVisible(true);
    return;
  }

  // universe names are optional
  var universe_name = goog.dom.$('new_universe_name').value;

  var count = this.table_container.getChildCount();
  var selected_ports = new Array();
  for (var i = 0; i < count; ++i) {
    var port_component = this.table_container.getChildAt(i);
    if (port_component.IsSelected()) {
      selected_ports.push(port_component.Id());
    }
  }

  if (selected_ports.length == 0) {
    dialog.setTitle('No ports selected');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent('At least one port must be bound to the universe');
    dialog.setVisible(true);
    return;
  }

  //ola.server.NewUniverse(new_universe_id, new_universe_name, selected_ports);
  dialog.SetAsBusy();
  //dialog.setVisible(true);
}
