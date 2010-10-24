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
 * The universe tab.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.events');
goog.require('goog.ui.Button');
goog.require('goog.ui.Container');
//goog.require('goog.ui.Select');
//goog.require('goog.ui.CustomButton');

goog.require('ola.BaseFrame');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.SortedList');
goog.require('ola.UniverseControl');
goog.require('ola.UniverseItem');
goog.require('ola.UidItem');
goog.require('ola.UidControl');
goog.require('ola.UidControlFactory');
goog.require('ola.RdmSectionItem');
goog.require('ola.RdmSectionControl');
goog.require('ola.RdmSectionControlFactory');

goog.provide('ola.mobile.UniverseTab');


/**
 * The class representing the Universe frame
 * @param {string} element_id the id of the element to use for this frame.
 * @constructor
 */
ola.mobile.UniverseTab = function() {
  this.universe_frame = new ola.BaseFrame('universe_frame');
  this.uid_frame = new ola.BaseFrame('uid_frame');
  this.rdm_frame = new ola.BaseFrame('rdm_frame');
  this.rdm_section_frame = new ola.BaseFrame('rdm_section_frame');

  this._hideAllFrames();

  this.universe_list = undefined;
  this.active_universe = undefined;
  this.uid_list = undefined;
  this.active_uid = undefined;
  this.rdm_list = undefined;
  this.active_section = undefined;
  this.items = undefined;

  this.ola_server = ola.common.Server.getInstance();
  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.UNIVERSE_LIST_EVENT,
                     this._updateUniverseList,
                     false, this);

  goog.events.listen(this.ola_server,
                     ola.common.Server.EventType.UIDS_EVENT,
                     this._updateUidList,
                     false, this);
};


/**
 * Hide all frames
 */
ola.mobile.UniverseTab.prototype._hideAllFrames = function() {
  this.universe_frame.Hide();
  this.uid_frame.Hide();
  this.rdm_frame.Hide();
  this.rdm_section_frame.Hide();
};


/**
 * Caled when the universe tab is clicked
 */
ola.mobile.UniverseTab.prototype.update = function() {
  this._hideAllFrames();
  this.universe_frame.SetAsBusy();
  this.universe_list = undefined;
  this.active_universe = undefined;
  this.active_uid = undefined;
  this.active_section = undefined;
  this.universe_frame.Show();
  this.ola_server.FetchUniversePluginList();
};


/**
 * Called when a new list of universes is received.
 */
ola.mobile.UniverseTab.prototype._updateUniverseList = function(e) {
  if (this.universe_list == undefined) {
    this.universe_frame.Clear();
    var universe_container = new goog.ui.Container();
    universe_container.render(this.universe_frame.element);

    var tab = this;
    this.universe_list = new ola.common.SortedList(
        universe_container,
        new ola.UniverseControlFactory(
          function(item) { tab._universeSelected(item.id()); }));
  }

  var items = new Array();
  for (var i = 0; i < e.universes.length; ++i) {
    var item = new ola.UniverseItem(e.universes[i]);
    items.push(item);
  }
  this.universe_list.updateFromData(items);
};


/**
 * Called when a universe is selected
 */
ola.mobile.UniverseTab.prototype._universeSelected = function(universe_id) {
  this._hideAllFrames();
  this.uid_frame.SetAsBusy();
  this.uid_list = undefined;
  this.rdm_list = undefined;
  this.active_universe = universe_id;
  this.uid_frame.Show();
  this.ola_server.FetchUids(universe_id);

  // setup a timer here
};


/**
 * Called when a new list of uids is received.
 */
ola.mobile.UniverseTab.prototype._updateUidList = function(e) {
  if (this.uid_list == undefined) {
    this.uid_frame.Clear();

    var uid_container = new goog.ui.Container();
    uid_container.render(this.uid_frame.element);

    var tab = this;
    this.uid_list = new ola.common.SortedList(
        uid_container,
        new ola.UidControlFactory(
          function(item) { tab._uidSelected(item.id()); }));

    var button = new goog.ui.Button('Back');
    button.render(this.uid_frame.element);

    goog.events.listen(button,
                       goog.ui.Component.EventType.ACTION,
                       function() { this.update() },
                       false, this);
  }

  var items = new Array();
  for (var i = 0; i < e.uids.length; ++i) {
    items.push(new ola.UidItem(e.uids[i]));
  }
  this.uid_list.updateFromData(items);
};


/**
 * Called when a uid is selected
 */
ola.mobile.UniverseTab.prototype._uidSelected = function(uid) {
  this._hideAllFrames();
  this.rdm_frame.SetAsBusy();
  this.rdm_list = undefined;
  this.active_uid = uid;
  this.rdm_frame.Show();

  var tab = this;
  this.ola_server.rdmGetSupportedSections(
      this.active_universe,
      uid,
      function(e) { tab._updateSupportedSections(e); });
};


