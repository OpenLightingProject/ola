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
 * The dmx monitor tab
 * Copyright (C) 2012 Simon Newton
 */

goog.require('goog.events');
goog.require('goog.ui.Button');
goog.require('goog.ui.Container');

goog.require('ola.BaseFrame');
goog.require('ola.UniverseControl');
goog.require('ola.UniverseItem');
goog.require('ola.common.DmxMonitor');
goog.require('ola.common.SectionRenderer');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.SortedList');

goog.provide('ola.mobile.MonitorTab');


/**
 * The class representing the monitor frame
 * @constructor
 */
ola.mobile.MonitorTab = function() {
  this.monitor_frame = new ola.BaseFrame('monitor_frame');
  this.universe_frame = new ola.BaseFrame('monitor_universe_frame');
  this._hideAllFrames();
  this._resetState();

  this.ola_server = ola.common.Server.getInstance();
  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.UNIVERSE_LIST_EVENT,
                     this._updateUniverseList,
                     false, this);

  this.monitor = new ola.common.DmxMonitor(
      goog.dom.$('monitor_frame'));
};

/**
 *  The title of this tab
 */
ola.mobile.MonitorTab.prototype.title = function() {
  return 'DMX Monitor';
};


/**
 * Called when the user navigates away from this tab
 */
ola.mobile.MonitorTab.prototype.blur = function() {
  this.monitor.setState(false, undefined);
};

/**
 * Reset certain variables to their default state
 */
ola.mobile.MonitorTab.prototype._resetState = function() {
  this.universe_list = undefined;
};


/**
 * Hide all frames
 */
ola.mobile.MonitorTab.prototype._hideAllFrames = function() {
  this.monitor_frame.Hide();
  this.universe_frame.Hide();
};


/**
 * Called when the controller tab is clicked
 */
ola.mobile.MonitorTab.prototype.update = function() {
  this._hideAllFrames();
  this._resetState();

  this.universe_frame.setAsBusy();
  this.universe_frame.Show();
  this.ola_server.FetchUniversePluginList();
};


/**
 * Called when a list of universes is received
 * @param {Object} e the event object.
 */
ola.mobile.MonitorTab.prototype._updateUniverseList = function(e) {
  if (this.universe_list == undefined) {
    this.universe_frame.Clear();
    var universe_container = new goog.ui.Container();
    universe_container.render(this.universe_frame.element);

    var tab = this;
    this.universe_list = new ola.common.SortedList(
        universe_container,
        new ola.UniverseControlFactory(
          function(item) {
            tab._universeSelected(item.id(), item.name());
          }));
  }

  var items = new Array();
  for (var i = 0; i < e.universes.length; ++i) {
    var item = new ola.UniverseItem(e.universes[i]);
    items.push(item);
  }

  this.universe_list.updateFromData(items);
};


/**
 * Called when a universe is selected
 * @param {number} universe_id the id of the universe selected.
 * @param {string} universe_name the name of the universe selected.
 */
ola.mobile.MonitorTab.prototype._universeSelected = function(
    universe_id,
    universe_name) {
  this._hideAllFrames();
  this.monitor.setState(true, universe_id);
  this.monitor_frame.Show();
};
