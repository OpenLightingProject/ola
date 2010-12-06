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
 * The RDM patcher tab.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.ui.AnimatedZippy');
goog.require('goog.events');

goog.require('ola.AvailablePort');
goog.require('ola.AvailablePortTable');
goog.require('ola.Dialog');
goog.require('ola.Port');
goog.require('ola.PortTable');
goog.require('ola.common.BaseUniverseTab');
goog.require('ola.common.Server');

goog.provide('ola.UniverseSettingsTab');


/**
 * The class that manages the RDMPatcher.
 * @constructor
 */
ola.UniverseSettingsTab = function(element, on_remove) {
  ola.common.BaseUniverseTab.call(this, element);
  this.on_remove = on_remove;
  this.server = ola.common.Server.getInstance();

  var save_button = goog.dom.$('universe_save_button');
  goog.ui.decorate(save_button);
  goog.events.listen(save_button,
                     goog.events.EventType.CLICK,
                     function() { this._saveButtonClicked(false); },
                     false,
                     this);

  this.merge_mode = goog.ui.decorate(goog.dom.$('universe_merge_mode'));

  this.input_table = new ola.PortTable();
  this.input_table.decorate(goog.dom.$('input_ports'));

  this.output_table = new ola.PortTable();
  this.output_table.decorate(goog.dom.$('output_ports'));

  var z1 = new goog.ui.AnimatedZippy('additional_ports_expander',
                                     'additional_ports');

  this.available_ports = new ola.AvailablePortTable();
  this.available_ports.decorate(goog.dom.$('universe_available_ports'));

  // setup notifications when the universe list changes
  goog.events.listen(this.server,
                     ola.common.Server.EventType.UNIVERSE_EVENT,
                     this._updateFromData,
                     false, this);
};
goog.inherits(ola.UniverseSettingsTab, ola.common.BaseUniverseTab);


/**
 * Set the universe for the patcher
 */
ola.UniverseSettingsTab.prototype.setUniverse = function(universe_id) {
  ola.UniverseSettingsTab.superClass_.setUniverse.call(this, universe_id);
};


/**
 * Called when the view port size changes
 */
ola.UniverseSettingsTab.prototype.sizeChanged = function(frame_size) {
  ola.UniverseSettingsTab.superClass_.sizeChanged.call(this, frame_size);
};


/**
 * Controls if we should do all the work involved in rendering the patcher.
 * This isn't needed if the patcher isn't visible.
 */
ola.UniverseSettingsTab.prototype.setActive = function(state) {
  ola.UniverseSettingsTab.superClass_.setActive.call(this, state);

  if (this.isActive())
    this._updateView();
};


/**
 * Fetch the latest data from the server to update the view
 */
ola.UniverseSettingsTab.prototype._updateView = function() {
  this.server.FetchUniverseInfo(this.getUniverse());
  this.available_ports.update(this.getUniverse());
};


/**
 * Update the tab from a Universe object
 */
ola.UniverseSettingsTab.prototype._updateFromData = function(e) {
  if (this.getUniverse() != e.universe['id']) {
    ola.logger.info('Mismatched universe, expected ' + this.getUniverse() +
        ', got ' + e.universe['id']);
    return;
  }

  goog.dom.$('universe_id').innerHTML = e.universe['id'];
  goog.dom.$('universe_name').value = e.universe['name'];
  if (e.universe['merge_mode'] == 'HTP') {
    this.merge_mode.setSelectedIndex(0);
  } else {
    this.merge_mode.setSelectedIndex(1);
  }

  this.input_table.update(e.universe['input_ports']);
  this.output_table.update(e.universe['output_ports']);
};



/**
 * Create a priority setting object from a port component
 * @param {Object} port_component the port component to generate the setting
 *   from.
 * @param {Array.<Object>} setting_list the list to add the setting to
 * @private
 */
ola.UniverseSettingsTab.prototype._generatePrioritySettingFromComponent = function(
    port_component, setting_list) {
  var priority = port_component.priority();
  if (priority != undefined) {
    var priority_setting = new Object();
    priority_setting.id = port_component.portId();
    priority_setting.priority = priority;
    var priority_mode = port_component.priorityMode();
    if (priority_mode != undefined) {
      priority_setting.mode = (priority_mode == 'Inherit' ? 0 : 1);
    }
    setting_list.push(priority_setting);
  }
};



/**
 * Called when the save button is clicked
 * @private
 */
ola.UniverseSettingsTab.prototype._saveButtonClicked = function(remove_confirmed) {
  var dialog = ola.Dialog.getInstance();

  var port_priorities = new Array();

  var remove_ports = new Array();
  var one_port_selected = false;
  var count = this.input_table.getChildCount();
  for (var i = 0; i < count; ++i) {
    var port = this.input_table.getChildAt(i);
    if (port.isSelected()) {
      one_port_selected = true;
      this._generatePrioritySettingFromComponent(port, port_priorities);
    } else {
      remove_ports.push(port.portId());
    }
  }
  var count = this.output_table.getChildCount();
  for (var i = 0; i < count; ++i) {
    var port = this.output_table.getChildAt(i);
    if (port.isSelected()) {
      one_port_selected = true;
      this._generatePrioritySettingFromComponent(port, port_priorities);
    } else {
      remove_ports.push(port.portId());
    }
  }

  // figure out the new ports to add
  var new_ports = this.available_ports.getSelectedRows();

  // if we're deleting the universe ask for confirmation
  if ((!one_port_selected) && new_ports.length == 0) {
    if (remove_confirmed) {
      this.was_removed = true;
    } else {
      goog.events.listen(
          dialog,
          goog.ui.Dialog.EventType.SELECT,
          this._removeConfirmed,
          false,
          this);
      dialog.setTitle('Confirm Universe Removal');
      dialog.setButtonSet(goog.ui.Dialog.ButtonSet.YES_NO);
      dialog.setContent(
          'Removing all ports will cause this universe to be deleted. Is ' +
          'this ok?');
      dialog.setVisible(true);
      return;
    }
  } else {
    this.was_removed = false;
  }

  var name = goog.dom.$('universe_name').value;

  if (name == '') {
    dialog.setTitle('Empty Universe Name');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent('The universe name cannot be empty');
    dialog.setVisible(true);
    return;
  }

  var tab = this;
  this.server.modifyUniverse(
      this.getUniverse(),
      name,
      this.merge_mode.getValue(),
      port_priorities,
      remove_ports,
      new_ports,
      function(e) { tab._saveCompleted(e); });

  var dialog = ola.Dialog.getInstance();
  dialog.setAsBusy();
  dialog.setVisible(true);
};


/**
 * Called when the universe removal is confirmed
 */
ola.UniverseSettingsTab.prototype._removeConfirmed = function(e) {
  var dialog = ola.Dialog.getInstance();
  goog.events.unlisten(
      dialog,
      goog.ui.Dialog.EventType.SELECT,
      this._removeConfirmed,
      false,
      this);
  if (e.key == goog.ui.Dialog.DefaultButtonKeys.YES) {
    dialog.setVisible(false);
    this._saveButtonClicked(true);
  }
}


/**
 * Called when the changes are saved
 * @private
 */
ola.UniverseSettingsTab.prototype._saveCompleted = function(e) {
  var dialog = ola.Dialog.getInstance();
  if (e.target.getStatus() == 200) {
    dialog.setVisible(false);
    if (this.was_removed && this.on_remove) {
      this.on_remove();
    }
    this._updateView()
  } else {
    dialog.setTitle('Failed to Save Settings');
    dialog.setContent(e.target.getLastUri() + ' : ' + e.target.getLastError());
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setVisible(true);
  }
};
