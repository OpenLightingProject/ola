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

goog.provide('ola.common.Server');
goog.provide('ola.common.Server.EventType');


/**
 * A pending request.
 * @constructor
 * @param {string} url the URL to fetch.
 * @param {function()} callback the function to run when the request completes.
 * @param {string=} opt_method 'GET' or 'POST'.
 * @param {string=} opt_content The post form data.
 */
ola.common.Request = function(url,
                              callback,
                              opt_method,
                              opt_content) {
  this.url = url;
  this.callback = callback;
  this.opt_method = opt_method;
  this.opt_content = opt_content;
};



/**
 * Create a new Server object, this is used to communicate with the OLA server
 * and fires events when the state changes.
 * @constructor
 */
ola.common.Server = function() {
  goog.events.EventTarget.call(this);
  this.pool = new goog.net.XhrIoPool({}, 1);
  this.universes = {};
  this.request_queue = new Array();
};
goog.inherits(ola.common.Server, goog.events.EventTarget);

// This is a singleton, call ola.common.Server.getInstance() to access it.
goog.addSingletonGetter(ola.common.Server);


/**
 * Events for the Server object.
 * @type {Object}
 */
ola.common.Server.EventType = {
  PLUGIN_EVENT: 'plugin_change',
  PLUGIN_LIST_EVENT: 'plugin_list_change',
  SERVER_INFO_EVENT: 'server_info_change',
  UNIVERSE_EVENT: 'universe_change',
  UNIVERSE_LIST_EVENT: 'universe_list_change'
};

/**
 * The url for the server stats
 * @type {string}
 */
ola.common.Server.SERVER_INFO_URL = 'json/server_stats';

/**
 * The url for the plugin info
 * @type {string}
 */
ola.common.Server.PLUGIN_INFO_URL = 'json/plugin_info';

/**
 * The url for the universe info.
 * @type {string}
 */
ola.common.Server.UNIVERSE_INFO_URL = 'json/universe_info';

/**
 * The url to fetch the list of universes.
 * @type {string}
 */
ola.common.Server.PLUGIN_UNIVERSE_LIST_URL = 'json/universe_plugin_list';

/**
 * The url to reload the server
 * @type {string}
 */
ola.common.Server.RELOAD_PLUGINS_URL = 'reload';

/**
 * The url to stop the server.
 * @type {string}
 */
ola.common.Server.STOP_SERVER_URL = 'quit';

/**
 * The url for the universe info.
 * @type {string}
 */
ola.common.Server.AVAILBLE_PORTS_URL = 'json/get_ports';

/**
 * The url to fetch the list of UIDs.
 * @type {string}
 */
ola.common.Server.UIDS_URL = 'json/rdm/uids';

/**
 * The url to trigger discovery.
 * @type {string}
 */
ola.common.Server.RDM_DISCOVERY_URL = 'rdm/run_discovery';

/**
 * The url to fetch the RDM sections.
 * @type {string}
 */
ola.common.Server.RDM_SECTIONS_URL = 'json/rdm/supported_sections';

/**
 * The url to fetch the contents of a section.
 * @type {string}
 */
ola.common.Server.RDM_GET_SECTION_INFO_URL = 'json/rdm/section_info';

/**
 * The url to set the RDM settings.
 * @type {string}
 */
ola.common.Server.RDM_SET_SECTION_INFO_URL = 'json/rdm/set_section_info';

/**
 * The url to toggle RDM identify mode.
 * @type {string}
 */
ola.common.Server.RDM_UID_IDENTIFY = 'json/rdm/uid_identify';

/**
 * The url to fetch information about a RDM UID (responder).
 * @type {string}
 */
ola.common.Server.RDM_UID_INFO = 'json/rdm/uid_info';

/**
 * The url to change the personality of a responder
 * @type {string}
 */
ola.common.Server.RDM_UID_PERSONALITY = 'json/rdm/uid_personalities';

/**
 * The url to create a new universe.
 * @type {string}
 */

ola.common.Server.NEW_UNIVERSE_URL = 'new_universe';

/**
 * The url to change the settings of a universe.
 * @type {string}
 */
ola.common.Server.MODIFY_UNIVERSE_URL = 'modify_universe';

/**
 * The url to set the DMX values.
 * @type {string}
 */
ola.common.Server.SET_DMX_URL = 'set_dmx';

