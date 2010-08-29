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
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.SplitPane');
goog.require('goog.ui.SplitPane.Orientation');
goog.require('goog.ui.TabPane');
goog.require('ola.Dialog');
goog.require('ola.Server');
goog.require('ola.Server.EventType');
goog.require('ola.SortedList');
goog.require('ola.UidControl');
goog.require('ola.UidControlFactory');

goog.provide('ola.UniverseFrame');

var ola = ola || {}


ola.UID_REFRESH_INTERVAL = 5000;


/**
 * The class representing the Universe frame
 * @param {string} element_id the id of the element to use for this frame.
 * @constructor
 */
ola.UniverseFrame = function(element_id) {
  ola.BaseFrame.call(this, element_id);
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

  // setup notifications when the universe or uid lists changes
  var ola_server = ola.Server.getInstance();
  goog.events.listen(ola_server, ola.Server.EventType.UNIVERSE_EVENT,
                     this._UpdateFromData,
                     false, this);
  goog.events.listen(ola_server, ola.Server.EventType.UIDS_EVENT,
                     this._updateUidList,
                     false, this);

  // setup the uid timer
  this.uid_timer = new goog.Timer(ola.UID_REFRESH_INTERVAL);
  goog.events.listen(this.uid_timer, goog.Timer.TICK,
                     function() { ola_server.FetchUids(); });

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
                     function() { this._saveButtonClicked(); },
                     false,
                     this);

  this.merge_mode = goog.ui.decorate(goog.dom.$('universe_merge_mode'));

  this.input_table_container = new ola.TableContainer();
  this.input_table_container.decorate(goog.dom.$('input_ports'));
  this.input_port_list = new ola.SortedList(
      this.input_table_container,
      new ola.PortComponentFactory());

  this.output_table_container = new ola.TableContainer();
  this.output_table_container.decorate(goog.dom.$('output_ports'));
  this.output_port_list = new ola.SortedList(
      this.output_table_container,
      new ola.PortComponentFactory());

  this.available_table_container = new ola.TableContainer();
  this.available_table_container.decorate(
      goog.dom.$('universe_available_ports'));
  this.available_port_list = new ola.SortedList(
      this.available_table_container,
      new ola.AvailablePortComponentFactory());
};


/**
 * Setup the RDM tab
 * @private
 */
ola.UniverseFrame.prototype._setupRDMTab = function() {
  var discovery_button = goog.dom.$('force_discovery_button');
  goog.ui.decorate(discovery_button);
  goog.events.listen(discovery_button,
                     goog.events.EventType.CLICK,
                     function() { this._discoveryButtonClicked(); },
                     false,
                     this);

  this.splitpane = new goog.ui.SplitPane(
      new goog.ui.Component(),
      new goog.ui.Component(),
      goog.ui.SplitPane.Orientation.HORIZONTAL);
  this.splitpane.setInitialSize(120);
  this.splitpane.setHandleSize(2);
  this.splitpane.decorate(goog.dom.$('rdm_split_pane'));

  var frame = this;
  var uid_container = new goog.ui.Container();
  uid_container.decorate(goog.dom.$('uid_container'));
  this.uid_list = new ola.SortedList(
      uid_container,
      new ola.UidControlFactory(function (id) { frame._ShowUID(id); }));
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
  if (this.tabPane.getSelectedIndex() == 0) {
    goog.style.setBorderBoxSize(
        goog.dom.$('tab_page_1'),
        new goog.math.Size(big_size.width - 7, big_size.height - 34));
  } else if (this.tabPane.getSelectedIndex() == 1) {
    this.splitpane.setSize(
        new goog.math.Size(big_size.width - 7, big_size.height - 62));
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
    this.uid_list.Clear();
    this.input_port_list.Clear();
    this.output_port_list.Clear();
    this.available_port_list.Clear();
  }
  this.current_universe = universe_id;
  ola.UniverseFrame.superClass_.Show.call(this);
  if (opt_select_main_tab) {
    this.tabPane.setSelectedIndex(0);
  }
  this._updateSelectedTab();
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

  var server = ola.Server.getInstance();
  this.uid_timer.stop();

  this.SetSplitPaneSize();

  if (selected_tab == 0) {
    server.FetchUniverseInfo(this.current_universe);

    goog.events.listen(server, ola.Server.EventType.AVAILBLE_PORTS_EVENT,
                       this._updateAvailablePorts,
                       false, this);
    server.FetchAvailablePorts();
  } else if (selected_tab == 1) {
    // update RDM
    server.FetchUids(this.current_universe);
    this.uid_timer.start();
  }
};


