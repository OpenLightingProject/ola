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
 * The RDM attributes panel.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.AnimatedZippy');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.Zippy');
goog.require('ola.Dialog');
goog.require('ola.Server');
goog.require('ola.Server.EventType');

goog.provide('ola.RDMAttributesPanel');


/**
 * The class representing the panel.
 * @param {string} element_id the id of the element to use for this frame.
 * @constructor
 */
ola.RDMAttributesPanel = function(element_id) {
  this.element = goog.dom.$(element_id);
  this.current_universe = undefined;
  this.current_uid = undefined;
  this.divs = new Array();
  this.section_data = undefined;
};


/**
 * Change the current universe.
 * @param {number} universe_id the current universe.
 */
ola.RDMAttributesPanel.prototype.updateUniverse = function(universe_id) {
  this.current_universe = universe_id;
  this.current_uid = undefined;
}


/**
 * Show the attributes for a particular UID.
 * @param {ola.UidItem} item the uid to show
 */
ola.RDMAttributesPanel.prototype.showUID = function(item) {
  this._setLoading();
  var server = ola.Server.getInstance();
  var panel = this;
  server.rdmGetSupportedSections(
      this.current_universe,
      item.asString(),
      function(e) { panel._supportedSections(e); });
  this.current_uid = item.asString();
};


/**
 * Clear the panel.
 */
ola.RDMAttributesPanel.prototype.clear = function() {
  this.current_uid = undefined;
  this._setEmpty();
};


/**
 * Display the 'empty' page.
 * @private
 */
ola.RDMAttributesPanel.prototype._setEmpty = function() {
  this.element.innerHTML = '';
};


/**
 * Display the loading image
 * @private
 */
ola.RDMAttributesPanel.prototype._setLoading = function() {
  this.element.innerHTML = (
      '<div style="margin-top: 30px" align="center"><img src="/loader.gif">' +
      '<br>Loading...</div>');
};


/**
 * Called when the supported sections request completes.
 * @private
 */
ola.RDMAttributesPanel.prototype._supportedSections = function(e) {
  this.element.innerHTML = '';
  this.divs = new Array();

  var sections = e.target.getResponseJson();
  var section_count = sections.length;
  for (var i = 0; i < section_count; ++i) {
    var section = obj[i]
    var fieldset = goog.dom.createElement('fieldset');
    var legend = goog.dom.createElement('legend');
    legend.innerHTML = sections[i]['name'];
    var div = goog.dom.createElement('div');
    div.innerHTML = '';
    goog.dom.appendChild(fieldset, legend);
    goog.dom.appendChild(fieldset, div);
    goog.dom.appendChild(this.element, fieldset);
    var z = new goog.ui.AnimatedZippy(legend, div);
    this.divs.push(div);

    goog.events.listen(z,
        goog.ui.Zippy.Events.TOGGLE,
        (function(x) {
          return function(e) { this._expandZippy(e, x); } }
        )(i),
        false,
        this);
  }
  this.section_data = sections;
};


/**
 * Called when one of the zippies is expanded
 */
ola.RDMAttributesPanel.prototype._expandZippy = function(e, i) {
  if (!e.expanded)
    return;

  var server = ola.Server.getInstance();
  var panel = this;
  server.rdmGetSectionInfo(
      this.current_universe,
      this.current_uid,
      this.section_data[i]['id'],
      this.section_data[i]['hint'],
      function(e) { panel._populateZippy(e, i); });
};


/**
 * Populate a zippy
 */
ola.RDMAttributesPanel.prototype._populateZippy = function(e, i) {
  var section_info = e.target.getResponseJson();
  var count = section_info.length;
  var div = this.divs[i];
  div.innerHTML = '';
  var table = goog.dom.createElement('table');
  table.className = 'ola-table';
  for (var i = 0; i < count; ++i) {
    var tr = goog.dom.createElement('tr');
    var name_td = goog.dom.createElement('td');
    name_td.innerHTML = section_info[i]['name'];
    var td = goog.dom.createElement('td');
    td.innerHTML = section_info[i]['value'];
    // switch based on type here

    goog.dom.appendChild(tr, name_td);
    goog.dom.appendChild(tr, td);
    goog.dom.appendChild(table, tr);
  }
  goog.dom.appendChild(div, table);
};
