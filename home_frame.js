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
 * The home frame.
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
goog.require('ola.Dialog');

goog.provide('ola.HomeFrame');

var ola = ola || {}


/**
 * A line in the active universe list.
 * @class
 */
ola.UniverseComponent = function(data, opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
  this.data = data;
};
goog.inherits(ola.UniverseComponent, goog.ui.Component);


/**
 * This component can't be used to decorate
 */
ola.UniverseComponent.prototype.canDecorate = function() {
  return false;
};


/**
 * Create the dom for this component
 */
ola.UniverseComponent.prototype.createDom = function() {
  var tr = this.dom_.createDom(
      'tr', {},
      goog.dom.createDom('td', {}, this.data.id.toString()),
      goog.dom.createDom('td', {}, this.data.name),
      goog.dom.createDom('td', {}, this.data.input_ports.toString()),
      goog.dom.createDom('td', {}, this.data.output_ports.toString()),
      goog.dom.createDom('td', {}, this.data.rdm_devices.toString()));
  this.setElementInternal(tr);
};


/**
 * Get the id of this item
 */
ola.UniverseComponent.prototype.Id = function() {
  return this.data['id'];
};


/**
 * Update this item with from new data
 */
ola.UniverseComponent.prototype.Update = function(new_data) {
  var element = this.getElement();
  var td = goog.dom.getFirstElementChild(element);
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['name'];
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['input_ports'].toString();
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['output_ports'].toString();
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['rdm_devices'].toString();
};


/**
 * The base class for a factory which produces UniverseComponents
 * @class
 */
ola.UniverseComponentFactory = function() {
};


/**
 * @returns an instance of a UniverseComponent
 */
ola.UniverseComponentFactory.prototype.newComponent = function(data) {
  return new ola.UniverseComponent(data);
};


/**
 * A class representing the home frame
 * @param element_id the id of the div to use for the home frame
 * @constructor
 */
ola.HomeFrame = function(element_id) {
  var ola_server = ola.Server.getInstance();
  ola.BaseFrame.call(this, element_id);
  var reload_button = goog.dom.$('reload_button');
  goog.ui.decorate(reload_button);
  goog.events.listen(reload_button,
                     goog.events.EventType.CLICK,
                     this._ReloadButtonClicked,
                     false, this);

  var stop_button = goog.dom.$('stop_button');
  goog.ui.decorate(stop_button);
  goog.events.listen(stop_button,
                     goog.events.EventType.CLICK,
                     this._StopButtonClicked,
                     false, this);

  var new_universe_button = goog.dom.$('new_universe_button');
  goog.ui.decorate(new_universe_button);

  goog.events.listen(ola_server, ola.Server.EventType.SERVER_INFO_EVENT,
                     this._UpdateFromData,
                     false, this);
  goog.events.listen(ola_server, ola.Server.EventType.UNIVERSE_LIST_EVENT,
                     this._UniverseListChanged,
                     false, this);
  goog.events.listen(ola_server, ola.Server.EventType.SERVER_STOP_EVENT,
                     this._StopServerComplete,
                     false, this);
  goog.events.listen(ola_server, ola.Server.EventType.PLUGIN_RELOAD_EVENT,
                     this._PluginReloadComplete,
                     false, this);
  ola_server.UpdateServerInfo();

  var table_container = new ola.TableContainer();
  table_container.decorate(goog.dom.$('active_universe_list'));
  this.universe_list = new ola.SortedList(
      table_container,
      new ola.UniverseComponentFactory());
}
goog.inherits(ola.HomeFrame, ola.BaseFrame);


/**
 * Update the home frame with new server data
 */
ola.HomeFrame.prototype._UpdateFromData = function(e) {
  goog.dom.$('server_hostname').innerHTML = e.server_info.hostname;
  goog.dom.$('server_ip').innerHTML = e.server_info.ip;
  goog.dom.$('server_version').innerHTML = e.server_info.version;
  goog.dom.$('server_uptime').innerHTML = e.server_info.up_since;
}


/**
 * Update the universe set
 */
ola.HomeFrame.prototype._UniverseListChanged = function(e) {
  this.universe_list.UpdateFromData(e.universes);
}


/**
 * Called when the stop button is clicked
 */
ola.HomeFrame.prototype._StopButtonClicked = function(e) {
  var dialog = ola.Dialog.getInstance();

  goog.events.listen(dialog, goog.ui.Dialog.EventType.SELECT,
      this._StopDialogSelected, false, this);

  dialog.setTitle('Please confirm');
  dialog.setButtonSet(goog.ui.Dialog.ButtonSet.YES_NO);
  dialog.setContent('Are you sure? OLA may not be configured to restart '
                    + 'automatically');
  dialog.setVisible(true);
}


/**
 * Called when the stop dialog exits.
 */
ola.HomeFrame.prototype._StopDialogSelected = function(e) {
  var dialog = ola.Dialog.getInstance();

  goog.events.unlisten(dialog, goog.ui.Dialog.EventType.SELECT,
      this._StopDialogSelected, false, this);

  if (e.key == goog.ui.Dialog.DefaultButtonKeys.YES) {
    dialog.SetAsBusy();
    dialog.setVisible(true);
    ola.Server.getInstance().StopServer();
    return false;
  }
}


/**
 * Update the home frame with new server data
 */
ola.HomeFrame.prototype._StopServerComplete = function(e) {
  ola.Dialog.getInstance().setVisible(false);
}


/**
 * Called when the reload button is clicked
 */
ola.HomeFrame.prototype._ReloadButtonClicked = function(e) {
  var dialog = ola.Dialog.getInstance();
  dialog.SetAsBusy();
  dialog.setVisible(true);
  ola.Server.getInstance().ReloadPlugins();
}


/**
 * Update the home frame with new server data
 */
ola.HomeFrame.prototype._PluginReloadComplete = function(e) {
  ola.Dialog.getInstance().setVisible(false);
}
