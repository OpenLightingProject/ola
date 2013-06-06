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
goog.require('goog.string');
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
ola.PluginFrame = function(element_id, show_plugin_fn) {
  ola.BaseFrame.call(this, element_id);
  this._show_plugin_fn = show_plugin_fn;
  goog.events.listen(
      ola.common.Server.getInstance(),
      ola.common.Server.EventType.PLUGIN_EVENT,
      this.UpdateFromData_,
      false,
      this);

  this.controls = new Array();
};
goog.inherits(ola.PluginFrame, ola.BaseFrame);


/**
 * Update this plugin frame from a Plugin object
 * @param {ola.PluginChangeEvent} e the plugin event.
 * @private
 */
ola.PluginFrame.prototype.UpdateFromData_ = function(e) {
  goog.dom.$('plugin_name').innerHTML = e.plugin['name'];
  goog.dom.$('plugin_preference_source').innerHTML =
    e.plugin['preferences_source'];
  var enabled_span = goog.dom.$('plugin_enabled');
  if (e.plugin['enabled']) {
    enabled_span.innerHTML = 'Yes';
    enabled_span.className = 'plugin_enabled';
  } else {
    enabled_span.innerHTML = 'No';
    enabled_span.className = 'plugin_disabled';
  }

  var active_span = goog.dom.$('plugin_active');
  if (e.plugin['active']) {
    active_span.innerHTML = 'Yes';
    active_span.className = 'plugin_enabled';
  } else {
    active_span.innerHTML = 'No';
    active_span.className = 'plugin_disabled';
  }

  var possible_conflicts = e.plugin['enabled'] && !e.plugin['active'];
  var conflict_row = goog.dom.$('plugin_conflict_row');
  var conflict_list = e.plugin['conflicts_with'];
  if (conflict_list.length) {
    conflict_row.style.display = 'table-row';

    // remove old controls
    for (var i = 0; i < this.controls.length; ++i) {
      this.controls[i].dispose();
    }
    this.controls = new Array();
    var conflicts = goog.dom.$('plugin_conflict_list');
    conflicts.innerHTML = '';

    // add new controls
    for (var i = 0; i < conflict_list.length; ++i) {
      var plugin = conflict_list[i];
      var control = new goog.ui.Control(
          goog.dom.createDom('span', null, plugin['name']));
      control.render(conflicts);
      this.AttachListener_(control, plugin['id']);
      this.controls.push(control);

      if (possible_conflicts && plugin['active']) {
        var icon = goog.dom.createDom('img', {'src': '/warning.png'});
        goog.dom.appendChild(conflicts, icon);
      }
      goog.dom.appendChild(conflicts, goog.dom.createDom('br'));
    }
  } else {
    conflict_row.style.display = 'none';
  }
  var description = goog.string.htmlEscape(e.plugin['description']);
  description = description.replace(/\n/g, '<br>');
  goog.dom.$('plugin_description').innerHTML = description;
};


/**
 * Called when a plugin name is clicked.
 * @param {ola.int} id the plugin id.
 * @private
 */
ola.PluginFrame.prototype.PluginControlClicked_ = function(id) {
  this._show_plugin_fn(id);
};

/**
 * Attach a listener to a control.
 * @private
 */
ola.PluginFrame.prototype.AttachListener_ = function(control, plugin_id) {
  goog.events.listen(control, goog.ui.Component.EventType.ACTION,
                     function(e) {
                       this.PluginControlClicked_(plugin_id);
                     }, false, this);
};
