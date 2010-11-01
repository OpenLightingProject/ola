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
goog.require('goog.ui.Checkbox')
goog.require('goog.ui.Component');
goog.require('goog.ui.Component.EventType');
goog.require('goog.ui.Control');
goog.require('goog.ui.Option');
goog.require('goog.ui.Select');
goog.require('ola.Dialog');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.SectionRenderer');

goog.provide('ola.RDMAttributesPanel');


/**
 * The class representing the panel.
 * @param {string} element_id the id of the element to use for this frame.
 * @constructor
 */
ola.RDMAttributesPanel = function(element_id, expander_button) {
  this.element = goog.dom.$(element_id);
  this.expander_button = expander_button;
  this.current_universe = undefined;
  this.current_uid = undefined;
  this.divs = new Array();
  this.zippies = new Array();
  // This holds the list of sections, and is updated as a section is loaded
  this.section_data = undefined;

  goog.events.listen(this.expander_button,
                     goog.ui.Component.EventType.ACTION,
                     function() { this._expandAllSections(); },
                     false,
                     this);
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
  var server = ola.common.Server.getInstance();
  var panel = this;
  server.rdmGetSupportedSections(
      this.current_universe,
      item.asString(),
      function(e) { panel._supportedSections(e); });
  this.current_uid = item.asString();
  this.expander_button.setEnabled(true);
};


/**
 * Clear the panel.
 */
ola.RDMAttributesPanel.prototype.clear = function() {
  this.current_uid = undefined;
  this.expander_button.setEnabled(false);
  this._setEmpty();
};


/**
 * Expand all the sections.
 * @private
 */
ola.RDMAttributesPanel.prototype._expandAllSections = function() {
  for (var i = 0; i < this.zippies.length; ++i) {
    if (!this.zippies[i].isExpanded()) {
      this.zippies[i].setExpanded(true);
      this._expandSection(i);
    }
  }
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
  this.zippies = new Array();

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
    this.zippies.push(z);

    goog.events.listen(legend,
        goog.events.EventType.CLICK,
        (function(x) {
          return function(e) {
            if (e.expanded)
              return;
            this._expandSection(x);
          }
        })(i),
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
ola.RDMAttributesPanel.prototype._expandSection = function(index) {
  if (this.section_data[index]['loaded'])
    return;

  this._loadSection(index);
};


/**
 * Load the contents for a zippy section
 * @private
 */
ola.RDMAttributesPanel.prototype._loadSection = function(index) {
  var server = ola.common.Server.getInstance();
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
    this._showErrorDialog('Error', section_response['error']);
    this.section_data[index]['loaded'] = false;
    var zippy = this.zippies[index];
    if (zippy.isExpanded()) {
      zippy.setExpanded(false);
    }
    return;
  }

  var panel = this;  // used in the onsubmit handler below
  var items = section_response['items'];
  var count = items.length;
  var form = goog.dom.createElement('form');
  form.id = this.section_data[index]['id'];
  form.onsubmit = function() { panel._saveSection(index); return false};
  var table = goog.dom.createElement('table');
  table.className = 'ola-table';
  var editable = false;

  for (var i = 0; i < count; ++i) {
    ola.common.SectionRenderer.RenderItem(table, items[i]);
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
    var text = section_response['save_button'] || 'Save';
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
      if (items[i]['type'] == 'uint') {
        // integer
        var value = form.elements[id].value;
        var int_val = parseInt(value);
        if (isNaN(int_val)) {
          this._showErrorDialog('Invalid Value',
             items[i]['description'] + ' must be an integer');
          return;
        }
        var min = items[i]['min'];
        if (min != undefined && int_val < min) {
          this._showErrorDialog('Invalid Value',
             items[i]['description'] + ' must be > ' + (min - 1));
          return;
        }
        var max = items[i]['max'];
        if (max != undefined && int_val > max) {
          this._showErrorDialog('Invalid Value',
             items[i]['description'] + ' must be < ' + (max + 1));
          return;
        }
        data += id + '=' + value + '&';
      } else if (items[i]['type'] == 'string') {
        var value = form.elements[id].value;
        data += id + '=' + value + '&';
      } else if (items[i]['type'] == 'bool') {
        var checked = items[i]['object'].isChecked();
        data += id + '=' + (checked ? '1' : '0') + '&';
      } else if (items[i]['type'] == 'select') {
        var offset = items[i]['object'].getSelectedIndex();
        var value = items[i]['value'][offset]['value'];
        data += id + '=' + value + '&';
      }
    }
  }

  var server = ola.common.Server.getInstance();
  var panel = this;
  server.rdmSetSectionInfo(
      this.current_universe,
      this.current_uid,
      this.section_data[index]['id'],
      this.section_data[index]['hint'],
      data,
      function(e) { panel._saveSectionComplete(e, index); });
};


/**
 * Called when the save is complete
 */
ola.RDMAttributesPanel.prototype._saveSectionComplete = function(e, index) {
  var response = e.target.getResponseJson();

  if (response['error']) {
    this._showErrorDialog('Set Failed', response['error']);
  } else {
    // reload data
    this._loadSection(index);
  }
}


/*
 * Show the dialog with an error message
 */
ola.RDMAttributesPanel.prototype._showErrorDialog = function(title, error) {
  var dialog = ola.Dialog.getInstance();
  dialog.setTitle(title);
  dialog.setContent(error);
  dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
  dialog.setVisible(true);
}
