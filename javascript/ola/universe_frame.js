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
 * The universe frame.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.Timer');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.math');
goog.require('goog.ui.AnimatedZippy');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.SplitPane');
goog.require('goog.ui.SplitPane.Orientation');
goog.require('goog.ui.TabPane');
goog.require('goog.ui.Toolbar');
goog.require('goog.ui.ToolbarButton');
goog.require('goog.ui.ToolbarSeparator');

goog.require('ola.BaseFrame');
goog.require('ola.AvailablePort');
goog.require('ola.AvailablePortTable');
goog.require('ola.Dialog');
goog.require('ola.DmxConsole');
goog.require('ola.Port');
goog.require('ola.PortTable');
goog.require('ola.RDMAttributesPanel');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.SortedList');
goog.require('ola.common.UidControl');
goog.require('ola.common.UidControlFactory');

goog.provide('ola.UniverseFrame');


ola.UID_REFRESH_INTERVAL = 5000;


/**
 * The class representing the Universe frame
 * @param {string} element_id the id of the element to use for this frame.
 * @constructor
 */
ola.UniverseFrame = function(element_id, ola_ui) {
  ola.BaseFrame.call(this, element_id);
  this.ola_ui = ola_ui;
  this.current_universe = undefined;

  // setup the tab pane
  this.tabPane = new goog.ui.TabPane(goog.dom.$(element_id + '_tab_pane'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_1'), "Settings"));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_2'), 'RDM'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_3'), 'Console'));

  goog.events.listen(this.tabPane, goog.ui.TabPane.Events.CHANGE,
                     this._updateSelectedTab, false, this);

  this._setupMainTab();

  // We need to make the RDM pane visible here otherwise the splitter
  // doesn't render correctly.
  this.tabPane.setSelectedIndex(1);
  this._setupRDMTab();
  this.tabPane.setSelectedIndex(0);

  this._setupConsoleTab();

  // setup notifications when the universe or uid lists changes
  var ola_server = ola.common.Server.getInstance();
  goog.events.listen(ola_server, ola.common.Server.EventType.UNIVERSE_EVENT,
                     this._updateFromData,
                     false, this);
  goog.events.listen(ola_server, ola.common.Server.EventType.UIDS_EVENT,
                     this._updateUidList,
                     false, this);

  // setup the uid timer
  this.uid_timer = new goog.Timer(ola.UID_REFRESH_INTERVAL);
  goog.events.listen(
      this.uid_timer,
      goog.Timer.TICK,
      function() {
        if (this.current_universe != undefined) {
          ola.common.Server.getInstance().FetchUids(this.current_universe);
        }
      },
      false,
      this);

};
goog.inherits(ola.UniverseFrame, ola.BaseFrame);


/**
 * Setup the main universe settings tab
 * @private
 */
ola.UniverseFrame.prototype._setupMainTab = function() {
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
};


/**
 * Setup the RDM tab
 * @private
 */
ola.UniverseFrame.prototype._setupRDMTab = function() {
  var toolbar = new goog.ui.Toolbar();
  var discovery_button = new goog.ui.ToolbarButton(
      goog.dom.createDom('div', 'ola-icon ola-icon-discovery'));
  discovery_button.setTooltip('Force RDM Discovery for this universe');
  toolbar.addChild(discovery_button, true);
  toolbar.addChild(new goog.ui.ToolbarSeparator(), true);

  this.expander_button = new goog.ui.ToolbarButton('Show All Attributes');
  this.expander_button.setTooltip('');
  toolbar.addChild(this.expander_button, true);
  this.expander_button.setEnabled(false);

  toolbar.render(goog.dom.getElement('rdm_toolbar'));

  goog.events.listen(discovery_button,
                     goog.ui.Component.EventType.ACTION,
                     function() { this._discoveryButtonClicked(); },
                     false,
                     this);

  this.splitpane = new goog.ui.SplitPane(
      new goog.ui.Component(),
      new goog.ui.Component(),
      goog.ui.SplitPane.Orientation.HORIZONTAL);
  this.splitpane.setInitialSize(250);
  this.splitpane.setHandleSize(2);
  this.splitpane.decorate(goog.dom.$('rdm_split_pane'));

  var rdm_panel = new ola.RDMAttributesPanel('rdm_attributes');
  this.rdm_panel = rdm_panel;

  var frame = this;
  var uid_container = new goog.ui.Container();
  uid_container.decorate(goog.dom.$('uid_container'));
  this.uid_list = new ola.common.SortedList(
      uid_container,
      new ola.common.UidControlFactory(
        function (item) { rdm_panel.showUID(item); }));
};


/**
 * Setup the console tab.
 * @private
 */
ola.UniverseFrame.prototype._setupConsoleTab = function() {
  // setup the console
  this.dmx_console = new ola.DmxConsole();

  goog.events.listen(
      this.dmx_console,
      ola.DmxConsole.CHANGE_EVENT,
      this._consoleChanged,
      false, this);
};


/**
 * Get the current selected universe.
 * @return {number} the selected universe.
 */
ola.UniverseFrame.prototype.getActiveUniverse = function() {
  return this.current_universe;
};


/**
 * Set the size of the split pane to match the parent element
 */
ola.UniverseFrame.prototype.SetSplitPaneSize = function(e) {
  var big_frame = goog.dom.$('ola-splitpane-content');
  var big_size = goog.style.getBorderBoxSize(big_frame);
  var selected_tab = this.tabPane.getSelectedIndex();
  if (selected_tab == 0) {
    goog.style.setBorderBoxSize(
        goog.dom.$('tab_page_1'),
        new goog.math.Size(big_size.width - 7, big_size.height - 34));
  } else if (selected_tab == 1) {
    this.splitpane.setSize(
        new goog.math.Size(big_size.width - 7, big_size.height - 62));
  } else if (selected_tab == 2) {
    goog.style.setBorderBoxSize(
        goog.dom.$('tab_page_3'),
        new goog.math.Size(big_size.width - 7, big_size.height - 34));
  }
};


/**
 * Show this frame. We extend the base method so we can populate the correct
 * tab.
 * @param universe_id {number} the universe id to show
 * @param opt_select_main_tab {boolean} set to true to display the main tab
 */
ola.UniverseFrame.prototype.Show = function(universe_id, opt_select_main_tab) {
  if (this.current_universe != universe_id) {
    this.dmx_console.resetConsole();
    this.rdm_panel.updateUniverse(universe_id);
    this.rdm_panel.clear();
  }
  this.current_universe = universe_id;
  ola.UniverseFrame.superClass_.Show.call(this);
  if (opt_select_main_tab) {
    this.tabPane.setSelectedIndex(0);
  }
  this._updateSelectedTab();
};


/**
 * Hide this frame. We extend the base method so we can stop updating the UID
 * list.
 */
ola.UniverseFrame.prototype.Hide = function() {
  this.uid_timer.stop();
  ola.UniverseFrame.superClass_.Hide.call(this);
};


/**
 * Update the tab that was selected
 * @private
 */
ola.UniverseFrame.prototype._updateSelectedTab = function(e) {
  var selected_tab = this.tabPane.getSelectedIndex();
  if (!this.IsVisible()) {
    return;
  }

  var server = ola.common.Server.getInstance();
  this.uid_timer.stop();
  this.dmx_console.stopTimer();

  this.SetSplitPaneSize();

  if (selected_tab == 0) {
    server.FetchUniverseInfo(this.current_universe);
    this.available_ports.update(this.current_universe);
  } else if (selected_tab == 1) {
    // update RDM
    server.FetchUids(this.current_universe);
    this.uid_timer.start();
  } else if (selected_tab == 2) {
    this.dmx_console.setupIfRequired();
    this.dmx_console.update();
  this.dmx_console.startTimer();
  }
};


/**
 * Update this universe frame from a Universe object
 */
ola.UniverseFrame.prototype._updateFromData = function(e) {
  if (this.current_universe != e.universe['id']) {
    ola.logger.info('Mismatched universe, expected ' + this.current_universe +
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
 * Update the UID list
 * @private
 */
ola.UniverseFrame.prototype._updateUidList = function(e) {
  if (e.universe_id == this.current_universe) {
    var items = new Array();
    for (var i = 0; i < e.uids.length; ++i) {
      items.push(new ola.common.UidItem(e.uids[i]));
    }
    this.uid_list.updateFromData(items);
  } else {
    ola.logger.info('RDM universe mismatch, was ' + e.universe_id +
                    ', expected ' + this.current_universe);
  }
};


/**
 * Create a priority setting object from a port component
 * @param {Object} port_component the port component to generate the setting
 *   from.
 * @param {Array.<Object>} setting_list the list to add the setting to
 * @private
 */
ola.UniverseFrame.prototype._generatePrioritySettingFromComponent = function(
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
ola.UniverseFrame.prototype._saveButtonClicked = function(remove_confirmed) {
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

  var server = ola.common.Server.getInstance();
  var frame = this;
  server.modifyUniverse(
      this.current_universe,
      name,
      this.merge_mode.getValue(),
      port_priorities,
      remove_ports,
      new_ports,
      function(e) { frame._saveCompleted(e); });

  var dialog = ola.Dialog.getInstance();
  dialog.SetAsBusy();
  dialog.setVisible(true);
};


/**
 * Called when the universe removal is confirmed
 */
ola.UniverseFrame.prototype._removeConfirmed = function(e) {
  var dialog = ola.Dialog.getInstance();
  if (e.key == goog.ui.Dialog.DefaultButtonKeys.YES) {
    dialog.setVisible(false);
    this._saveButtonClicked(true);
  }
}


/**
 * Called when the changes are saved
 * @private
 */
ola.UniverseFrame.prototype._saveCompleted = function(e) {
  var dialog = ola.Dialog.getInstance();
  if (e.target.getStatus() == 200) {
    dialog.setVisible(false);
    if (this.was_removed) {
      this.ola_ui.ShowHome();
    }
    this._updateSelectedTab()
  } else {
    dialog.setTitle('Failed to Save Settings');
    dialog.setContent(e.target.getLastUri() + ' : ' + e.target.getLastError());
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setVisible(true);
  }
};


/**
 * Called when the discovery button is clicked.
 * @private
 */
ola.UniverseFrame.prototype._discoveryButtonClicked = function(e) {
  var server = ola.common.Server.getInstance();
  var frame = this;
  server.runRDMDiscovery(
      this.current_universe,
      function(e) { frame._discoveryComplete(e); });

  var dialog = ola.Dialog.getInstance();
  dialog.SetAsBusy();
  dialog.setVisible(true);
};


/**
 * Called when the discovery request returns. This doesn't actually mean that
 * the discovery process has completed, just that it's started.
 * @private
 */
ola.UniverseFrame.prototype._discoveryComplete = function(e) {
  var dialog = ola.Dialog.getInstance();
  dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
  if (e.target.getStatus() == 200) {
    dialog.setTitle('Discovery Process Started');
    dialog.setContent('The discovery process has begun.');
  } else {
    dialog.setTitle('Failed to Start Discovery Process');
    dialog.setContent(e.target.getLastUri() + ' : ' + e.target.getLastError());
  }
  dialog.setVisible(true);
};


/**
 * Called when the console values change
 */
ola.UniverseFrame.prototype._consoleChanged = function(e) {
  var data = this.dmx_console.getData();
  ola.common.Server.getInstance().setChannelValues(this.current_universe, data);
};
