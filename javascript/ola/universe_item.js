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
 * Object that represent a universe.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('ola.common.DataItem');

goog.provide('ola.UniverseItem');


/**
 * An object which represents a universe in the list
 * @param {Object} data the data to use to construct this item.
 * @constructor
 */
ola.UniverseItem = function(data) {
  this._id = data['id'];
  this._name = data['name'];
  this._input_ports = data['input_ports'];
  this._output_ports = data['output_ports'];
  this._rdm_devices = data['rdm_devices'];
};
goog.inherits(ola.UniverseItem, ola.common.DataItem);


/**
 * Get the id of this universe.
 * @return {number} the universe id.
 */
ola.UniverseItem.prototype.id = function() { return this._id; };


/**
 * Return the universe name
 * @return {string} the name.
 */
ola.UniverseItem.prototype.name = function() { return this._name; };


/**
 * Return the number of input ports
 * @return {number} the number of input ports.
 */
ola.UniverseItem.prototype.inputPortCount = function() {
  return this._input_ports;
};


/**
 * Return the number of output ports
 * @return {number} the number of output ports.
 */
ola.UniverseItem.prototype.outputPortCount = function() {
  return this._output_ports;
};


/**
 * Return the number of RDm devices.
 * @return {number} the number of RDM devices.
 */
ola.UniverseItem.prototype.rdmDeviceCount = function() {
  return this._rdm_devices;
};