/**
 * Called when a list of supported sections is received.
 */
ola.mobile.UniverseTab.prototype._updateSupportedSections = function(e) {
  if (this.rdm_list == undefined) {
    this.rdm_frame.Clear();

    var rdm_container = new goog.ui.Container();
    rdm_container.render(this.rdm_frame.element);

    var tab = this;
    this.rdm_list = new ola.common.SortedList(
        rdm_container,
        new ola.RdmSectionControlFactory(
          function(item) { tab._sectionSelected(item); }));

    var button = new goog.ui.Button('Back');
    button.render(this.rdm_frame.element);

    goog.events.listen(
        button,
        goog.ui.Component.EventType.ACTION,
        function() { this._universeSelected(this.active_universe) },
        false, this);
  }

  var sections = e.target.getResponseJson();
  var section_count = sections.length;
  var items = new Array();
  for (var i = 0; i < section_count; ++i) {
    items.push(new ola.RdmSectionItem(sections[i]));
  }
  this.rdm_list.updateFromData(items);
};


/**
 * Called when a section is selected
 */
ola.mobile.UniverseTab.prototype._sectionSelected = function(section) {
  this._hideAllFrames();
  this.rdm_section_frame.SetAsBusy();
  this.rdm_section_frame.Show();
  this.active_section = section;
  this._loadSection();
};


/**
 * Called when we need to load a section
 */
ola.mobile.UniverseTab.prototype._loadSection = function() {
  var tab = this;

  this.ola_server.rdmGetSectionInfo(
      this.active_universe,
      this.active_uid,
      this.active_section.id(),
      this.active_section.hint(),
      function(e) { tab._updateSection(e); });
};


/**
 * Called when a section is ready to be drawn
 */
ola.mobile.UniverseTab.prototype._updateSection = function(e) {
  var section_response = e.target.getResponseJson();

  this.rdm_section_frame.Clear();
  var div= this.rdm_section_frame.element;
  div.innerHTML = '';

  if (section_response['error']) {
    div.innerHTML = section_response['error'];
    return;
  }

  var items = section_response['items'];
  var count = items.length;
  var form = goog.dom.createElement('form');
  form.id = this.active_section.id();
  var tab = this;
  form.onsubmit = function() { tab._saveSection(); return false};
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

  var button = new goog.ui.Button('Back');
  button.render(div);

  goog.events.listen(button,
                     goog.ui.Component.EventType.ACTION,
                     function() { this._uidSelected(this.active_uid) },
                     false, this);

  if (section_response['refresh']) {
    var button = new goog.ui.Button('Refresh');
    button.render(div);

    goog.events.listen(button,
                       goog.ui.Component.EventType.ACTION,
                       function() { this._loadSection() },
                       false, this);
  }

  if (editable) {
    var text = section_response['save_button'] || 'Save';
    var button = new goog.ui.Button(text);
    button.render(div);

    goog.events.listen(button,
                       goog.ui.Component.EventType.ACTION,
                       function() { this._saveSection() },
                       false, this);
  }

  this.items = section_response['items'];
};


/**
 * Generate the html for an item
 * @param {Element} table the table object to add the row to.
 * @param {Object} item_info the data for the item.
 * @private
 */
ola.mobile.UniverseTab.prototype._buildItem = function(table, item_info) {
  var type = item_info['type'];
  var value = item_info['value'];
  var id = item_info['id'];

  if (type == 'hidden') {
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
    } else if (type == 'bool') {
      var check = new goog.ui.Checkbox();
      check.setChecked(value == 1);
      check.render(td);
      item_info['object'] = check;
    } else {
      // select box
      var select = new goog.ui.Select();
      var count = value.length;
      for (var i = 0; i < count; ++i) {
        select.addItem(new goog.ui.Option(value[i]['label']));
      }
      if (item_info['selected_offset'] != undefined) {
        select.setSelectedIndex(item_info['selected_offset']);
      }
      select.render(td);
      item_info['object'] = select;
    }
  } else {
    td.innerHTML = value;
  }
};


/**
 * Called when we need to save a section
 */
ola.mobile.UniverseTab.prototype._saveSection = function() {
  var items = this.items;
  var count = items.length;

  var form = goog.dom.$(this.active_section.id());

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

  var tab = this;
  this.ola_server.rdmSetSectionInfo(
      this.active_universe,
      this.active_uid,
      this.active_section.id(),
      this.active_section.hint(),
      data,
      function(e) { tab._saveSectionComplete(e); });
}


/**
 * Called when the save section completes.
 */
ola.mobile.UniverseTab.prototype._saveSectionComplete = function(e) {
  var response = e.target.getResponseJson();
  if (response['error']) {
    alert(response['error']);
  } else {
    // reload data
    this._loadSection();
  }
};
