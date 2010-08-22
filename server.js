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
 * A class that handles interactions with the ola server.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.net.XhrIoPool');

goog.provide('ola.Server');
goog.provide('ola.Server.EventType');

var ola = ola || {}


/**
 * Create a new Server object, this is used to communicate with the OLA server
 * and fires events when the state changes.
 * @constructor
 */
ola.Server = function() {
  goog.events.EventTarget.call(this);
  this.pool = new goog.net.XhrIoPool({}, 1);
}
goog.inherits(ola.Server, goog.events.EventTarget);

// This is a singleton, call ola.Server.getInstance() to access it.
goog.addSingletonGetter(ola.Server);


ola.Server.EventType = {
  PLUGIN_EVENT: 'plugin_change',
  PLUGIN_LIST_EVENT: 'plugin_list_change',
  SERVER_INFO_EVENT: 'server_info_change',
  UNIVERSE_LIST_EVENT: 'universe_list_change',
  UNIVERSE_EVENT: 'universe_change',
  SERVER_STOP_EVENT: 'server_stopped',
  PLUGIN_RELOAD_EVENT: 'plugins_reloaded',
  AVAILBLE_PORTS_EVENT: 'available_ports',
}

ola.Server.SERVER_INFO_URL = '/json/server_stats';
ola.Server.PLUGIN_INFO_URL = '/json/plugin_info';
ola.Server.UNIVERSE_INFO_URL = '/json/universe_info';
ola.Server.PLUGIN_UNIVERSE_LIST_URL = '/json/universe_plugin_list';
ola.Server.RELOAD_PLUGINS_URL = '/json/reload_plugins';
ola.Server.STOP_SERVER_URL = '/json/stop_server';
ola.Server.AVAILBLE_PORTS_URL = '/json/get_ports';


/**
 * This event is fired when the server info changes
 */
ola.ServerInfoChangeEvent = function(server_info) {
  goog.events.Event.call(this, ola.Server.EventType.SERVER_INFO_EVENT);
  this.server_info = server_info;
}
goog.inherits(ola.ServerInfoChangeEvent, goog.events.Event);


/**
 * This event is fired when the plugin list changes
 */
ola.PluginListChangeEvent = function(new_list) {
  goog.events.Event.call(this, ola.Server.EventType.PLUGIN_LIST_EVENT);
  this.plugins = new_list;
}
goog.inherits(ola.PluginListChangeEvent, goog.events.Event);


/**
 * This event is fired when the universe list changes
 */
ola.UniverseListChangeEvent = function(new_list) {
  goog.events.Event.call(this, ola.Server.EventType.UNIVERSE_LIST_EVENT);
  this.universes = new_list;
}
goog.inherits(ola.PluginListChangeEvent, goog.events.Event);


/**
 * This event is fired when the plugin info is available
 */
ola.PluginChangeEvent = function(plugin) {
  goog.events.Event.call(this, ola.Server.EventType.PLUGIN_EVENT);
  this.plugin = plugin;
}
goog.inherits(ola.PluginChangeEvent, goog.events.Event);


/**
 * This event is fired when the universe info is available
 */
ola.UniverseChangeEvent = function(universe) {
  goog.events.Event.call(this, ola.Server.EventType.UNIVERSE_EVENT);
  this.universe = universe;
}
goog.inherits(ola.UniverseChangeEvent, goog.events.Event);


/**
 * This event is fired when the server stops.
 */
ola.ServerStopEvent = function() {
  goog.events.Event.call(this, ola.Server.EventType.SERVER_STOP_EVENT);
}
goog.inherits(ola.ServerStopEvent, goog.events.Event);


/**
 * This event is fired when the plugins_reload
 */
ola.PluginReloadEvent = function() {
  goog.events.Event.call(this, ola.Server.EventType.PLUGIN_RELOAD_EVENT);
}
goog.inherits(ola.PluginReloadEvent, goog.events.Event);


/**
 * This event is fired when the available ports is ready
 */
ola.AvailablePortsEvent = function(ports) {
  goog.events.Event.call(this, ola.Server.EventType.AVAILBLE_PORTS_EVENT);
  this.ports = ports;
}
goog.inherits(ola.AvailablePortsEvent, goog.events.Event);


/**
 * Update the server info data
 */
ola.Server.prototype.UpdateServerInfo = function() {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.ServerInfoChangeEvent(obj));
    this._CleanupRequest(e.target);
  }
  this._InitiateRequest(ola.Server.SERVER_INFO_URL, on_complete);
}


/**
 * Reload the plugins
 */
ola.Server.prototype.ReloadPlugins = function() {
  var on_complete = function(e) {
    this.dispatchEvent(new ola.PluginReloadEvent());
    this._CleanupRequest(e.target);
  }
  this._InitiateRequest(ola.Server.RELOAD_PLUGINS_URL, on_complete);
}


/**
 * Stop the server
 */
ola.Server.prototype.StopServer = function () {
  var on_complete = function(e) {
    this.dispatchEvent(new ola.ServerStopEvent());
    this._CleanupRequest(e.target);
  }
  this._InitiateRequest(ola.Server.STOP_SERVER_URL, on_complete);
}


/**
 * Fetch the list of plugins & universes active on the server
 */
ola.Server.prototype.FetchUniversePluginList = function() {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.PluginListChangeEvent(obj.plugins));
    this.dispatchEvent(new ola.UniverseListChangeEvent(obj.universes));
    this._CleanupRequest(e.target);
  }
  this._InitiateRequest(ola.Server.PLUGIN_UNIVERSE_LIST_URL, on_complete);
}


/**
 * Fetch the info for a plugin
 */
ola.Server.prototype.FetchPluginInfo = function(plugin_id) {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.PluginChangeEvent(obj));
    this._CleanupRequest(e.target);
  }
  var url = ola.Server.PLUGIN_INFO_URL + '?id=' + plugin_id;
  this._InitiateRequest(url, on_complete);
}


/**
 * Fetch the info for a universe
 */
ola.Server.prototype.FetchUniverseInfo = function(universe_id) {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.UniverseChangeEvent(obj));
    this._CleanupRequest(e.target);
  }
  var url = ola.Server.UNIVERSE_INFO_URL + '?id=' + universe_id;
  this._InitiateRequest(url, on_complete);
}


/**
 * Fetch the available pors
 */
ola.Server.prototype.FetchAvailablePorts = function() {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.AvailablePortsEvent(obj));
    this._CleanupRequest(e.target);
  }
  this._InitiateRequest(ola.Server.AVAILBLE_PORTS_URL, on_complete);
}


/**
 * Initiate a JSON request
 * @param url the url to fetch
 * @param callback the callback to invoke when the request completes
 * @private
 */
ola.Server.prototype._InitiateRequest = function(url, callback) {
  var xhr = this.pool.getObject(undefined, 1);
  goog.events.listen(xhr, goog.net.EventType.COMPLETE, callback, false, this);
  xhr.send(url);
}


/**
 * Clean up from a request, this removes the listener and returns the channel
 * to the pool.
 * @private
 */
ola.Server.prototype._CleanupRequest = function(xhr) {
  goog.events.removeAll(xhr);
  this.pool.releaseObject(xhr);
}
