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
 * The plugin tab for the mobile UI.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.events');
goog.require('goog.ui.Container');

goog.require('ola.BaseFrame');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.SortedList');
goog.require('ola.common.PluginItem');
goog.require('ola.common.PluginControlFactory');

goog.provide('ola.mobile.PluginTab');


/**
 * The class representing the Universe frame
 * @param {string} element_id the id of the element to use for this frame.
 * @constructor
 */
ola.mobile.PluginTab = function() {
  this.plugin_frame = new ola.BaseFrame('plugin_frame');
  this.plugin_info_frame = new ola.BaseFrame('plugin_info_frame');

  this._hideAllFrames();

  this.plugin_list = undefined;

  this.ola_server = ola.common.Server.getInstance();
  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.PLUGIN_LIST_EVENT,
                     this._updatePluginList,
                     false, this);

  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.PLUGIN_EVENT,
                     this._updatePluginInfo,
                     false, this);
};


/**
 * Hide all frames
 */
ola.mobile.PluginTab.prototype._hideAllFrames = function() {
  this.plugin_frame.Hide();
  this.plugin_info_frame.Hide();
};


/**
 * Caled when the plugin tab is clicked
 */
ola.mobile.PluginTab.prototype.update = function() {
  this._hideAllFrames();
  this.plugin_frame.setAsBusy();
  this.plugin_list = undefined;
  this.plugin_frame.Show();
  this.ola_server.FetchUniversePluginList();
};


/**
 * Called when a new list of plugins is received.
 */
ola.mobile.PluginTab.prototype._updatePluginList = function(e) {
  if (this.plugin_list == undefined) {
    this.plugin_frame.Clear();
    var plugin_container = new goog.ui.Container();
    plugin_container.render(this.plugin_frame.element);

    var tab = this;
    this.plugin_list = new ola.common.SortedList(
        plugin_container,
        new ola.common.PluginControlFactory(
          function(item) { tab._pluginSelected(item.id()); }));
  }

  var items = new Array();
  for (var i = 0; i < e.plugins.length; ++i) {
    var item = new ola.common.PluginItem(e.plugins[i]);
    items.push(item);
  }
  this.plugin_list.updateFromData(items);
};


/**
 * Called when a plugin is selected
 */
ola.mobile.PluginTab.prototype._pluginSelected = function(plugin_id) {
  this._hideAllFrames();
  this.plugin_info_frame.setAsBusy();
  this.plugin_info_frame.Show();
  this.ola_server.FetchPluginInfo(plugin_id);
};


/**
 * Called when new plugin info is available
 */
ola.mobile.PluginTab.prototype._updatePluginInfo = function(e) {
  this.plugin_info_frame.Clear();
  var description = e.plugin['description'];
  description = description.replace(/\n/g, '<br>');
  this.plugin_info_frame.element.innerHTML = description;
};
