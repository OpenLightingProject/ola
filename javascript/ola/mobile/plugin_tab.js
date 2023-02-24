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
 * The plugin tab for the mobile UI.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.events');
goog.require('goog.ui.Container');

goog.require('ola.BaseFrame');
goog.require('ola.common.PluginControlFactory');
goog.require('ola.common.PluginItem');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.SortedList');

goog.provide('ola.mobile.PluginTab');


/**
 * The class representing the Plugin frame
 * @constructor
 */
ola.mobile.PluginTab = function() {
  this.plugin_frame = new ola.BaseFrame('plugin_frame');
  this.plugin_info_frame = new ola.BaseFrame('plugin_info_frame');

  this.hideAllFrames_();

  this.plugin_list = undefined;

  this.ola_server = ola.common.Server.getInstance();
  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.PLUGIN_LIST_EVENT,
                     this.updatePluginList_,
                     false, this);

  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.PLUGIN_EVENT,
                     this.updatePluginInfo_,
                     false, this);
};


/**
 *  The title of this tab
 */
ola.mobile.PluginTab.prototype.title = function() {
  return 'Plugins';
};


/**
 * Called when the tab loses focus
 */
ola.mobile.PluginTab.prototype.blur = function() {};


/**
 * Hide all frames
 * @private
 */
ola.mobile.PluginTab.prototype.hideAllFrames_ = function() {
  this.plugin_frame.Hide();
  this.plugin_info_frame.Hide();
};


/**
 * Called when the plugin tab is clicked
 */
ola.mobile.PluginTab.prototype.update = function() {
  this.hideAllFrames_();
  this.plugin_frame.setAsBusy();
  this.plugin_list = undefined;
  this.plugin_frame.Show();
  this.ola_server.FetchUniversePluginList();
};


/**
 * Called when a new list of plugins is received.
 * @param {Object} e the event object.
 * @private
 */
ola.mobile.PluginTab.prototype.updatePluginList_ = function(e) {
  if (this.plugin_list == undefined) {
    this.plugin_frame.Clear();
    var plugin_container = new goog.ui.Container();
    plugin_container.render(this.plugin_frame.element);

    var tab = this;
    this.plugin_list = new ola.common.SortedList(
        plugin_container,
        new ola.common.PluginControlFactory(
          function(item) { tab.pluginSelected_(item.id()); }));
  }

  var items = new Array();
  for (var i = 0; i < e.plugins.length; ++i) {
    var item = new ola.common.PluginItem(e.plugins[i]);
    items.push(item);
  }
  this.plugin_list.updateFromData(items);
};


/**
 * Called when a plugin is selected.
 * @param {number} plugin_id the id of the plugin selected.
 * @private
 */
ola.mobile.PluginTab.prototype.pluginSelected_ = function(plugin_id) {
  this.hideAllFrames_();
  this.plugin_info_frame.setAsBusy();
  this.plugin_info_frame.Show();
  this.ola_server.FetchPluginInfo(plugin_id);
};


/**
 * Called when new plugin info is available
 * @param {Object} e the event object.
 * @private
 */
ola.mobile.PluginTab.prototype.updatePluginInfo_ = function(e) {
  this.plugin_info_frame.Clear();
  var description = goog.string.htmlEscape(e.plugin['description']);
  description = description.replace(/\\n/g, '<br>');
  this.plugin_info_frame.element.innerHTML = description;
};
