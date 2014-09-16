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
 * The RDM attributes panel.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.Timer');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.AnimatedZippy');
goog.require('goog.ui.Checkbox');
goog.require('goog.ui.Component');
goog.require('goog.ui.Component.EventType');
goog.require('goog.ui.Control');
goog.require('goog.ui.Option');
goog.require('goog.ui.Select');
goog.require('ola.Dialog');
goog.require('ola.common.SectionRenderer');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');

goog.provide('ola.RDMAttributesPanel');


/**
 * The class representing the panel.
 * @param {string} element_id the id of the element to use for this frame.
 * @constructor
 */
ola.RDMAttributesPanel = function(element_id, toolbar) {
  this.element = goog.dom.$(element_id);
  this.current_universe = undefined;
  this.current_uid = undefined;
  this.divs = new Array();
  this.zippies = new Array();
  // This holds the list of sections, and is updated as a section is loaded
  this.section_data = undefined;

  this.expander_button = toolbar.getChild('showAllSectionsButton');
  this.expander_button.setTooltip('Show All Attributes');
  this.expander_button.setEnabled(false);
  goog.events.listen(this.expander_button,
                     goog.ui.Component.EventType.ACTION,
                     function() { this.expandAllSections_(); },
                     false,
                     this);

  this.collapse_button = toolbar.getChild('hideAllSectionsButton');
  this.collapse_button.setTooltip('Hide All Attributes');
  this.collapse_button.setEnabled(false);
  goog.events.listen(this.collapse_button,
                     goog.ui.Component.EventType.ACTION,
                     function() { this.hideAllSections_(); },
                     false,
                     this);

  var refresh_menu = toolbar.getChild('refreshButton');
  refresh_menu.setTooltip('Configure how often attributes are refreshed');
  goog.events.listen(refresh_menu,
                     goog.ui.Component.EventType.ACTION,
                     this.refreshChanged_,
                     false,
                     this);

  this.refresh_timer = new goog.Timer(30000);

  goog.events.listen(
      this.refresh_timer,
      goog.Timer.TICK,
      this.refreshEvent_,
      false,
      this);
};


/**
 * The mapping of strings to timeout intervals.
 */
