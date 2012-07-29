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
 * The DMX monitor tab.
 * Copyright (C) 2012 Simon Newton
 */

goog.require('goog.Timer');
goog.require('goog.events');

goog.require('ola.common.BaseUniverseTab');
goog.require('ola.common.Server');

goog.provide('ola.DmxMonitorTab');

ola.DmxMonitorTab.NUMBER_OF_CHANNELS = 512;
ola.DmxMonitorTab.MAX_CHANNEL_VALUE = 255;
// The time between data fetches
ola.DmxMonitorTab.PAUSE_TIME_IN_MS = 1000;

/**
 * The DMX monitor tab.
 * @constructor
 */
ola.DmxMonitorTab = function(element) {
  ola.common.BaseUniverseTab.call(this, element);
  this.value_cells = new Array();
  this.setup = false;
};
goog.inherits(ola.DmxMonitorTab, ola.common.BaseUniverseTab);


/**
 * Called when the tab changes visibiliy.
 */
ola.DmxMonitorTab.prototype.setActive = function(state) {
  ola.DmxMonitorTab.superClass_.setActive.call(this, state);

  if (this.isActive()) {
    this.setupIfRequired();
    this.fetchValues();
  }
};


/**
 * Setup the boxes if required.
 */
ola.DmxMonitorTab.prototype.setupIfRequired = function() {
  if (this.setup) {
    return;
  }

  var value_table = goog.dom.$('monitor_values');
  for (var i = 0; i < ola.DmxMonitorTab.NUMBER_OF_CHANNELS; ++i) {
    var div = goog.dom.createElement('div');
    div.innerHTML = 0;
    div.title = 'Channel ' + (i + 1);
    goog.dom.appendChild(value_table, div);
    this.value_cells.push(div);
  }
  this.setup = true;
}


/**
 * Fetches the new DMX values.
 */
ola.DmxMonitorTab.prototype.fetchValues = function(e) {
  if (!this.isActive())
    return;

  var t = this;
  ola.common.Server.getInstance().getChannelValues(
    this.getUniverse(),
    function(data) {
     t.updateData(data['dmx']);
    });
};


/**
 * Called when new data arrives.
 */
ola.DmxMonitorTab.prototype.updateData = function(data) {
  var data_length = data.length;
  for (var i = 0; i < data_length; ++i) {
    this._setCellValue(i, data[i]);
  }

  if (this.isActive()) {
    var t = this;
    goog.Timer.callOnce(
      function(data) { t.fetchValues(); },
      ola.DmxMonitorTab.PAUSE_TIME_IN_MS
    );
  }
};


/**
 * Set the value of a channel cell
 * @param {number} offset the channel offset.
 * @param {number} value the value to set the channel to.
 */
ola.DmxMonitorTab.prototype._setCellValue = function(offset, value) {
  var element = this.value_cells[offset];
  if (element == undefined) {
    return;
  }
  element.innerHTML = value;
  var remaining = ola.DmxMonitorTab.MAX_CHANNEL_VALUE - value;
  element.style.background = 'rgb(' + remaining + ',' + remaining + ',' +
    remaining + ')';
  if (value > 90) {
    element.style.color = '#ffffff';
  } else {
    element.style.color = '#000000';
  }
};
