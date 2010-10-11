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
goog.require('goog.ui.Component.EventType');
goog.require('goog.ui.AnimatedZippy');
goog.require('goog.ui.Component');
goog.require('goog.ui.Control');
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
  // This holds the list of sections, and is updated as a section is loaded
  this.section_data = undefined;
};


/**
 * Change the current universe.
 * @param {number} universe_id the current universe.
 */
ola.RDMAttributesPanel.prototype.updateUniverse = function(universe_id) {
  this.current_universe = universe_id;
  this.current_uid = undefined;
};


/**
 * Show the attributes for a particular UID.
 * @param {ola.UidItem} item the uid to show
 */
ola.RDMAttributesPanel.prototype.showUID = function(item) {
  this._setLoading(this.element);
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
ola.RDMAttributesPanel.prototype._setLoading = function(element) {
  element.innerHTML = (
      '<div align="center"><img src="/loader.gif">' +
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
    var image = goog.dom.createElement('img');
    image.src = '/blank.gif';
    image.width = "12";
    image.height = "12";
    goog.dom.appendChild(legend, image);
    var title = goog.dom.createTextNode(' ' + sections[i]['name']);
    goog.dom.appendChild(legend, title);
    var div = goog.dom.createElement('div');
    div.align = 'center';
    this._setLoading(div);
    goog.dom.appendChild(fieldset, legend);
    goog.dom.appendChild(fieldset, div);
    goog.dom.appendChild(this.element, fieldset);
    var z = new goog.ui.AnimatedZippy(legend, div);
    this.divs.push(div);

    goog.events.listen(z,
        goog.ui.Zippy.Events.TOGGLE,
        (function(x) {
          return function(e) { this._expandSection(e, x); } }
        )(i),
        false,
        this);

    sections['data'] = undefined;
    sections['loaded'] = false;
  }
  this.section_data = sections;
};


/**
 * Called when one of the zippies is expanded
 * @private
 */
ola.RDMAttributesPanel.prototype._expandSection = function(e, index) {
  if (!e.expanded)
    return;

  if (this.section_data[index]['loaded'])
    return;

  this._loadSection(index);
};


/**
 * Load the contents for a zippy section
 * @private
 */
ola.RDMAttributesPanel.prototype._loadSection = function(index) {
  var server = ola.Server.getInstance();
  var panel = this;
  server.rdmGetSectionInfo(
      this.current_universe,
      this.current_uid,
      this.section_data[index]['id'],
      this.section_data[index]['hint'],
      function(e) { panel._populateSection(e, index); });
  this.section_data[index]['loaded'] = true;
};


/**
 * Populate a zippy
 * @param {int} index the index of the zippy to populate.
 * @private
 */
ola.RDMAttributesPanel.prototype._populateSection = function(e, index) {
  var section_response = e.target.getResponseJson();
  var div = this.divs[index];
  div.innerHTML = '';

  if (section_response['error']) {
    alert(section_response['error']);
    return;
  }

  var items = section_response['items'];
  var count = items.length;
  var form = goog.dom.createElement('form');
  form.id = this.section_data[index]['id'];
  var table = goog.dom.createElement('table');
  table.className = 'ola-table';
  var editable = false;

  for (var i = 0; i < count; ++i) {
    this._buildItem(table, items[i]);
    // if any field has an id and doesn't have it's own button we display the
    // save button.
    editable |= (items[i]['id'] && !items[i]['button']);
  }
  goog.dom.appendChild(form, table);
  goog.dom.appendChild(div, form);

  if (section_response['refresh']) {
    var button = new goog.ui.CustomButton('Refresh');
    button.render(div);

    goog.events.listen(button,
                       goog.ui.Component.EventType.ACTION,
                       function() { this._loadSection(index) },
                       false, this);
  }

  if (editable) {
    var text = section_response['button'] || 'Save';
    var button = new goog.ui.CustomButton(text);
    button.render(div);

    goog.events.listen(button,
                       goog.ui.Component.EventType.ACTION,
                       function() { this._saveSection(index) },
                       false, this);
  }

  this.section_data[index]['data'] = section_response;
};


/**
 * Generate the html for an item
 * @param {Element} table the table object to add the row to.
 * @param {Object} item_info the data for the item.
 * @private
 */
ola.RDMAttributesPanel.prototype._buildItem = function(table, item_info) {
  var type = item_info['type'];
  var value = item_info['value'];
  var id = item_info['id'];

  if (type == 'hidden' && !id) {
    // we don't need a new row here
    var input = goog.dom.createElement('input');
    input.id = id;
    input.type = 'hidden';
    input.value = value;
    goog.dom.appendChild(table, input);
    return;
  }

  // everything else needs a row
  var row = goog.dom.createElement('tr');
  goog.dom.appendChild(table, row);
  var name_td = goog.dom.createElement('td');
  name_td.innerHTML = item_info['description'];
  goog.dom.appendChild(row, name_td);
  var td = goog.dom.createElement('td');
  goog.dom.appendChild(row, td);

  if (id) {
    // id implies this field is editable
    if (type == 'string' || type == 'uint' || type == 'hidden') {
      var input = goog.dom.createElement('input');
      input.value = value;
      input.name = id;
      if (type == 'hidden') {
        input.type = 'hidden';
      }
      goog.dom.appendChild(td, input);

      if (item_info['button']) {
        // this item get's it's own button
        var button = new goog.ui.CustomButton(item_info['button']);
        button.render(td);
      }
    } else {
      // select box
    }
  } else {
    td.innerHTML = value;
  }

};


/**
 * Save the contents of a section.
 */
ola.RDMAttributesPanel.prototype._saveSection = function(index) {
  var items = this.section_data[index]['data']['items'];
  var count = items.length;

  var form = goog.dom.$(this.section_data[index]['id']);

  var data = '';
  for (var i = 0; i < count; ++i) {
    var id = items[i]['id'];
    if (id) {
      var value = form.elements[id].value;
      data += id + '=' + value + '&';
    }
  }

  var server = ola.Server.getInstance();
  var panel = this;
  server.rdmSetSectionInfo(
      this.current_universe,
      this.current_uid,
      this.section_data[index]['id'],
      data,
      function(e) { panel._saveSectionComplete(e, index); });
};


/**
 * Called when the save is complete
 */
ola.RDMAttributesPanel.prototype._saveSectionComplete = function(e, index) {
  var response = e.target.getResponseJson();

  if (response['error']) {
    var dialog = ola.Dialog.getInstance();
    dialog.setTitle('Set Failed');
    dialog.setContent(response['error']);
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
  } else {
    // reload data
    this._loadSection(index);
  }
}
