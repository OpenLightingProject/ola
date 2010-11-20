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
 * The main OLA UI.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.Timer');
goog.require('goog.dom');
goog.require('goog.dom.ViewportSizeMonitor');
goog.require('goog.events');
goog.require('goog.math');
goog.require('goog.ui.AnimatedZippy');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.SplitPane');
goog.require('goog.ui.SplitPane.Orientation');

goog.require('ola.common.GenericControl');
goog.require('ola.HomeFrame');
goog.require('ola.LoggerWindow');
goog.require('ola.NewUniverseFrame');
goog.require('ola.common.PluginControlFactory');
goog.require('ola.PluginFrame');
goog.require('ola.common.PluginItem');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.SortedList');
goog.require('ola.UniverseFrame');
goog.require('ola.UniverseItem');
goog.require('ola.UniverseControl');

goog.provide('ola.OlaUi');
goog.provide('ola.Setup');

var ola = ola || {};

ola.LIST_UPDATE_INTERVAL_MS = 5000;

ola.HOME_FRAME_ID = 'home_frame';
ola.UNIVERSE_FRAME_ID = 'universe_frame';
ola.PLUGIN_FRAME_ID = 'plugin_frame';
ola.SPLIT_PANE_ID = 'split_pane';
ola.NEW_UNIVERSE_FRAME_ID = 'new_universe_frame';


/**
 * Setup the OLA ola_ui widgets
 * @constructor
 */
ola.OlaUI = function() {
  this.logger_window = new ola.LoggerWindow();
  this.ola_server = ola.common.Server.getInstance();
  this.home_frame = new ola.HomeFrame(ola.HOME_FRAME_ID);
  this.universe_frame = new ola.UniverseFrame(ola.UNIVERSE_FRAME_ID, this);
  this.plugin_frame = new ola.PluginFrame(ola.PLUGIN_FRAME_ID, this.ola_server);
  this.new_universe_frame = new ola.NewUniverseFrame(ola.NEW_UNIVERSE_FRAME_ID,
                                                     this);

  goog.events.listen(goog.dom.$('new_universe_button'),
                     goog.events.EventType.CLICK,
                     this._ShowNewUniverse,
                     false,
                     this);

  // setup the main split pane
  var lhs = new goog.ui.Component();
  var rhs = new goog.ui.Component();
  this.splitpane1 = new goog.ui.SplitPane(lhs, rhs,
      goog.ui.SplitPane.Orientation.HORIZONTAL);
  this.splitpane1.setInitialSize(130);
  this.splitpane1.setHandleSize(2);
  this.splitpane1.decorate(goog.dom.$(ola.SPLIT_PANE_ID));

  // redraw on resize events
  this.vsm = new goog.dom.ViewportSizeMonitor();
  this._UpdateUI();
  goog.events.listen(this.vsm, goog.events.EventType.RESIZE, this._UpdateUI,
    false, this);

  this._SetupNavigation();
  this.ShowHome();

  // show the main frame now
  goog.dom.$(ola.SPLIT_PANE_ID).style.visibility = 'visible';
};


/**
 * Setup the navigation section of the UI
 * @private
 */
ola.OlaUI.prototype._SetupNavigation = function() {
  var home_control = goog.dom.$('home_control');
  goog.ui.decorate(home_control);
  goog.events.listen(home_control,
                     goog.events.EventType.CLICK,
                     this.ShowHome,
                     false,
                     this);

  var z1 = new goog.ui.AnimatedZippy('plugin_list_control',
                                     'plugin_container');
  var z2 = new goog.ui.AnimatedZippy('universe_list_control',
                                     'universe_container');

  // setup the plugin & universe lists
  var ui = this;
  var plugin_container = new goog.ui.Container();
  plugin_container.decorate(goog.dom.$('plugin_container'));
  this.plugin_list = new ola.common.SortedList(
      plugin_container,
      new ola.common.PluginControlFactory(
        function(item) { ui._ShowPlugin(item.id()); }));

  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.PLUGIN_LIST_EVENT,
                     this._updatePluginList,
                     false, this);

  var universe_container = new goog.ui.Container();
  universe_container.decorate(goog.dom.$('universe_container'));
  this.universe_list = new ola.common.SortedList(
      universe_container,
      new ola.UniverseControlFactory(
        function(item) { ui.ShowUniverse(item.id(), true); }));

  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.UNIVERSE_LIST_EVENT,
                     this._updateUniverseList,
                     false, this);

  this.timer = new goog.Timer(ola.LIST_UPDATE_INTERVAL_MS);
  goog.events.listen(this.timer, goog.Timer.TICK,
                     function() { this.FetchUniversePluginList(); },
                     false,
                     this.ola_server);
  this.ola_server.FetchUniversePluginList();
  this.timer.start();
};


/**
 * Update universe list.
 */
ola.OlaUI.prototype._updateUniverseList = function(e) {
  var items = new Array();
  ola.logger.info('Got ' + e.universes.length + ' universes');
  for (var i = 0; i < e.universes.length; ++i) {
    items.push(new ola.UniverseItem(e.universes[i]));
  }
  this.universe_list.updateFromData(items);
};


/**
 * Update the plugin list
 */
ola.OlaUI.prototype._updatePluginList = function(e) {
  var items = new Array();
  for (var i = 0; i < e.plugins.length; ++i) {
    var item = new ola.common.PluginItem(e.plugins[i]);
    items.push(item);
  }
  this.plugin_list.updateFromData(items);
}


/**
 * Display the home frame
 * @private
 */
ola.OlaUI.prototype.ShowHome = function() {
  this._HideAllFrames();
  this.home_frame.Show();
};


/**
 * Display the universe frame
 * @param universe_id {number} the ID of the universe to load in the frame.
 * @param opt_select_main_tab {boolean} set to true to display the main tab.
 */
ola.OlaUI.prototype.ShowUniverse = function(universe_id, opt_select_main_tab) {
  this._HideAllFrames();
  this.universe_frame.Show(universe_id, opt_select_main_tab);
};


/**
 * Display the new universe frame
 * @private
 */
ola.OlaUI.prototype._ShowNewUniverse = function() {
  this._HideAllFrames();
  this.new_universe_frame.Show();
};


/**
 * Display the plugin frame
 * @param plugin_id the ID of the plugin to show in the frame.
 * @private
 */
ola.OlaUI.prototype._ShowPlugin = function(plugin_id) {
  this.ola_server.FetchPluginInfo(plugin_id);
  this._HideAllFrames();
  this.plugin_frame.Show();
};


/**
 * Hide all the frames
 * @private
 */
ola.OlaUI.prototype._HideAllFrames = function() {
  this.home_frame.Hide();
  this.universe_frame.Hide();
  this.plugin_frame.Hide();
  this.new_universe_frame.Hide();
};


/**
 * Update the UI size. This is called when the window size changes
 * @private
 */
ola.OlaUI.prototype._UpdateUI = function(e) {
  var size = this.vsm.getSize();
  this.splitpane1.setSize(new goog.math.Size(size.width, size.height - 85));
  this.logger_window.SetSize(size);
  this.universe_frame.setSplitPaneSize();
};


ola.Setup = function() {
  var ola_ui = new ola.OlaUI();
};
goog.exportSymbol('ola.Setup', ola.Setup);
