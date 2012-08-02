goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.Checkbox');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.Select');
goog.require('goog.ui.TabPane');

goog.require('ola.common.GenericControl');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.ServerStats');
goog.require('ola.mobile.ControllerTab');
goog.require('ola.mobile.MonitorTab');
goog.require('ola.mobile.PluginTab');
goog.require('ola.mobile.UniverseTab');

goog.provide('ola.mobile');

/**
 * Setup the OLA ola_ui widgets
 * @constructor
 */
ola.MobileUI = function() {
  this.tabs = new Array();
  this.tabs.push(new ola.common.ServerStats());
  this.tabs.push(new ola.mobile.UniverseTab());
  this.tabs.push(new ola.mobile.MonitorTab());
  this.tabs.push(new ola.mobile.ControllerTab());
  this.tabs.push(new ola.mobile.PluginTab());

  // setup the tab pane
  this.tabPane = new goog.ui.TabPane(goog.dom.$('tab_pane'));
  for (var i = 0; i < this.tabs.length; ++i) {
    this.tabPane.addPage(new goog.ui.TabPane.TabPage(
      goog.dom.$('tab_page_' + i),
      this.tabs[i].title()));
  }
  goog.events.listen(this.tabPane, goog.ui.TabPane.Events.CHANGE,
                     this.updateSelectedTab, false, this);
};


/**
 * Update the tab that was selected
 */
ola.MobileUI.prototype.updateSelectedTab = function() {
  var selected_tab = this.tabPane.getSelectedIndex();

  for (var i = 0; i < this.tabs.length; ++i) {
    if (i != selected_tab) {
      this.tabs[i].blur();
    }
  }
  this.tabs[selected_tab].update();
};


/**
 * The main setup function.
 */
ola.mobile.Setup = function() {
  var ola_ui = new ola.MobileUI();
};
goog.exportSymbol('ola.mobile.Setup', ola.mobile.Setup);
