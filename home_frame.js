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
goog.require('goog.ui.Dialog');
goog.require('ola.BaseFrame');
goog.require('ola.LoggerWindow');
goog.require('ola.Server');
goog.require('ola.Server.EventType');
goog.require('ola.SortedList');

goog.provide('ola.HomeFrame');

var ola = ola || {}


/**
 * A class representing the home frame
 * @param element_id the id of the div to use for the home frame
 * @constructor
 */
ola.HomeFrame = function(element_id, ola_server) {
  ola.BaseFrame.call(this, element_id);
  var reload_button = goog.dom.$('reload_button');
  goog.ui.decorate(reload_button);

  var stop_button = goog.dom.$('stop_button');
  goog.ui.decorate(stop_button);
  goog.events.listen(stop_button,
                     goog.events.EventType.CLICK,
                     this._StopButtonClicked,
                     false, this);

  var new_universe_button = goog.dom.$('new_universe_button');
  goog.ui.decorate(new_universe_button);

  goog.events.listen(reload_button,
                     goog.events.EventType.CLICK,
                     ola_server.FetchUniversePluginList,
                     false,
                     ola_server);

  goog.events.listen(ola_server, ola.Server.EventType.SERVER_INFO_EVENT,
                     this._UpdateFromData,
                     false, this);
  goog.events.listen(ola_server, ola.Server.EventType.UNIVERSE_LIST_EVENT,
                     this._UniverseListChanged,
                     false, this);
  ola_server.UpdateServerInfo();

  this.dialog = new goog.ui.Dialog(null, true);

  /*
  this.universe_list = new ola.SortedList(
      'universe_list',
      function(item) {
        var new_tr = goog.dom.createDom(
            'tr', {},
            goog.dom.createDom('td', {}, item.id.toString()),
            goog.dom.createDom('td', {}, item.name),
            goog.dom.createDom('td', {}, item.input_ports.toString()),
            goog.dom.createDom('td', {}, item.output_ports.toString()),
            goog.dom.createDom('td', {}, item.rdm_devices.toString()));
        return new_tr;
      },
      function(element, data) {
        var td = goog.dom.getFirstElementChild(element);
        td = goog.dom.getNextElementSibling(td);
        td.innerHTML = data.name;
        td = goog.dom.getNextElementSibling(td);
        td.innerHTML = data.input_ports.toString();
        td = goog.dom.getNextElementSibling(td);
        td.innerHTML = data.output_ports.toString();
        td = goog.dom.getNextElementSibling(td);
        td.innerHTML = data.rdm_devices.toString();
      });
  */
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
  //this.universe_list.UpdateFromData(e.universes);
}


/**
 * Called when the stop button is clicked
 */
ola.HomeFrame.prototype._StopButtonClicked = function(e) {
  this.dialog.setTitle('Please confirm');
  this.dialog.setButtonSet(goog.ui.Dialog.ButtonSet.YES_NO);
  this.dialog.setContent('Are you sure? OLA may not be configured to restart '
                         + 'automatically');
  this.dialog.setVisible(true);
}
