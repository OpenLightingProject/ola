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
goog.require('goog.ui.SplitPane');
goog.require('goog.ui.TabPane');
goog.require('goog.ui.SplitPane.Orientation');

goog.require('ola.Dialog');
goog.require('ola.Server');
goog.require('ola.Server.EventType');
goog.require('ola.SortedList');

goog.provide('ola.UniverseFrame');

var ola = ola || {}


/**
 * The class representing the Universe frame
 * @constructor
 */
ola.UniverseFrame = function(element_id, ola_server) {
  ola.BaseFrame.call(this, element_id);
  this.ola_server = ola_server
  this.current_universe = undefined;
  goog.events.listen(ola_server, ola.Server.EventType.UNIVERSE_EVENT,
                     this._UpdateFromData,
                     false, this);

  var tabPane = new goog.ui.TabPane(
    document.getElementById(ola.UNIVERSE_TAB_PANE_ID));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_1'), "Settings"));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_2'), 'RDM'));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_3'), 'Console'));
  this.selected_tab = 0;
  tabPane.setSelectedIndex(1);

  goog.events.listen(tabPane, goog.ui.TabPane.Events.CHANGE,
                     this.TabChanged, false, this);

  this._SetupMainTab();
  this._SetupRDMTab();
  // this has to be done after the RDM split pane is setup otherwise the size
  // doesn't render correctly.
  tabPane.setSelectedIndex(0);
}

goog.inherits(ola.UniverseFrame, ola.BaseFrame);


/**
 * Setup the main universe settings tab
 */
ola.UniverseFrame.prototype._SetupMainTab = function() {

  var save_button = goog.dom.$('universe_save_button');
  goog.ui.decorate(save_button);
}

/**
 * Setup the RDM tab
 */
ola.UniverseFrame.prototype._SetupRDMTab = function() {
  var discovery_button = goog.dom.$('force_discovery_button');
  goog.ui.decorate(discovery_button);

  var lhs2 = new goog.ui.Component();
  var rhs2 = new goog.ui.Component();
  this.splitpane2 = new goog.ui.SplitPane(lhs2, rhs2,
      goog.ui.SplitPane.Orientation.HORIZONTAL);
  this.splitpane2.setInitialSize(150);
  this.splitpane2.setHandleSize(2);
  this.splitpane2.decorate(goog.dom.$('rdm_split_pane'));
  this.splitpane2.setSize(new goog.math.Size(500, 400));
}


/**
 * Get the current selected universe
 */
ola.UniverseFrame.prototype.ActiveUniverse = function() {
  return this.current_universe;
}


/**
 * Show this frame. We extend the base method so we can populate the correct
 * tab.
 */
ola.UniverseFrame.prototype.Show = function(universe_id) {
  this.current_universe = universe_id;
  this._UpdateSelectedTab();
  ola.UniverseFrame.superClass_.Show.call(this);
}


/**
 * Called when the select tab changes
 */
ola.UniverseFrame.prototype.TabChanged = function(e) {
  this.selected_tab = e.page.getIndex();
  this._UpdateSelectedTab();
}


ola.UniverseFrame.prototype._UpdateSelectedTab = function() {
  if (this.selected_tab == 0) {
    this.ola_server.FetchUniverseInfo(this.current_universe);
  } else if (this.selected_tab == 1) {
    // update RDM
  }
}


/**
 * Update this universe frame from a Universe object
 */
ola.UniverseFrame.prototype._UpdateFromData = function(e) {
  this.current_universe = e.universe.id;
  goog.dom.$('universe_id').innerHTML = e.universe.id;
  goog.dom.$('universe_name').innerHTML = e.universe.name;
  goog.dom.$('universe_merge_mode').innerHTML = e.universe.merge_mode;
}


/**
 * Update this universe frame from a Universe object
 */
ola.UniverseFrame.prototype._UpdateFromData = function(e) {
  this.current_universe = e.universe.id;
  goog.dom.$('universe_id').innerHTML = e.universe.id;
  goog.dom.$('universe_name').innerHTML = e.universe.name;
  goog.dom.$('universe_merge_mode').innerHTML = e.universe.merge_mode;
}
