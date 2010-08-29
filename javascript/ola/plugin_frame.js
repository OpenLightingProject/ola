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
goog.require('ola.Server');
goog.require('ola.Server.EventType');

goog.provide('ola.PluginFrame');

var ola = ola || {};


/**
 * The class representing the Plugin frame
 * @param {string} element_id the ID of the element to use.
 * @constructor
 */
ola.PluginFrame = function(element_id) {
  ola.BaseFrame.call(this, element_id);
  goog.events.listen(
      ola.Server.getInstance(),
      ola.Server.EventType.PLUGIN_EVENT,
      this._UpdateFromData,
      false,
      this);
};
goog.inherits(ola.PluginFrame, ola.BaseFrame);


/**
 * Update this plugin frame from a Plugin object
 * @param {ola.PluginChangeEvent} e the plugin event.
 */
ola.PluginFrame.prototype._UpdateFromData = function(e) {
  goog.dom.$('plugin_description').innerHTML = e.plugin['description'];
};