/**
 * The url to return the current DMX values
 * @type {string}
 */
ola.common.Server.GET_DMX_URL = 'get_dmx';

/**
 * The request queue size.
 * This should be more than the max # of RDM sections we ever expect
 * @type {number}
 */
ola.common.Server.REQUEST_QUEUE_LIMIT = 30;

/**
 * This event is fired when the server info changes
 * @constructor
 * @param {Object} server_info the server info.
 */
ola.common.ServerInfoChangeEvent = function(server_info) {
  goog.events.Event.call(this, ola.common.Server.EventType.SERVER_INFO_EVENT);
  this.server_info = server_info;
};
goog.inherits(ola.common.ServerInfoChangeEvent, goog.events.Event);


/**
 * This event is fired when the plugin list changes
 * @constructor
 * @param {Array.<Object>} new_list the list of Plugin  objects.
 */
ola.PluginListChangeEvent = function(new_list) {
  goog.events.Event.call(this, ola.common.Server.EventType.PLUGIN_LIST_EVENT);
  this.plugins = new_list;
};
goog.inherits(ola.PluginListChangeEvent, goog.events.Event);


/**
 * This event is fired when the universe list changes
 * @constructor
 * @param {Array.<Object>} new_list the list of Universe objects.
 */
ola.UniverseListChangeEvent = function(new_list) {
  goog.events.Event.call(this,
                         ola.common.Server.EventType.UNIVERSE_LIST_EVENT);
  this.universes = new_list;
};
goog.inherits(ola.PluginListChangeEvent, goog.events.Event);


/**
 * This event is fired when the plugin info is available
 * @constructor
 * @param {Object} plugin the new Plugin object.
 */
ola.PluginChangeEvent = function(plugin) {
  goog.events.Event.call(this, ola.common.Server.EventType.PLUGIN_EVENT);
  this.plugin = plugin;
};
goog.inherits(ola.PluginChangeEvent, goog.events.Event);


/**
 * This event is fired when the universe info is available
 * @constructor
 * @param {Object} universe the Universe object.
 */
ola.UniverseChangeEvent = function(universe) {
  goog.events.Event.call(this, ola.common.Server.EventType.UNIVERSE_EVENT);
  this.universe = universe;
};
goog.inherits(ola.UniverseChangeEvent, goog.events.Event);


/**
 * Check if this universe is active
 * @param {number} universe_id the ID of the universe to check.
 * @return {boolean} true if the universe exists, false otherwise.
 */
ola.common.Server.prototype.CheckIfUniverseExists = function(universe_id) {
  return this.universes[universe_id] != undefined;
};


/**
 * Update the server info data
 */
ola.common.Server.prototype.UpdateServerInfo = function() {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.common.ServerInfoChangeEvent(obj));
  };
  this._initiateRequest(ola.common.Server.SERVER_INFO_URL, on_complete);
};


/**
 * Reload the plugins
 * @param {function(Object)} callback the function to call when the request
 * completes.
 */
ola.common.Server.prototype.reloadPlugins = function(callback) {
  this._initiateRequest(ola.common.Server.RELOAD_PLUGINS_URL, callback);
};


/**
 * Stop the server
 * @param {function(Object)} callback the function to call when the request
 * completes.
 */
ola.common.Server.prototype.stopServer = function(callback) {
  this._initiateRequest(ola.common.Server.STOP_SERVER_URL, callback);
};


/**
 * Fetch the list of plugins & universes active on the server
 */
ola.common.Server.prototype.FetchUniversePluginList = function() {
  var on_complete = function(e) {
    if (e.target.getStatus() != 200) {
      ola.logger.info('Request failed: ' + e.target.getLastUri() + ' : ' +
          e.target.getLastError());
      return;
    }
    var obj = e.target.getResponseJson();

    // update the internal list of universes here
    this.universes = {};
    for (var i = 0; i < obj['universes'].length; ++i) {
      this.universes[obj['universes'][i]['id']] = true;
    }
    this.dispatchEvent(new ola.PluginListChangeEvent(obj['plugins']));
    this.dispatchEvent(new ola.UniverseListChangeEvent(obj['universes']));
  };
  this._initiateRequest(ola.common.Server.PLUGIN_UNIVERSE_LIST_URL,
                        on_complete);
};


