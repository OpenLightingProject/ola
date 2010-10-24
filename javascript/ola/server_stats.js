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
 * The class the manages the server stats table.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.Component');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');

goog.provide('ola.common.ServerStats');


/**
 * A class that updates the server stats.
 * @param {string} element_id the id of the div to use for the home frame.
 * @constructor
 */
ola.common.ServerStats = function(element_id) {
  var ola_server = ola.common.Server.getInstance();

  goog.events.listen(ola_server, ola.common.Server.EventType.SERVER_INFO_EVENT,
                     this._updateServerInfo,
                     false, this);

  // update the server info now
  ola_server.UpdateServerInfo();
};


/**
 * Update the home frame with new server data
 */
ola.common.ServerStats.prototype._updateServerInfo = function(e) {
  goog.dom.$('server_hostname').innerHTML = e.server_info['hostname'];
  goog.dom.$('server_ip').innerHTML = e.server_info['ip'];
  goog.dom.$('server_broadcast').innerHTML = e.server_info['broadcast'];
  goog.dom.$('server_mac').innerHTML = e.server_info['hw_address'];
  goog.dom.$('server_version').innerHTML = e.server_info['version'];
  goog.dom.$('server_uptime').innerHTML = e.server_info['up_since'];

  if (!e.server_info['quit_enabled']) {
    var stop_button = goog.dom.$('stop_button');
    if (stop_button) {
      stop_button.style.display = 'none';
    }
  }
};
