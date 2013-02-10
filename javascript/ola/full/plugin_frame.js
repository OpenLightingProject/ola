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
 * The plugin frame.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('ola.BaseFrame');
goog.require('ola.common.PluginItem');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');

goog.provide('ola.PluginFrame');


/**
 * The class representing the Plugin frame
 * @param {string} element_id the ID of the element to use.
 * @constructor
 */
ola.PluginFrame = function(element_id, plugin_control_factory) {
  ola.BaseFrame.call(this, element_id);
  this.plugin_control_factory = plugin_control_factory;
  goog.events.listen(
      ola.common.Server.getInstance(),
      ola.common.Server.EventType.PLUGIN_EVENT,
      this._UpdateFromData,
      false,
      this);

  this.conflict_list_container = new goog.ui.Container(
    goog.ui.Container.Orientation.HORIZONTAL);
  this.conflict_list_container.setFocusable(false);
  this.conflict_list_container.decorate(goog.dom.$('plugin_conflict_list'));
};
goog.inherits(ola.PluginFrame, ola.BaseFrame);


/**
 * Update this plugin frame from a Plugin object
 * @param {ola.PluginChangeEvent} e the plugin event.
 */
ola.PluginFrame.prototype._UpdateFromData = function(e) {
  var description = e.plugin['description']
  description = description.replace(/\n/g, '<br>');
  goog.dom.$('plugin_name').innerHTML = e.plugin['name'];
  var enabled_span = goog.dom.$('plugin_enabled');
  if (e.plugin['enabled']) {
    enabled_span.innerHTML = 'Enabled';
    enabled_span.className = 'plugin_enabled';
  } else {
    enabled_span.innerHTML = 'Disabled';
    enabled_span.className = 'plugin_disabled';
  }

  var conflict_row = goog.dom.$('plugin_conflict_row');
  var conflict_list = e.plugin['conflicts_with'];
  if (conflict_list.length) {
    conflict_row.style.display = 'table-row';
    this.conflict_list_container.removeChildren(true);
    var fn = this.show_plugin_fn;
    for (var i = 0; i < conflict_list.length; ++i) {
      var plugin = new ola.common.PluginItem(conflict_list[i]);
      var component = this.plugin_control_factory.newComponent(plugin);
      component.addClassName('goog-inline-block');
      this.conflict_list_container.addChild(component, true);
    }
  } else {
    conflict_row.style.display = 'none';
  }
  goog.dom.$('plugin_description').innerHTML = description;
};