/**
 * Update this universe frame from a Universe object
 */
ola.UniverseFrame.prototype._UpdateFromData = function(e) {
  if (this.current_universe != e.universe['id']) {
    ola.logger.info('Mismatched universe, expected ' + this.current_universe +
        ', got ' + e.universe['id']);
    return;
  }

  this.current_universe = e.universe['id'];
  goog.dom.$('universe_id').innerHTML = e.universe['id'];
  goog.dom.$('universe_name').value = e.universe['name'];

  if (e.universe['merge_mode'] == 'HTP') {
    this.merge_mode.setSelectedIndex(0);
  } else {
    this.merge_mode.setSelectedIndex(1);
  }

  this.input_port_list.UpdateFromData(e.universe['input_ports']);
  this.output_port_list.UpdateFromData(e.universe['output_ports']);
};


/**
 * Called when the available ports are updated
 * @private
 */
ola.UniverseFrame.prototype._updateAvailablePorts = function(e) {
  this.available_port_list.UpdateFromData(e.ports);
  goog.events.unlisten(
      ola.Server.getInstance(),
      ola.Server.EventType.AVAILBLE_PORTS_EVENT,
      this._updateAvailablePorts,
      false, this);
};


/**
 * Update the UID list
 * @private
 */
ola.UniverseFrame.prototype._updateUidList = function(e) {
  if (e.universe_id == this.current_universe) {
    this.uid_list.UpdateFromData(e.uids);
  } else {
    ola.logger.info('RDM universe mismatch, was ' + e.universe_id +
                    ', expected ' + this.current_universe);
  }
};


/**
 * Show information for a particular UID
 * @param id {number} the UID represented as a float
 * @private
 */
ola.UniverseFrame.prototype._ShowUID = function(id) {
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
    priority_setting.id = port_component.PortId();
    priority_setting.priority = priority;
    var priority_mode = port_component.priorityMode();
    if (priority_mode != undefined) {
      priority_setting.mode = priority_mode;
    }
    setting_list.push(priority_setting);
  }
};


/**
 * Called when the save button is clicked
 * @private
 */
ola.UniverseFrame.prototype._saveButtonClicked = function(e) {
  var port_priorities = new Array();

  var remove_ports = new Array();
  var count = this.input_table_container.getChildCount();
  for (var i = 0; i < count; ++i) {
    var port_component = this.input_table_container.getChildAt(i);
    if (port_component.IsSelected()) {
      this._generatePrioritySettingFromComponent(port_component,
                                                 port_priorities);
    } else {
      remove_ports.push(port_component.PortId());
    }
  }
  var count = this.output_table_container.getChildCount();
  for (var i = 0; i < count; ++i) {
    var port_component = this.output_table_container.getChildAt(i);
    if (port_component.IsSelected()) {
      this._generatePrioritySettingFromComponent(port_component,
                                                 port_priorities);
    } else {
      remove_ports.push(port_component.PortId());
    }
  }

  // figure out the new ports to add
  count = this.available_table_container.getChildCount();
  var new_ports = new Array();
  for (var i = 0; i < count; ++i) {
    var port_component = this.available_table_container.getChildAt(i);
    if (port_component.IsSelected()) {
      new_ports.push(port_component.PortId());
    }
  }


  var server = ola.Server.getInstance();
  var frame = this;
  server.modifyUniverse(
      this.current_universe,
      goog.dom.$('universe_name').value,
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
 * Called when the changes are saved
 * @private
 */
ola.UniverseFrame.prototype._saveCompleted = function(e) {
  var dialog = ola.Dialog.getInstance();
  if (e.target.getStatus() == 200) {
    dialog.setVisible(false);
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
  var server = ola.Server.getInstance();
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
