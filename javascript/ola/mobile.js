goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.TabPane');
goog.require('goog.ui.Checkbox');
goog.require('goog.ui.Select');

goog.require('ola.common.GenericControl');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.ServerStats');
goog.require('ola.mobile.UniverseTab');
goog.require('ola.mobile.PluginTab');
goog.require('ola.mobile.ControllerTab');

goog.provide('ola.mobile');

var ola = ola || {};


/**
 * Setup the OLA ola_ui widgets
 * @constructor
 */
ola.MobileUI = function() {
  this.ola_server = ola.common.Server.getInstance();

  // setup the tab pane
  this.tabPane = new goog.ui.TabPane(goog.dom.$('tab_pane'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_1'), "Home"));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_2'), 'RDM'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_3'), 'Plugins'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_4'), 'Controller'));
  goog.events.listen(this.tabPane, goog.ui.TabPane.Events.CHANGE,
                     this._updateSelectedTab, false, this);

  this.server_stats = new ola.common.ServerStats();
  this.universe_tab = new ola.mobile.UniverseTab();
  this.plugin_tab = new ola.mobile.PluginTab();
  this.controller_tab = new ola.mobile.ControllerTab();
};


/**
 * Update the tab that was selected
 * @private
 */
ola.MobileUI.prototype._updateSelectedTab = function() {
  var selected_tab = this.tabPane.getSelectedIndex();

  if (selected_tab == 0) {
    this.ola_server.UpdateServerInfo();
  } else if (selected_tab == 1) {
    this.universe_tab.update();
  } else if (selected_tab == 2){
    this.plugin_tab.update();
  } else {
    this.controller_tab.update();
  }
}


ola.mobile.Setup = function() {
  var ola_ui = new ola.MobileUI();
};
goog.exportSymbol('ola.mobile.Setup', ola.mobile.Setup);
