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
goog.require('ola.AvailablePort');
goog.require('ola.AvailablePortTable');
goog.require('ola.BaseFrame');
goog.require('ola.LoggerWindow');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');

goog.provide('ola.NewUniverseFrame');


/**
 * The class representing the Universe frame
 * @param {string} element_id the id of the element to use for this frame.
 * @constructor
 * @param {Object} ola_ui The OlaUI object.
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
                     this.addUniverseButtonClicked,
                     false, this);

  this.available_ports = new ola.AvailablePortTable();
  this.available_ports.decorate(goog.dom.$('available_ports'));
};
goog.inherits(ola.NewUniverseFrame, ola.BaseFrame);


/**
 * Show this frame. We extend the base method so we can populate the ports.
 */
ola.NewUniverseFrame.prototype.Show = function() {
  // clear out the fields
  goog.dom.$('new_universe_id').value = '';
  goog.dom.$('new_universe_name').value = '';
  this.available_ports.update();
  ola.UniverseFrame.superClass_.Show.call(this);
};


/**
 * Called when the add universe button is clicked
 * @param {Object} e The event object.
 */
ola.NewUniverseFrame.prototype.addUniverseButtonClicked = function(e) {
  var dialog = ola.Dialog.getInstance();
  var universe_id_input = goog.dom.$('new_universe_id');
  var universe_id = parseInt(universe_id_input.value);
  if (isNaN(universe_id) || universe_id < 0 || universe_id > 4294967295) {
    dialog.setTitle('Invalid Universe Number');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent('The universe number must be between 0 and 4294967295');
    dialog.setVisible(true);
    return;
  }

  var ola_server = ola.common.Server.getInstance();
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

  var selected_ports = this.available_ports.getSelectedRows();
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
      function(e) { frame.newUniverseComplete(e); });
  dialog.setAsBusy();
  dialog.setVisible(true);
};


/**
 * Called when the new universe action completes.
 * @param {Object} e The event object.
 */
ola.NewUniverseFrame.prototype.newUniverseComplete = function(e) {
  var dialog = ola.Dialog.getInstance();
  if (e.target.getStatus() != 200) {
    dialog.setTitle('New Universe Failed');
    dialog.setContent(e.target.getLastUri() + ' : ' + e.target.getLastError());
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setVisible(true);
    return;
  }
  var obj = e.target.getResponseJson();
  if (obj['ok']) {
    dialog.setVisible(false);
    this.ola_ui.ShowUniverse(obj['universe'], true);
    // update the universe list now
    ola.common.Server.getInstance().FetchUniversePluginList();
  } else {
    dialog.setTitle('New Universe Failed');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent(obj['message']);
    dialog.setVisible(true);
  }
};
