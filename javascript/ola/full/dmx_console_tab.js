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

  goog.events.listen(
      this.tick_timer,
      goog.Timer.TICK,
      this._consoleChanged,
      false,
      this);

  goog.events.listen(
      this.dmx_console,
      ola.DmxConsole.CHANGE_EVENT,
      this._consoleChanged,
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
 * Called when the tab changes visibiliy.
 */
ola.DmxConsoleTab.prototype.setActive = function(state) {
  ola.DmxConsoleTab.superClass_.setActive.call(this, state);

  if (this.isActive()) {
    this.dmx_console.setupIfRequired();
    this.dmx_console.update();
    this.tick_timer.start();
  } else {
    this.tick_timer.stop();
  }
};


/**
 * Called when the console values change
 */
ola.DmxConsoleTab.prototype._consoleChanged = function(e) {
  var data = this.dmx_console.getData();
  ola.common.Server.getInstance().setChannelValues(this.getUniverse(), data);
};
