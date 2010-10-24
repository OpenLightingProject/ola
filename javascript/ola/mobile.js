goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.TabPane');
goog.require('goog.ui.Checkbox');
goog.require('goog.ui.Select');

goog.require('ola.Dialog');
goog.require('ola.common.GenericControl');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.ServerStats');
goog.require('ola.mobile.UniverseTab');
goog.require('ola.mobile.PluginTab');

goog.provide('ola.Mobile');

var ola = ola || {};


/**
 * Setup the OLA ola_ui widgets
 * @constructor
 */
ola.MobileUI = function() {
  this.ola_server = ola.common.Server.getInstance();

  var foo = new goog.ui.Checkbox();

  // setup the tab pane
  this.tabPane = new goog.ui.TabPane(goog.dom.$('tab_pane'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_1'), "Home"));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_2'), 'RDM'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_3'), 'Plugins'));

  goog.events.listen(this.tabPane, goog.ui.TabPane.Events.CHANGE,
                     this._updateSelectedTab, false, this);

  goog.events.listen(this.ola_server, ola.common.Server.EventType.SERVER_INFO_EVENT,
                     this._updateServerInfo,
                     false, this);

  this.server_stats = new ola.common.ServerStats();
  this.universe_tab = new ola.mobile.UniverseTab();
  this.plugin_tab = new ola.mobile.PluginTab();
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
  } else {
    this.plugin_tab.update();
  }
}



/**
 * Update the tab that was selected
 * @private
 */
ola.MobileUI.prototype._updateServerInfo = function() {


}


ola.Mobile = function() {
  var ola_ui = new ola.MobileUI();
};
goog.exportSymbol('ola.Mobile', ola.Mobile);
