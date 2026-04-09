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
 * The DMX console tab.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.Timer');
goog.require('goog.events');

goog.require('ola.DmxConsole');
goog.require('ola.common.BaseUniverseTab');
goog.require('ola.common.Server');

goog.provide('ola.DmxConsoleTab');


/**
 * The DMX console tab.
 * @constructor
 */
ola.DmxConsoleTab = function(element) {
  ola.common.BaseUniverseTab.call(this, element);

  // setup the console
  this.dmx_console = new ola.DmxConsole();
  this.tick_timer = new goog.Timer(1000);
  this.mute_events = true;

  goog.events.listen(
      this.tick_timer,
      goog.Timer.TICK,
      this.consoleChanged_,
      false,
      this);

  goog.events.listen(
      this.dmx_console,
      ola.DmxConsole.CHANGE_EVENT,
      this.consoleChanged_,
      false,
      this);
};
goog.inherits(ola.DmxConsoleTab, ola.common.BaseUniverseTab);


/**
 * Set the universe.
 */
ola.DmxConsoleTab.prototype.setUniverse = function(universe_id) {
  ola.DmxConsoleTab.superClass_.setUniverse.call(this, universe_id);
  this.dmx_console.resetConsole();
};


/**
 * Called when the tab changes visibility.
 */
ola.DmxConsoleTab.prototype.setActive = function(state) {
  ola.DmxConsoleTab.superClass_.setActive.call(this, state);

  if (this.isActive()) {
    this.mute_events = true;
    this.dmx_console.setupIfRequired();
    this.dmx_console.update();
    this.loadValues();
  } else {
    this.tick_timer.stop();
  }
};


/**
 * Fetches the new DMX values.
 */
ola.DmxConsoleTab.prototype.loadValues = function(e) {
  var t = this;
  ola.common.Server.getInstance().getChannelValues(
    this.getUniverse(),
    function(data) {
     t.newValues(data['dmx']);
    });
};


/**
 * Update the console with the new values
 */
ola.DmxConsoleTab.prototype.newValues = function(data) {
  this.dmx_console.setData(data);
  this.mute_events = false;
  if (this.isActive())
    this.tick_timer.start();
};


/**
 * Called when the console values change
 * @private
 */
ola.DmxConsoleTab.prototype.consoleChanged_ = function(e) {
  if (this.mute_events) {
    return;
  }
  this.mute_events = true;

  var data = this.dmx_console.getData();
  var t = this;
  ola.common.Server.getInstance().setChannelValues(
      this.getUniverse(),
      data,
      function(e) {
        t.mute_events = false;
      });
};
