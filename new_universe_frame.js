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
  var tr = this.dom_.createDom(
      'tr', {},
      goog.dom.createDom('td', {}, ''),
      goog.dom.createDom('td', {}, this.data['device']),
      goog.dom.createDom('td', {}, this.data['is_output'] ? 'Output' :
        'Input'),
      goog.dom.createDom('td', {}, this.data['description']));
  this.setElementInternal(tr);
};


/**
 * Get the id of this item
 */
ola.AvailablePortComponent.prototype.Id = function() {
  return this.data['id'];
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

  var table_container = new ola.TableContainer();
  table_container.decorate(goog.dom.$('available_ports'));
  this.port_list = new ola.SortedList(
      table_container,
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
 * Called when the stop button is clicked
 */
ola.NewUniverseFrame.prototype._AddButtonClicked = function(e) {
  // verify all fields here

  var dialog = ola.Dialog.getInstance();
  dialog.SetAsBusy();
  dialog.setVisible(true);
}
