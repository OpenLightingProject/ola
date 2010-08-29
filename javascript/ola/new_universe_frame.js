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
goog.require('ola.BaseFrame');
goog.require('ola.LoggerWindow');
goog.require('ola.Server');
goog.require('ola.Server.EventType');
goog.require('ola.SortedList');

goog.provide('ola.NewUniverseFrame');

var ola = ola || {};


/**
 * The class representing the Universe frame
 * @constructor
 */
ola.NewUniverseFrame = function(element_id, ola_ui) {
  ola.BaseFrame.call(this, element_id);
  this.ola_ui = ola_ui;

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
                     this._addUniverseButtonClicked,
                     false, this);

  this.table_container = new ola.TableContainer();
  this.table_container.decorate(goog.dom.$('available_ports'));
  this.port_list = new ola.SortedList(
      this.table_container,
      new ola.AvailablePortComponentFactory());
};
goog.inherits(ola.NewUniverseFrame, ola.BaseFrame);


/**
 * Show this frame. We extend the base method so we can populate the ports.
 */
ola.NewUniverseFrame.prototype.Show = function() {
  var ola_server = ola.Server.getInstance();
  goog.events.listen(ola_server, ola.Server.EventType.AVAILBLE_PORTS_EVENT,
                     this._updateAvailablePorts,
                     false, this);
  ola_server.FetchAvailablePorts();

  ola.UniverseFrame.superClass_.Show.call(this);
};


/**
 * Called when the available ports are updated
 * @private
 */
ola.NewUniverseFrame.prototype._updateAvailablePorts = function(e) {
  this.port_list.UpdateFromData(e.ports);
  goog.events.unlisten(
      ola.Server.getInstance(),
      ola.Server.EventType.AVAILBLE_PORTS_EVENT,
      this._updateAvailablePorts,
      false, this);
};


/**
 * Called when the add universe button is clicked
 * @private
 */
ola.NewUniverseFrame.prototype._addUniverseButtonClicked = function(e) {
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
      selected_ports.push(port_component.PortId());
    }
  }

  if (selected_ports.length == 0) {
    dialog.setTitle('No ports selected');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent('At least one port must be bound to the universe');
    dialog.setVisible(true);
    return;
  }

  var frame = this;
  ola_server.createUniverse(
      universe_id,
      universe_name,
      selected_ports,
      function(e) { frame._newUniverseComplete(e); });
  dialog.SetAsBusy();
  dialog.setVisible(true);
};


/**
 * Called when the new universe action completes.
 * @private
 */
ola.NewUniverseFrame.prototype._newUniverseComplete = function(e) {
  var dialog = ola.Dialog.getInstance();
  var obj = e.target.getResponseJson();
  if (obj['ok']) {
    dialog.setVisible(false);
    this.ola_ui.ShowUniverse(obj['universe'], true);
  } else {
    dialog.setTitle('New Universe Failed');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent(obj['message']);
    dialog.setVisible(true);
  }
};