/**
 * Fetch the info for a plugin
 * @param {number} plugin_id the id of the plugin to fetch.
 */
ola.common.Server.prototype.FetchPluginInfo = function(plugin_id) {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.PluginChangeEvent(obj));
  };
  var url = ola.common.Server.PLUGIN_INFO_URL + '?id=' + plugin_id;
  this._initiateRequest(url, on_complete);
};


/**
 * Fetch the info for a universe
 * @param {number} universe_id the id of the universe to fetch.
 */
ola.common.Server.prototype.FetchUniverseInfo = function(universe_id) {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.UniverseChangeEvent(obj));
  };
  var url = ola.common.Server.UNIVERSE_INFO_URL + '?id=' + universe_id;
  this._initiateRequest(url, on_complete);
};


/**
 * Fetch the available ports.
 * @param {number=} opt_universe an optional universe id.
 * @param {function()} callback the callback to invoke when complete.
 */
ola.common.Server.prototype.fetchAvailablePorts = function(opt_universe,
                                                           callback) {
  var url = ola.common.Server.AVAILBLE_PORTS_URL;
  if (opt_universe != undefined) {
    url += '?id=' + opt_universe;
  }
  this._initiateRequest(url, callback);
};


/**
 * Create a new universe
 * @param {number} universe_id the ID of the universe.
 * @param {string} name the new universe name.
 * @param {Array.<number>} port_ids a list of ports to patch to this universe.
 * @param {function(Object)} callback the function to call when the request
 *   completes.
 */
ola.common.Server.prototype.createUniverse = function(universe_id,
                                                      name,
                                                      port_ids,
                                                      callback) {
  var post_data = 'id=' + universe_id + (
      name ? '&name=' + encodeURI(name) : '') + '&add_ports=' +
      port_ids.join(',');
  this._initiateRequest(ola.common.Server.NEW_UNIVERSE_URL, callback, 'POST',
                        post_data);
};


/**
 * Trigger RDM discovery for this universe. This returns when discovery
 *   completes.
 * @param {number} universe_id the ID of the universe to run discovery for.
 * @param {boolean} full true if we should do full discovery, false for
 *   incremental.
 * @param {function(Object)} callback the function to call when the discovery
 *   request is ack'ed.
 */
ola.common.Server.prototype.runRDMDiscovery = function(universe_id,
                                                       full,
                                                       callback) {
  var url = ola.common.Server.RDM_DISCOVERY_URL + '?id=' + universe_id;
  if (!full) {
    url += '&incremental=true';
  }
  this._initiateRequest(url, callback);
};


/**
 * Get the list of supported sections for a UID.
 * @param {number} universe_id the ID of the universe.
 * @param {string} uid the string representation of a UID.
 * @param {function(Object)} callback the function to call when the discovery
 *   request is ack'ed.
 */
ola.common.Server.prototype.rdmGetSupportedSections = function(universe_id,
                                                               uid,
                                                               callback) {
  var url = (ola.common.Server.RDM_SECTIONS_URL + '?id=' + universe_id +
      '&uid=' + uid);
  this._initiateRequest(url, callback);
};


/**
 * Get the details for a particular rdm section
 * @param {number} universe_id the ID of the universe.
 * @param {string} uid the string representation of a UID.
 * @param {string} section_name the section to get.
 * @param {string} hint an arbitary string passed back to the server.
 * @param {function(Object)} callback the function to call when the discovery
 *   request is ack'ed.
 */
ola.common.Server.prototype.rdmGetSectionInfo = function(universe_id,
                                                         uid,
                                                         section_name,
                                                         hint,
                                                         callback) {
  var url = (ola.common.Server.RDM_GET_SECTION_INFO_URL + '?id=' +
      universe_id + '&uid=' + uid + '&section=' + section_name + '&hint=' +
      hint);
  this._initiateRequest(url, callback);
};


/**
 * Get the details for a particular rdm section
 * @param {number} universe_id the ID of the universe.
 * @param {string} uid the string representation of a UID.
 * @param {string} section_name the section to get.
 * @param {string} hint a cookie to pass back to the server.
 * @param {data} data passed back to the server.
 * @param {function(Object)} callback the function to call when the discovery
 *   request is ack'ed.
 */
