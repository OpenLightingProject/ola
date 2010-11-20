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

goog.require('goog.dom');
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
  this.patcher = new ola.RDMPatcher(element);

  // These are devices that we know exist, but we don't have the start address
  // or footprint for. We only fetch that information when the patcher becomes
  // visible.
  this.pending_devices = new Array();

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
  this.pending_devices = new Array();
  this.patcher.reset();
};


/**
 * Called when the view port size changes
 */
ola.RDMPatcherTab.prototype.sizeChanged = function(frame_size) {
  ola.RDMPatcherTab.superClass_.sizeChanged.call(this, frame_size);
  this.patcher.sizeChanged();
};


/**
 * Controls if we should do all the work involved in rendering the patcher.
 * This isn't needed if the patcher isn't visible.
 */
ola.RDMPatcherTab.prototype.setActive = function(state) {
  ola.RDMPatcherTab.superClass_.setActive.call(this, state);

  if (!this.isActive())
    return;

  // we've just become visible
  this.patcher.hide();
  this.loading_div.style.display = 'block';
  var server = ola.common.Server.getInstance();
  var tab = this;
  server.fetchUids(
      this.universe_id,
      function(e) { tab._updateUidList(e); });
};


/**
 * Called when the UID list changes.
 */
ola.RDMPatcherTab.prototype._updateUidList = function(e) {
  if (e.target.getStatus() != 200) {
    ola.logger.info('Request failed: ' + e.target.getLastUri() + ' : ' +
        e.target.getLastError());
    return;
  }

  var obj = e.target.getResponseJson();
  var uids = obj['uids'];

  ola.logger.info('got uids');

  var items = new Array();
  for (var i = 0; i < uids.length; ++i) {
    items.push(new ola.common.UidItem(uids[i]));
  }

  // add anything in items but not in the pending list
  items.sort(function(a, b) { return a.compare(b); });
  this.pending_devices.sort(function(a, b) { return a.compare(b); });

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
    //TODO(simon): we could be dragging or in a dialog at this point, so we
    // don't want to force a render.
    ola.logger.info('updating');
    this.patcher.update();
    return;
  }

  ola.logger.info('fetching device');
  var ola_server = ola.common.Server.getInstance();

};


/**
 * Called when we get new information for a device
 */
ola.RDMPatcherTab.prototype._deviceInfoComplete = function(e) {
  if (!this.isActive()) {
    return;
  }

  // add to the patcher

  this._fetchNextDeviceOrRender();
};
