/**
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The controller tab.
 * Copyright (C) 2011 Chris Stranex
 */

goog.require('goog.events');
goog.require('goog.ui.Button');
goog.require('goog.ui.Container');

goog.require('ola.BaseFrame');
goog.require('ola.UniverseControl');
goog.require('ola.UniverseItem');
goog.require('ola.common.KeypadController');
goog.require('ola.common.SectionRenderer');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.SortedList');
goog.provide('ola.mobile.ControllerTab');


/**
 * The class representing the Controller frame
 * @constructor
 */
ola.mobile.ControllerTab = function() {
  this.controller_frame = new ola.BaseFrame('controller_frame');
  this.universe_frame = new ola.BaseFrame('controller_universe_frame');
  this.hideAllFrames_();
  this.resetState_();

  this.ola_server = ola.common.Server.getInstance();
  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.UNIVERSE_LIST_EVENT,
                     this.updateUniverseList_,
                     false, this);
};


/**
 *  The title of this tab
 */
ola.mobile.ControllerTab.prototype.title = function() {
  return 'DMX Keypad';
};


/**
 * Called when the user navigates away from this tab
 */
ola.mobile.ControllerTab.prototype.blur = function() {};


/**
 * Reset certain variables to their default state
 * @private
 */
ola.mobile.ControllerTab.prototype.resetState_ = function() {
  this.universe_list = undefined;
  this.active_universe = undefined;
};


/**
 * Hide all frames
 * @private
 */
ola.mobile.ControllerTab.prototype.hideAllFrames_ = function() {
  this.controller_frame.Hide();
  this.universe_frame.Hide();
};


/**
 * Called when the controller tab is clicked
 */
ola.mobile.ControllerTab.prototype.update = function() {
  this.hideAllFrames_();
  this.resetState_();

  this.universe_frame.setAsBusy();
  this.universe_frame.Show();
  this.ola_server.FetchUniversePluginList();
};


/**
 * Called when a list of universes is received
 * @param {Object} e the event object.
 * @private
 */
ola.mobile.ControllerTab.prototype.updateUniverseList_ = function(e) {
  if (this.universe_list == undefined) {
    this.universe_frame.Clear();
    var universe_container = new goog.ui.Container();
    universe_container.render(this.universe_frame.element);

    var tab = this;
    this.universe_list = new ola.common.SortedList(
        universe_container,
        new ola.UniverseControlFactory(
          function(item) {
            tab.universeSelected_(item.id(), item.name());
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
 * @private
 */
ola.mobile.ControllerTab.prototype.universeSelected_ = function(
    universe_id,
    universe_name) {
  this.hideAllFrames_();
  this.active_universe = universe_id;

  this.keypad = new ola.common.KeypadController(
    universe_name,
    universe_id);
  this.controller_frame.Clear();
  this.controller_frame.element.appendChild(this.keypad.table);
  this.controller_frame.Show();
};
