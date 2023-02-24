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
 * The DMX monitor.
 * This calls a method when new DMX data arrives.
 * Copyright (C) 2012 Simon Newton
 */

goog.require('goog.Timer');
goog.require('goog.events');
goog.require('ola.common.DmxConstants');
goog.require('ola.common.Server');

goog.provide('ola.common.DmxMonitor');


/**
 * The DMX monitor tab.
 * @constructor
 */
ola.common.DmxMonitor = function(container) {
  this.container = container;
  this.value_cells = new Array();
  this.setup = false;
  this.enabled = false;
  this.universe_id = undefined;
};


/** The time between data fetches @type {number} */
ola.common.DmxMonitor.PAUSE_TIME_IN_MS = 1000;


/**
 * Enable / Disable the monitor.
 * @param {boolean} enabled true to enable, false to disable.
 * @param {number} universe_id the universe to use.
 */
ola.common.DmxMonitor.prototype.setState = function(enabled,
                                                    universe_id) {
  this.enabled = enabled;
  this.universe_id = universe_id;

  if (this.enabled) {
    if (!this.setup) {
      this.setupCells();
    }
    this.fetchValues();
  }
};


/**
 * Setup the boxes if required.
 */
ola.common.DmxMonitor.prototype.setupCells = function() {
  for (var i = 0; i < ola.common.DmxConstants.MAX_CHANNEL_NUMBER; ++i) {
    var cell = goog.dom.createElement('div');
    cell.title = 'Channel ' + (i + 1);
    var channel = goog.dom.createElement('div');
    channel.innerHTML = i + 1;
    var span = goog.dom.createElement('span');
    span.innerHTML = '&nbsp';
    goog.dom.appendChild(cell, channel);
    goog.dom.appendChild(cell, span);
    goog.dom.appendChild(this.container, cell);
    this.value_cells.push(span);
  }
  this.setup = true;
};


/**
 * Fetches the new DMX values.
 */
ola.common.DmxMonitor.prototype.fetchValues = function(e) {
  if (!this.enabled)
    return;

  var t = this;
  ola.common.Server.getInstance().getChannelValues(
    this.universe_id,
    function(data) {
     t.updateData(data['dmx']);
    });
};


/**
 * Called when new data arrives.
 */
ola.common.DmxMonitor.prototype.updateData = function(data) {
  var data_length = Math.min(ola.common.DmxConstants.MAX_CHANNEL_NUMBER,
                             data.length);
  for (var i = 0; i < data_length; ++i) {
    this.setCellValue_(i, data[i]);
  }

  for (var i = data_length; i < ola.common.DmxConstants.MAX_CHANNEL_NUMBER;
       ++i) {
    this.clearCellValue_(i);
  }

  if (this.enabled) {
    var t = this;
    goog.Timer.callOnce(
      function(data) { t.fetchValues(); },
      ola.common.DmxMonitor.PAUSE_TIME_IN_MS
    );
  }
};


/**
 * Set the value of a channel cell
 * @param {number} offset the channel offset.
 * @param {number} value the value to set the channel to.
 * @private
 */
ola.common.DmxMonitor.prototype.setCellValue_ = function(offset, value) {
  var element = this.value_cells[offset];
  if (element == undefined) {
    return;
  }
  element.innerHTML = value;
  var remaining = ola.common.DmxConstants.MAX_CHANNEL_VALUE - value;
  element.style.background = 'rgb(' + remaining + ',' + remaining + ',' +
    remaining + ')';
  if (value > ola.common.DmxConstants.BACKGROUND_CHANGE_CHANNEL_LEVEL) {
    element.style.color = '#ffffff';
  } else {
    element.style.color = '#000000';
  }
};


/**
 * Erase a cell value to indicate we didn't get data.
 * @param {number} offset the channel offset.
 * @private
 */
ola.common.DmxMonitor.prototype.clearCellValue_ = function(offset) {
  var element = this.value_cells[offset];
  if (element == undefined) {
    return;
  }
  element.innerHTML = '&nbsp;';
  element.style.background = '#ffffff';
};