ola.common.Server.prototype.rdmSetSectionInfo = function(universe_id,
                                                         uid,
                                                         section_name,
                                                         hint,
                                                         data,
                                                         callback) {
  var url = (ola.common.Server.RDM_SET_SECTION_INFO_URL + '?id=' +
      universe_id + '&uid=' + uid + '&section=' + section_name + '&hint=' +
      hint + '&' + data);
  this._initiateRequest(url, callback);
};


/**
 * Fetch the uids for a universe
 * @param {number} universe_id the ID of the universe.
 * @param {function(Object)} callback the function to call when the request
 *   completes.
 */
ola.common.Server.prototype.fetchUids = function(universe_id, callback) {
  var url = ola.common.Server.UIDS_URL + '?id=' + universe_id;
  this._initiateRequest(url, callback);
};


/**
 * Fetch the dmx start address, footprint & personality for a uid.
 * @param {number} universe_id the ID of the universe.
 * @param {string} uid the string representation of a UID.
 * @param {function(Object)} callback the function to call when the request
 *   completes.
 */
ola.common.Server.prototype.rdmGetUIDInfo = function(universe_id,
                                                     uid,
                                                     callback) {
  var url = (ola.common.Server.RDM_UID_INFO + '?id=' + universe_id +
      '&uid=' + uid);
  this._initiateRequest(url, callback);
};


/**
 * Check if a device is in identify mode.
 * @param {number} universe_id the ID of the universe.
 * @param {string} uid the string representation of a UID.
 * @param {function(Object)} callback the function to call when the request
 *   completes.
 */
ola.common.Server.prototype.rdmGetUIDIdentifyMode = function(universe_id,
                                                             uid,
                                                             callback) {
  var url = (ola.common.Server.RDM_UID_IDENTIFY + '?id=' + universe_id +
      '&uid=' + uid);
  this._initiateRequest(url, callback);
};


/**
 * Fetch the personalities for a device
 * @param {number} universe_id the ID of the universe.
 * @param {string} uid the string representation of a UID.
 * @param {function(Object)} callback the function to call when the request
 *   completes.
 */
ola.common.Server.prototype.rdmGetUIDPersonalities = function(universe_id,
                                                              uid,
                                                              callback) {
  var url = (ola.common.Server.RDM_UID_PERSONALITY + '?id=' + universe_id +
      '&uid=' + uid);
  this._initiateRequest(url, callback);
};


/**
 * Update the settings for a universe.
 * @param {number} universe_id the id of the universe to modify.
 * @param {string} universe_name the new name.
 * @param {string} merge_mode HTP or LTP.
 * @param {Array.<{{id: string, mode: string, priority: number}}>
 *   port_priorities an array of new port priorities.
 * @param {Array.<string>} ports_to_remove list of port ids to remove.
 * @param {Array.<string>} ports_to_add list of port ids to add.
 * @param {function()} callback the callback to invoke when complete.
 */
ola.common.Server.prototype.modifyUniverse = function(universe_id,
                                                      universe_name,
                                                      merge_mode,
                                                      port_priorities,
                                                      ports_to_remove,
                                                      ports_to_add,
                                                      callback) {
  var post_data = ('id=' + universe_id + '&name=' + universe_name +
      '&merge_mode=' + merge_mode + '&add_ports=' + ports_to_add.join(',') +
      '&remove_ports=' + ports_to_remove.join(','));

  modified_port_ids = new Array();
  for (var i = 0; i < port_priorities.length; ++i) {
    var priority_setting = port_priorities[i];
    post_data += ('&' + priority_setting.id + '_priority_value=' +
        priority_setting.priority);
    if (priority_setting.mode != undefined) {
      post_data += ('&' + priority_setting.id + '_priority_mode=' +
          priority_setting.mode);
    }
    modified_port_ids.push(priority_setting.id);
  }
  post_data += ('&modify_ports=' + modified_port_ids.join(','));
  var url = ola.common.Server.MODIFY_UNIVERSE_URL;
  this._initiateRequest(url, callback, 'POST', post_data);
};


/**
 * Get the dmx values for a universe
 * @param {number} universe_id the id of the universe to get values for.
 * @param {function(e)} callback the callback to invoke when complete.
 */
ola.common.Server.prototype.getChannelValues = function(universe_id,
                                                        callback) {
  var url = ola.common.Server.GET_DMX_URL + '?u=' + universe_id;
  this._initiateRequest(
      url,
      function(e) {
        callback(e.target.getResponseJson());
      });
};


