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
goog.require('goog.dom.ViewportSizeMonitor');
goog.require('goog.events');
goog.require('goog.math');
goog.require('goog.net.XhrIo');
goog.require('goog.net.XhrIoPool');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.CustomButton');
goog.require('goog.ui.Select');
goog.require('goog.ui.SplitPane');
goog.require('goog.ui.SplitPane.Orientation');
goog.require('goog.ui.TabPane');
goog.require('ola.Dialog');
goog.require('ola.Server');
goog.require('ola.Server.EventType');
goog.require('ola.SortedList');

goog.provide('ola.UniverseFrame');

var ola = ola || {}


ola.UID_REFRESH_INTERVAL = 5000;
ola.UNIVERSE_TAB_PANE_ID = 'universe_tab_pane';

/**
 * The class for an item in the uid list
 */
ola.UidControl = function(data, callback, opt_renderer, opt_domHelper) {
  goog.ui.Control.call(this, data['uid'], opt_renderer, opt_domHelper);
  this.id = data['id'];
  this.uid = data['uid'];
  this.callback = callback;
};
goog.inherits(ola.UidControl, goog.ui.Control);


/**
 * Update this item with from new data
 */
ola.UidControl.prototype.Id = function() {
  return this.id
}


/**
 * Setup the event handlers for this control
 */
ola.UidControl.prototype.enterDocument = function() {
  goog.ui.Control.superClass_.enterDocument.call(this);
  this.getElement().title = 'UID ' + this.uid;
  goog.events.listen(this.getElement(),
                     goog.events.EventType.CLICK,
                     function() { this.callback(this.id); },
                     false,
                     this);
};


/**
 * Update this item with from new data.
 */
ola.UidControl.prototype.Update = function(new_data) {
  this.setContent(new_data['uid']);
}


/**
 * The base class for a factory which produces control items
 * @class
 */
ola.UidControlFactory = function(callback) {
  this.callback = callback;
}


/**
 * @returns an instance of a UidControl
 */
ola.UidControlFactory.prototype.newComponent = function(data) {
  return new ola.UidControl(data, this.callback);
}


/**
 * The class representing the Universe frame
 * @constructor
 */
ola.UniverseFrame = function(element_id) {
  ola.BaseFrame.call(this, element_id);
  this.current_universe = undefined;

  var ola_server = ola.Server.getInstance();
  goog.events.listen(ola_server, ola.Server.EventType.UNIVERSE_EVENT,
                     this._UpdateFromData,
                     false, this);
  goog.events.listen(ola_server, ola.Server.EventType.UIDS_EVENT,
                     this._UpdateUids,
                     false, this);

  this.tabPane = new goog.ui.TabPane(goog.dom.$(ola.UNIVERSE_TAB_PANE_ID));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_1'), "Settings"));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_2'), 'RDM'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_3'), 'Console'));

  goog.events.listen(this.tabPane, goog.ui.TabPane.Events.CHANGE,
                     this._UpdateSelectedTab, false, this);

  this._SetupMainTab();

  // We need to make the RDM pane visible here otherwise the splitter
  // doesn't render correctly.
  this.tabPane.setSelectedIndex(1);
  this._SetupRDMTab();
  this.tabPane.setSelectedIndex(0);

  this.uid_timer = new goog.Timer(ola.UID_REFRESH_INTERVAL);
  goog.events.listen(this.uid_timer, goog.Timer.TICK,
                     function() { ola_server.FetchUids(); });
}
goog.inherits(ola.UniverseFrame, ola.BaseFrame);


/**
 * Setup the main universe settings tab
 */
ola.UniverseFrame.prototype._SetupMainTab = function() {
  var save_button = goog.dom.$('universe_save_button');
  goog.ui.decorate(save_button);
  goog.events.listen(save_button,
                     goog.events.EventType.CLICK,
                     function() { this._Save(); },
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
}


/**
 * Setup the RDM tab
 */
ola.UniverseFrame.prototype._SetupRDMTab = function() {
  var discovery_button = goog.dom.$('force_discovery_button');
  goog.ui.decorate(discovery_button);

  var lhs2 = new goog.ui.Component();
  var rhs2 = new goog.ui.Component();
  this.splitpane = new goog.ui.SplitPane(lhs2, rhs2,
      goog.ui.SplitPane.Orientation.HORIZONTAL);
  this.splitpane.setInitialSize(120);
  this.splitpane.setHandleSize(2);
  this.splitpane.decorate(goog.dom.$('rdm_split_pane'));
  this.splitpane.setSize(new goog.math.Size(500, 400));

  var frame = this;
  var uid_container = new goog.ui.Container();
  uid_container.decorate(goog.dom.$('uid_container'));
  this.uid_list = new ola.SortedList(
      uid_container,
      new ola.UidControlFactory(function (id) { frame._ShowUID(id); }));
}


/**
 * Get the current selected universe
 */
ola.UniverseFrame.prototype.ActiveUniverse = function() {
  return this.current_universe;
}


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
}


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
  this._UpdateSelectedTab();
}


/**
 * Update the tab that was selected
 */
ola.UniverseFrame.prototype._UpdateSelectedTab = function(e) {
  var selected_tab = this.tabPane.getSelectedIndex();
  if (!this.IsVisible()) {
    return;
  }

  var server = ola.Server.getInstance();
  this.uid_timer.stop();

  this.SetSplitPaneSize();

  if (selected_tab == 0) {
    server.FetchUniverseInfo(this.current_universe);

    var ola_server = ola.Server.getInstance();
    goog.events.listen(ola_server, ola.Server.EventType.AVAILBLE_PORTS_EVENT,
                       this._UpdateAvailablePorts,
                       false, this);
    ola_server.FetchAvailablePorts();
  } else if (selected_tab == 1) {
    // update RDM
    server.FetchUids(this.current_universe);
    this.uid_timer.start();
  }
}


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
}



/**
 * Called when the available ports are updated
 */
ola.UniverseFrame.prototype._UpdateAvailablePorts = function(e) {
  this.available_port_list.UpdateFromData(e.ports);
  goog.events.unlisten(
      ola.Server.getInstance(),
      ola.Server.EventType.AVAILBLE_PORTS_EVENT,
      this._UpdateAvailablePorts,
      false, this);
}


/**
 * Update the UID list
 */
ola.UniverseFrame.prototype._UpdateUids = function(e) {
  if (e.universe_id == this.current_universe) {
    this.uid_list.UpdateFromData(e.uids);
  } else {
    ola.logger.info('RDM universe mismatch, was ' + e.universe_id +
                    ', expected ' + this.current_universe);
  }
}


/**
 * Show information for a particular UID
 * @param id
 */
ola.UniverseFrame.prototype._ShowUID = function(id) {
}


/**
 * Called when the save button is clicked
 */
ola.UniverseFrame.prototype._Save = function(e) {

  // see if we need to remove any ports
  var remove_ports = new Array();
  var count = this.input_table_container.getChildCount();
  for (var i = 0; i < count; ++i) {
    var port_component = this.input_table_container.getChildAt(i);
    if (!port_component.IsSelected()) {
      remove_ports.push(port_component.PortId());
    }
  }
  var count = this.output_table_container.getChildCount();
  for (var i = 0; i < count; ++i) {
    var port_component = this.output_table_container.getChildAt(i);
    if (!port_component.IsSelected()) {
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
  //server.modifyUniverse(  );
}


/**
 * Called when the changes are saved
 */
ola.UniverseFrame.prototype._saveCompleted = function(e) {

}
