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
 * The RDM patcher tab.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.ui.Toolbar');
goog.require('goog.dom');
goog.require('goog.ui.ToolbarButton');
goog.require('goog.ui.ToolbarMenuButton')
goog.require('goog.ui.ToolbarSeparator');

goog.require('ola.RDMPatcher');
goog.require('ola.RDMPatcherDevice');
goog.require('ola.common.BaseUniverseTab');
goog.require('ola.common.Server');
goog.require('ola.common.UidItem');

goog.provide('ola.RDMPatcherTab');


/**
 * The tab for the RDM patcher.
 * @constructor
 */
ola.RDMPatcherTab = function(element) {
  ola.common.BaseUniverseTab.call(this, element);

  var toolbar = new goog.ui.Toolbar();
  toolbar.decorate(goog.dom.$('patcher_toolbar'));

  var autopatch_button = toolbar.getChild('autoPatchButton')
  autopatch_button.setTooltip('Automatically Patch Devices');
  /*
  goog.events.listen(discovery_button,
                     goog.ui.Component.EventType.ACTION,
                     function() { this._discoveryButtonClicked(); },
                     false,
                     this);
  */
  var refresh_button = toolbar.getChild('patcherRefreshButton')
  refresh_button.setTooltip('Refresh Devices');
  goog.events.listen(refresh_button,
                     goog.ui.Component.EventType.ACTION,
                     function() { this._update(); },
                     false,
                     this);

  toolbar.addChild(new goog.ui.ToolbarSeparator(), true);

  this.patcher = new ola.RDMPatcher('patcher_div', 'patcher_status');

  // These are devices that we know exist, but we don't have the start address
  // or footprint for.
  this.pending_devices = new Array();
  // These are devices that we have all the info for.
  this.devices = new Array();

  this.loading_div = goog.dom.createElement('div');
  this.loading_div.style.width = '100%';
  this.loading_div.style.textAlign = 'center';
  this.loading_div.innerHTML = '<img src="/loader.gif"><br>Loading...</div>'
  this.loading_div.style.marginTop = '10px';
  goog.dom.appendChild(goog.dom.$(element), this.loading_div);
};
goog.inherits(ola.RDMPatcherTab, ola.common.BaseUniverseTab);


/**
 * Set the universe for the patcher
 */
ola.RDMPatcherTab.prototype.setUniverse = function(universe_id) {
  ola.RDMPatcherTab.superClass_.setUniverse.call(this, universe_id);
  this.patcher.setUniverse(universe_id);
  this.pending_devices = new Array();
  this.patcher.setDevices(new Array());
};


/**
 * Called when the view port size changes
 */
ola.RDMPatcherTab.prototype.sizeChanged = function(frame_size) {
  ola.RDMPatcherTab.superClass_.sizeChanged.call(this, frame_size);
  // tab bar: 34, toolbar: 27, status line 16, extra: 5
  this.patcher.sizeChanged(frame_size.height - 34 - 27 - 16 - 5);
};


/**
 * Controls if we should do all the work involved in rendering the patcher.
 * This isn't needed if the patcher isn't visible.
 */
ola.RDMPatcherTab.prototype.setActive = function(state) {
  ola.RDMPatcherTab.superClass_.setActive.call(this, state);

  if (!this.isActive())
    return;
  this._update();
};


/**
 * Called when the UID list changes.
 */
ola.RDMPatcherTab.prototype._updateUidList = function(e) {
  var response = ola.common.Server.getInstance().checkForErrorLog(e);
  if (response == undefined) {
    return;
  }

  // It would be really nice to re-used the old values so we didn't have to
  // re-fetch everything but until we have some notification mechanism we can't
  // risk caching stale data.
  this.pending_devices = new Array();
  this.devices = new Array();

  var uids = e.target.getResponseJson()['uids'];
  for (var i = 0; i < uids.length; ++i) {
    this.pending_devices.push(new ola.common.UidItem(uids[i]));
  }

  if (this.isActive()) {
    this._fetchNextDeviceOrRender();
  }
};


/**
 * Fetch the information for the next device in the list, or render the patcher
 * if we've run out of devices.
 */
ola.RDMPatcherTab.prototype._fetchNextDeviceOrRender = function() {
  // fetch the new device in the list
  if (!this.pending_devices.length) {
    this.patcher.setDevices(this.devices);
    this.loading_div.style.display = 'none';
    this.patcher.update();
    return;
  }

  var server = ola.common.Server.getInstance();
  var device = this.pending_devices.shift();
  ola.logger.info('Fetching device ' + device.asString());
  var tab = this;
  server.rdmGetUIDInfo(
      this.getUniverse(),
      device.asString(),
      function(e) {
        tab._deviceInfoComplete(device, e);
      });
};


/**
 * Called when we get new information for a device
 */
ola.RDMPatcherTab.prototype._deviceInfoComplete = function(device, e) {
  if (!this.isActive()) {
    return;
  }

  var response = ola.common.Server.getInstance().checkForErrorLog(e);
  if (response == undefined) {
    this._fetchNextDeviceOrRender();
    return;
  }

  var label = device.deviceName();
  if (label) {
    label += ' [' + device.asString() + ']';
  } else {
    label = device.asString();
  }

  if (response['footprint'] > 0) {
    this.devices.push(
      new ola.RDMPatcherDevice(
        device.asString(),
        label,
        response['address'],
        response['footprint'],
        response['personality'],
        response['personality_count'])
    );
  }
  this._fetchNextDeviceOrRender();
};


/**
 * Fetch the devices and render
 */
ola.RDMPatcherTab.prototype._update = function() {
  // we've just become visible
  this.patcher.hide();
  this.loading_div.style.display = 'block';
  var server = ola.common.Server.getInstance();
  var tab = this;
  server.fetchUids(
      this.universe_id,
      function(e) { tab._updateUidList(e); });
}