/**
 * Update the dmx values for a universe
 * @param {number} universe_id the id of the universe to modify.
 * @param {Array.<number>} data the channel values.
 * @param {function(e)} callback the callback to invoke when complete.
 */
ola.common.Server.prototype.setChannelValues = function(universe_id,
                                                        data,
                                                        callback) {
  var post_data = 'u=' + universe_id + '&d=' + data.join(',');
  var url = ola.common.Server.SET_DMX_URL;
  this._initiateRequest(
      url,
      function(e) {
        callback(e.target);
      },
      'POST',
      post_data);
};



/**
 * Check if a request completed properly and if not, show a dialog.
 * This checks just the HTTP code.
 * @param {Object} e the event object.
 * @return {boolean} true if ok, false otherwise.
 */
ola.common.Server.prototype.checkStatusDialog = function(e) {
  if (e.target.getStatus() != 200) {
    this._showErrorDialog(e.target.getLastUri() + ' : ' +
                          e.target.getLastError());
    return false;
  }
  return true;
};


/**
 * Check if a request completed properly and if not, show a dialog.
 * This checks both the HTTP code, and the existance of the 'error' property in
 * the response.
 * @param {Object} e the event object.
 * @return {object} The JSON output, or undefined if an error occured.
 */
ola.common.Server.prototype.checkForErrorDialog = function(e) {
  if (e.target.getStatus() == 200) {
    var response = e.target.getResponseJson();
    if (response['error']) {
      this._showErrorDialog(response['error']);
      return undefined;
    }
    return response;
  } else {
    this._showErrorDialog(e.target.getLastUri() + ' : ' +
                          e.target.getLastError());
    return undefined;
  }
};


/**
 * Check if a request completed properly and if not log the error
 * This checks both the HTTP code, and the existance of the 'error' property in
 * the response.
 * @param {Object} e the event object.
 * @return {object} The JSON output, or undefined if an error occured.
 */
ola.common.Server.prototype.checkForErrorLog = function(e) {
  if (e.target.getStatus() == 200) {
    var response = e.target.getResponseJson();
    if (response['error']) {
      ola.logger.info(response['error']);
      return undefined;
    }
    return response;
  } else {
    ola.logger.info(e.target.getLastUri() + ' : ' +
                    e.target.getLastError());
    return undefined;
  }
};


/**
 * Show the error dialog
 * @param {string} message the error message.
 */
ola.common.Server.prototype._showErrorDialog = function(message) {
  var dialog = ola.Dialog.getInstance();
  dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
  dialog.setTitle('Request Failed');
  dialog.setContent(message);
  dialog.setVisible(true);
};


/**
 * Initiate a JSON request
 * @param {string} url the url to fetch.
 * @param {function()} callback the callback to invoke when the request
 *   completes.
 * @param {string=} opt_method 'GET' or 'POST'.
 * @param {string=} opt_content The post form data.
 */
ola.common.Server.prototype._initiateRequest = function(url,
                                                        callback,
                                                        opt_method,
                                                        opt_content) {
  if (this.request_queue.length >= ola.common.Server.REQUEST_QUEUE_LIMIT) {
    var dialog = ola.Dialog.getInstance();
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setTitle('Failed to Communicate with Server');
    dialog.setContent(
        'The request pool was empty, the server is probably down.');
    dialog.setVisible(true);
    return;
  }

  var request = new ola.common.Request(url, callback, opt_method, opt_content);
  this.request_queue.push(request);

  var t = this;
  this.pool.getObject(
    function(xhr) {
      if (!t.request_queue.length)
        return;
      var r = t.request_queue.shift();
      if (r.callback)
        goog.events.listen(xhr,
                           goog.net.EventType.COMPLETE,
                           r.callback,
                           false,
                           t);
      goog.events.listen(xhr,
                         goog.net.EventType.READY,
                         t._cleanupRequest,
                         false,
                         t);
      xhr.send(r.url, r.opt_method, r.opt_content);
    },
    1);
};


/**
 * Clean up from a request, this removes the listener and returns the channel
 * to the pool.
 * @param {Object} e the event object.
 */
ola.common.Server.prototype._cleanupRequest = function(e) {
  var xhr = e.target;
  goog.events.removeAll(xhr);
  this.pool.releaseObject(xhr);
};
