/**
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The mobile OLA UI.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.TabPane');

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