ola.RDMAttributesPanel.RefreshInterval = {
  '30s': 30000,
  '1m': 60000,
  '5m': 300000
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
 * @param {ola.UidItem} item the uid to show.
 */
ola.RDMAttributesPanel.prototype.showUID = function(item) {
  this.setLoading_(this.element);
  var server = ola.common.Server.getInstance();
  var panel = this;
  server.rdmGetSupportedSections(
      this.current_universe,
      item.asString(),
      function(e) { panel.supportedSections_(e); });
  this.current_uid = item.asString();
  this.expander_button.setEnabled(true);
  this.collapse_button.setEnabled(true);
};


/**
 * Clear the panel.
 */
ola.RDMAttributesPanel.prototype.clear = function() {
  this.current_uid = undefined;
  this.expander_button.setEnabled(false);
  this.collapse_button.setEnabled(false);
  this.setEmpty_();
};


/**
 * Expand all the sections.
 * @private
 */
ola.RDMAttributesPanel.prototype.expandAllSections_ = function() {
  for (var i = 0; i < this.zippies.length; ++i) {
    if (!this.zippies[i].isExpanded()) {
      this.zippies[i].setExpanded(true);
      this.expandSection_(i);
    }
  }
};


/**
 * Hide all the sections.
 * @private
 */
ola.RDMAttributesPanel.prototype.hideAllSections_ = function() {
  for (var i = 0; i < this.zippies.length; ++i) {
    if (this.zippies[i].isExpanded()) {
      this.zippies[i].setExpanded(false);
    }
  }
};


/**
 * Set the refresh rate
 * @param {Object} e the event object.
 * @private
 */
ola.RDMAttributesPanel.prototype.refreshChanged_ = function(e) {
  var value = e.target.getCaption();
  if (value == 'Never') {
    this.refresh_timer.stop();
  } else {
    var timeout = ola.RDMAttributesPanel.RefreshInterval[value];
    if (timeout != undefined) {
      this.refresh_timer.setInterval(timeout);
    } else {
      ola.logger.info('Invalid timeout ' + value);
    }
    this.refresh_timer.start();
  }
};


/**
 * Called when the refresh timer fires
 * @private
 */
ola.RDMAttributesPanel.prototype.refreshEvent_ = function(e) {
  for (var i = 0; i < this.zippies.length; ++i) {
    if (this.zippies[i].isExpanded()) {
      this.loadSection_(i);
    }
  }
};


/**
 * Display the 'empty' page.
 * @private
 */
ola.RDMAttributesPanel.prototype.setEmpty_ = function() {
  this.element.innerHTML = '';
};


/**
 * Display the loading image
 * @private
 */
ola.RDMAttributesPanel.prototype.setLoading_ = function(element) {
  element.innerHTML = (
      '<div align="center"><img src="/loader.gif">' +
      '<br>Loading...</div>');
};


/**
 * Called when the supported sections request completes.
 * @param {Object} e the event object.
 * @private
 */
ola.RDMAttributesPanel.prototype.supportedSections_ = function(e) {
  this.element.innerHTML = '';
  this.divs = new Array();
  this.zippies = new Array();

  var sections = e.target.getResponseJson();
  var section_count = sections.length;
  for (var i = 0; i < section_count; ++i) {
    var section = obj[i];
    var fieldset = goog.dom.createElement('fieldset');
    var legend = goog.dom.createElement('legend');
    var image = goog.dom.createElement('img');
    image.src = '/blank.gif';
    image.width = '12';
    image.height = '12';
    goog.dom.appendChild(legend, image);
    var title = goog.dom.createTextNode(' ' + sections[i]['name']);
    goog.dom.appendChild(legend, title);
    var div = goog.dom.createElement('div');
    div.align = 'center';
    this.setLoading_(div);
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
            this.expandSection_(x);
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
ola.RDMAttributesPanel.prototype.expandSection_ = function(index) {
  if (this.section_data[index]['loaded'])
    return;

  this.loadSection_(index);
};


/**
 * Load the contents for a zippy section
 * @private
 */
ola.RDMAttributesPanel.prototype.loadSection_ = function(index) {
  var server = ola.common.Server.getInstance();
  var panel = this;
  server.rdmGetSectionInfo(
      this.current_universe,
      this.current_uid,
      this.section_data[index]['id'],
      this.section_data[index]['hint'],
      function(e) { panel.populateSection_(e, index); });
  this.section_data[index]['loaded'] = true;
};


/**
 * Populate a zippy
 * @param {Object} e the event object.
 * @param {int} index the index of the zippy to populate.
 * @private
 */
ola.RDMAttributesPanel.prototype.populateSection_ = function(e, index) {
  var section_response = e.target.getResponseJson();
  var div = this.divs[index];
  div.innerHTML = '';

  if (section_response['error']) {
    var error_title = 'Error: ' + this.section_data[index]['name'];
    this.showErrorDialog_(error_title, section_response['error']);
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
  form.onsubmit = function() { panel.saveSection_(index); return false};
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
                       function() { this.loadSection_(index) },
                       false, this);
  }

  if (editable) {
    var text = section_response['save_button'] || 'Save';
    var button = new goog.ui.CustomButton(text);
    button.render(div);

    goog.events.listen(button,
                       goog.ui.Component.EventType.ACTION,
                       function() { this.saveSection_(index) },
                       false, this);
  }

  this.section_data[index]['data'] = section_response;
};


/**
 * Save the contents of a section.
 * @private
 */
ola.RDMAttributesPanel.prototype.saveSection_ = function(index) {
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
          this.showErrorDialog_('Invalid Value',
             items[i]['description'] + ' must be an integer');
          return;
        }
        var min = items[i]['min'];
        if (min != undefined && int_val < min) {
          this.showErrorDialog_('Invalid Value',
             items[i]['description'] + ' must be > ' + (min - 1));
          return;
        }
        var max = items[i]['max'];
        if (max != undefined && int_val > max) {
          this.showErrorDialog_('Invalid Value',
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
      function(e) { panel.saveSectionComplete_(e, index); });
};


/**
 * Called when the save is complete
 * @param {Object} e the event object.
 * @private
 */
ola.RDMAttributesPanel.prototype.saveSectionComplete_ = function(e, index) {
  var response = e.target.getResponseJson();

  if (response['error']) {
    var error_title = 'Set ' + this.section_data[index]['name'] + ' Failed';
    this.showErrorDialog_(error_title, response['error']);
  } else {
    // reload data
    this.loadSection_(index);
  }
};


/**
 * Show the dialog with an error message
 * @private
 */
ola.RDMAttributesPanel.prototype.showErrorDialog_ = function(title, error) {
  var dialog = ola.Dialog.getInstance();
  dialog.setTitle(title);
  dialog.setContent(error);
  dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
  dialog.setVisible(true);
};
