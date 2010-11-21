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
 * The home frame.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.Component');
goog.require('goog.ui.CustomButton');
goog.require('ola.BaseFrame');
goog.require('ola.Dialog');
goog.require('ola.LoggerWindow');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.ServerStats');
goog.require('ola.common.SortedList');
goog.require('ola.UniverseItem');

goog.provide('ola.HomeFrame');


/**
 * A container that uses the tbody element
 * @constructor
 */
ola.TableContainer = function(opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
};
goog.inherits(ola.TableContainer, goog.ui.Component);


/**
 * Create the dom for the TableContainer
 */
ola.TableContainer.prototype.createDom = function(container) {
  this.decorateInternal(this.dom_.createElement('tbody'));
};


/**
 * Decorate an existing element
 */
ola.TableContainer.prototype.decorateInternal = function(element) {
  ola.TableContainer.superClass_.decorateInternal.call(this, element);
};


/**
 * Check if we can decorate an element.
 * @param {Element} element the dom element to check.
 */
ola.TableContainer.prototype.canDecorate = function(element) {
  return element.tagName == 'TBODY';
};


/**
 * A line in the active universe list.
 * @param {ola.UniverseItem} universe_item the item to use for this row.
 * @constructor
 */
ola.UniverseRow = function(universe_item, opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
  this._item = universe_item;
};
goog.inherits(ola.UniverseRow, goog.ui.Component);


/**
 * Return the underlying UniverseItem
 * @return {ola.UniverseItem}
 */
ola.UniverseRow.prototype.item = function() { return this._item; };


/**
 * This component can't be used to decorate
 */
ola.UniverseRow.prototype.canDecorate = function() { return false; };


/**
 * Create the dom for this component
 */
ola.UniverseRow.prototype.createDom = function() {
  var tr = this.dom_.createDom(
      'tr', {},
      goog.dom.createDom('td', {}, this._item.id().toString()),
      goog.dom.createDom('td', {}, this._item.name()),
      goog.dom.createDom('td', {}, this._item.inputPortCount().toString()),
      goog.dom.createDom('td', {}, this._item.outputPortCount().toString()),
      goog.dom.createDom('td', {}, this._item.rdmDeviceCount().toString()));
  this.setElementInternal(tr);
};


/**
 * Update this item with from new data
 * @param {ola.UniverseItem} universe_item the new item to update from.
 */
ola.UniverseRow.prototype.update = function(universe_item) {
  var element = this.getElement();
  var td = goog.dom.getFirstElementChild(element);
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = universe_item.name();
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = universe_item.inputPortCount().toString();
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = universe_item.outputPortCount().toString();
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = universe_item.rdmDeviceCount().toString();
};


/**
 * The base class for a factory which produces UniverseRows
 * @constructor
 */
ola.UniverseRowFactory = function() {};


/**
 * @return {ola.UniverseRow} an instance of a UniverseRow.
 */
ola.UniverseRowFactory.prototype.newComponent = function(data) {
  return new ola.UniverseRow(data);
};


/**
 * A class representing the home frame
 * @param {string} element_id the id of the div to use for the home frame.
 * @constructor
 */
ola.HomeFrame = function(element_id) {
  var ola_server = ola.common.Server.getInstance();
  ola.BaseFrame.call(this, element_id);

  var reload_button = goog.dom.$('reload_button');
  goog.ui.decorate(reload_button);
  goog.events.listen(reload_button,
                     goog.events.EventType.CLICK,
                     this._reloadButtonClicked,
                     false, this);

  var stop_button = goog.dom.$('stop_button');
  goog.ui.decorate(stop_button);
  goog.events.listen(stop_button,
                     goog.events.EventType.CLICK,
                     this._stopButtonClicked,
                     false, this);

  var new_universe_button = goog.dom.$('new_universe_button');
  goog.ui.decorate(new_universe_button);

  goog.events.listen(ola_server, ola.common.Server.EventType.UNIVERSE_LIST_EVENT,
                     this._universeListChanged,
                     false, this);

  this.server_stats = new ola.common.ServerStats();

  var table_container = new ola.TableContainer();
  table_container.decorate(goog.dom.$('active_universe_list'));
  this.universe_list = new ola.common.SortedList(table_container,
                                          new ola.UniverseRowFactory());
};
goog.inherits(ola.HomeFrame, ola.BaseFrame);


/**
 * Update the universe set
 */
ola.HomeFrame.prototype._universeListChanged = function(e) {
  var items = new Array();
  for (var i = 0; i < e.universes.length; ++i) {
    items.push(new ola.UniverseItem(e.universes[i]));
  }
  this.universe_list.updateFromData(items);
};


/**
 * Called when the stop button is clicked
 * @private
 */
ola.HomeFrame.prototype._stopButtonClicked = function(e) {
  var dialog = ola.Dialog.getInstance();

  goog.events.listen(dialog, goog.ui.Dialog.EventType.SELECT,
      this._stopServerConfirmed, false, this);

  dialog.setTitle('Please confirm');
  dialog.setButtonSet(goog.ui.Dialog.ButtonSet.YES_NO);
  dialog.setContent(
      'Are you sure? OLA may not be configured to restart automatically');
  dialog.setVisible(true);
};


/**
 * Called when the stop dialog exits.
 * @private
 */
ola.HomeFrame.prototype._stopServerConfirmed = function(e) {
  var dialog = ola.Dialog.getInstance();

  goog.events.unlisten(dialog, goog.ui.Dialog.EventType.SELECT,
      this._stopServerConfirmed, false, this);

  if (e.key == goog.ui.Dialog.DefaultButtonKeys.YES) {
    dialog.setAsBusy();
    dialog.setVisible(true);
    var frame = this;
    ola.common.Server.getInstance().stopServer(
      function(e) { frame._stopServerComplete(e); });
    return false;
  }
};


/**
 * Update the home frame with new server data
 * @private
 */
ola.HomeFrame.prototype._stopServerComplete = function(e) {
  var dialog = ola.Dialog.getInstance();
  if (e.target.getStatus() == 200) {
    dialog.setVisible(false);
  } else {
    dialog.setTitle('Failed to stop the server');
    dialog.setContent(e.target.getLastUri() + ' : ' + e.target.getLastError());
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
  }
};


/**
 * Called when the reload button is clicked
 * @private
 */
ola.HomeFrame.prototype._reloadButtonClicked = function(e) {
  var dialog = ola.Dialog.getInstance();
  dialog.setAsBusy();
  dialog.setVisible(true);
  var frame = this;
  ola.common.Server.getInstance().reloadPlugins(
      function(e) { frame._pluginReloadComplete(e); });
};


/**
 * Update the home frame with new server data
 * @private
 */
ola.HomeFrame.prototype._pluginReloadComplete = function(e) {
  var dialog = ola.Dialog.getInstance();
  if (e.target.getStatus() == 200) {
    dialog.setVisible(false);
  } else {
    dialog.setTitle('Failed to Reload plugins');
    dialog.setContent(e.target.getLastUri() + ' : ' + e.target.getLastError());
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
  }
};
