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
goog.require('goog.events');
goog.require('goog.ui.TabPane');
goog.require('ola.BaseFrame');
goog.require('ola.Dialog');
goog.require('ola.DmxConsoleTab');
goog.require('ola.DmxMonitorTab');
goog.require('ola.RDMPatcherTab');
goog.require('ola.RDMTab');
goog.require('ola.UniverseSettingsTab');
goog.require('ola.common.Server');

goog.provide('ola.UniverseFrame');


/**
 * The class representing the Universe frame
 * @param {string} element_id the id of the element to use for this frame.
 * @param {Object} ola_ui the OlaUI object.
 * @constructor
 */
ola.UniverseFrame = function(element_id, ola_ui) {
  ola.BaseFrame.call(this, element_id);
  this.ola_ui = ola_ui;
  this.current_universe = undefined;

  // setup the tab pane
  this.tabPane = new goog.ui.TabPane(goog.dom.$(element_id + '_tab_pane'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_1'), 'Settings'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_2'), 'RDM'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_3'), 'RDM Patcher'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_4'), 'DMX Monitor'));
  this.tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_5'), 'DMX Console'));

  this.tabs = new Array();

  this.tabs.push(
      new ola.UniverseSettingsTab(
        'tab_page_1',
        function() { ola_ui.ShowHome(); })
  );

  // We need to make the RDM pane visible here otherwise the splitter
  // doesn't render correctly.
  this.tabPane.setSelectedIndex(1);
  this.tabs.push(new ola.RDMTab('tab_page_2'));
  this.tabPane.setSelectedIndex(0);

  this.tabs.push(new ola.RDMPatcherTab('tab_page_3'));
  this.tabs.push(new ola.DmxMonitorTab('tab_page_4'));
  this.tabs.push(new ola.DmxConsoleTab('tab_page_5'));

  goog.events.listen(this.tabPane, goog.ui.TabPane.Events.CHANGE,
                     this.updateSelectedTab_, false, this);

  var server = ola.common.Server.getInstance();
  goog.events.listen(server,
                     ola.common.Server.EventType.UNIVERSE_LIST_EVENT,
                     this.newUniverseList_,
                     false, this);
};
goog.inherits(ola.UniverseFrame, ola.BaseFrame);


/**
 * Set the size of the split pane to match the parent element
 * @param {Object} e the event object.
 */
ola.UniverseFrame.prototype.setSplitPaneSize = function(e) {
  var big_frame = goog.dom.$('ola-splitpane-content');
  var big_size = goog.style.getBorderBoxSize(big_frame);

  var selected_tab = this.tabPane.getSelectedIndex();
  this.tabs[selected_tab].sizeChanged(big_size);
};


/**
 * Show this frame. We extend the base method so we can populate the correct
 * tab.
 * @param {number} universe_id the universe id to show.
 * @param {boolean} opt_select_main_tab set to true to display the main tab.
 */
ola.UniverseFrame.prototype.Show = function(universe_id, opt_select_main_tab) {
  if (this.current_universe != universe_id) {
    for (var i = 0; i < this.tabs.length; ++i) {
      this.tabs[i].setUniverse(universe_id);
    }
    this.current_universe = universe_id;
  }
  ola.UniverseFrame.superClass_.Show.call(this);
  if (opt_select_main_tab) {
    this.tabPane.setSelectedIndex(0);
  }
  this.updateSelectedTab_();
};


/**
 * Hide this frame. We extend the base method so we can notify the active tab.
 */
ola.UniverseFrame.prototype.Hide = function() {
  for (var i = 0; i < this.tabs.length; ++i) {
    if (this.tabs[i].isActive()) {
      this.tabs[i].setActive(false);
    }
  }
  ola.UniverseFrame.superClass_.Hide.call(this);
};


/**
 * Update the tab that was selected
 * @param {Object} e the event object.
 * @private
 */
ola.UniverseFrame.prototype.updateSelectedTab_ = function(e) {
  if (!this.IsVisible()) {
    return;
  }

  for (var i = 0; i < this.tabs.length; ++i) {
    if (this.tabs[i].isActive()) {
      this.tabs[i].setActive(false);
    }
  }

  this.setSplitPaneSize();
  var selected_tab = this.tabPane.getSelectedIndex();
  this.tabs[selected_tab].setActive(true);
};


/**
 * Called when new universes are available. We use this to detect if this
 * universe has been deleted.
 * @param {Object} e the event object.
 * @private
 */
ola.UniverseFrame.prototype.newUniverseList_ = function(e) {
  var found = false;
  for (var i = 0; i < e.universes.length; ++i) {
    if (e.universes[i]['id'] == this.current_universe) {
      found = true;
      break;
    }
  }

  if (this.IsVisible() && !found) {
    var dialog = ola.Dialog.getInstance();
    dialog.setTitle('Universe ' + this.current_universe + ' Removed');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent('This universe has been removed by another user.');
    dialog.setVisible(true);
    this.ola_ui.ShowHome();
  }
};
