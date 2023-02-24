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
 * The universe tab.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.events');
goog.require('goog.net.HttpStatus');
goog.require('goog.ui.Button');
goog.require('goog.ui.Container');

goog.require('ola.BaseFrame');
goog.require('ola.UniverseControl');
goog.require('ola.UniverseItem');
goog.require('ola.common.RdmSectionControl');
goog.require('ola.common.RdmSectionControlFactory');
goog.require('ola.common.RdmSectionItem');
goog.require('ola.common.SectionRenderer');
goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');
goog.require('ola.common.SortedList');
goog.require('ola.common.UidControlFactory');
goog.require('ola.common.UidItem');

goog.provide('ola.mobile.UniverseTab');


/**
 * The class representing the Universe frame
 * @constructor
 */
ola.mobile.UniverseTab = function() {
  this.universe_frame = new ola.BaseFrame('universe_frame');
  this.uid_frame = new ola.BaseFrame('uid_frame');
  this.rdm_frame = new ola.BaseFrame('rdm_frame');
  this.rdm_section_frame = new ola.BaseFrame('rdm_section_frame');

  this.hideAllFrames_();

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
                     this.updateUniverseList_,
                     false, this);

};


/** The title of this tab */
ola.mobile.UniverseTab.prototype.title = function() {
  return 'RDM';
};


/**
 * Called when the tab loses focus
 */
ola.mobile.UniverseTab.prototype.blur = function() {};


/**
 * Hide all frames
 * @private
 */
ola.mobile.UniverseTab.prototype.hideAllFrames_ = function() {
  this.universe_frame.Hide();
  this.uid_frame.Hide();
  this.rdm_frame.Hide();
  this.rdm_section_frame.Hide();
};


/**
 * Called when the universe tab is clicked
 */
ola.mobile.UniverseTab.prototype.update = function() {
  this.hideAllFrames_();
  this.universe_frame.setAsBusy();
  this.universe_list = undefined;
  this.active_universe = undefined;
  this.active_uid = undefined;
  this.active_section = undefined;
  this.universe_frame.Show();
  this.ola_server.FetchUniversePluginList();
};


/**
 * Called when a new list of universes is received.
 * @param {Object} e the event object.
 * @private
 */
ola.mobile.UniverseTab.prototype.updateUniverseList_ = function(e) {
  if (this.universe_list == undefined) {
    this.universe_frame.Clear();
    var universe_container = new goog.ui.Container();
    universe_container.render(this.universe_frame.element);

    var tab = this;
    this.universe_list = new ola.common.SortedList(
        universe_container,
        new ola.UniverseControlFactory(
          function(item) { tab.universeSelected_(item.id()); }));
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
 * @param {number} universe_id the id of the universe selected.
 * @private
 */
ola.mobile.UniverseTab.prototype.universeSelected_ = function(universe_id) {
  this.hideAllFrames_();
  this.uid_frame.setAsBusy();
  this.uid_list = undefined;
  this.rdm_list = undefined;
  this.active_universe = universe_id;
  this.uid_frame.Show();

  var tab = this;
  this.ola_server.fetchUids(
      universe_id,
      function(e) { tab.updateUidList_(e); });

  // setup a timer here
};


/**
 * Called when a new list of uids is received.
 * @param {Object} e the event object.
 * @private
 */
ola.mobile.UniverseTab.prototype.updateUidList_ = function(e) {
  if (e.target.getStatus() != goog.net.HttpStatus.OK) {
    return;
  }

  if (this.uid_list == undefined) {
    this.uid_frame.Clear();

    var uid_container = new goog.ui.Container();
    uid_container.render(this.uid_frame.element);

    var tab = this;
    this.uid_list = new ola.common.SortedList(
        uid_container,
        new ola.common.UidControlFactory(
          function(item) { tab.uidSelected_(item.id()); }));

    var button = new goog.ui.Button('Back');
    button.render(this.uid_frame.element);

    goog.events.listen(button,
                       goog.ui.Component.EventType.ACTION,
                       function() { this.update() },
                       false, this);
  }

  var obj = e.target.getResponseJson();
  var uids = obj['uids'];
  var items = new Array();
  for (var i = 0; i < uids.length; ++i) {
    items.push(new ola.common.UidItem(uids[i]));
  }
  this.uid_list.updateFromData(items);
};


/**
 * Called when a uid is selected
 * @param {Object} uid the UID selected.
 * @private
 */
ola.mobile.UniverseTab.prototype.uidSelected_ = function(uid) {
  this.hideAllFrames_();
  this.rdm_frame.setAsBusy();
  this.rdm_list = undefined;
  this.active_uid = uid;
  this.rdm_frame.Show();

  var tab = this;
  this.ola_server.rdmGetSupportedSections(
      this.active_universe,
      uid,
      function(e) { tab.updateSupportedSections_(e); });
};


/**
 * Called when a list of supported sections is received.
 * @param {Object} e the event object.
 * @private
 */
ola.mobile.UniverseTab.prototype.updateSupportedSections_ = function(e) {
  if (this.rdm_list == undefined) {
    this.rdm_frame.Clear();

    var rdm_container = new goog.ui.Container();
    rdm_container.render(this.rdm_frame.element);

    var tab = this;
    this.rdm_list = new ola.common.SortedList(
        rdm_container,
        new ola.common.RdmSectionControlFactory(
          function(item) { tab.sectionSelected_(item); }));

    var button = new goog.ui.Button('Back');
    button.render(this.rdm_frame.element);

    goog.events.listen(
        button,
        goog.ui.Component.EventType.ACTION,
        function() { this.universeSelected_(this.active_universe) },
        false, this);
  }

  var sections = e.target.getResponseJson();
  var section_count = sections.length;
  var items = new Array();
  for (var i = 0; i < section_count; ++i) {
    items.push(new ola.common.RdmSectionItem(sections[i]));
  }
  this.rdm_list.updateFromData(items);
};


/**
 * Called when a section is selected
 * @param {Object} section the Section object.
 * @private
 */
ola.mobile.UniverseTab.prototype.sectionSelected_ = function(section) {
  this.hideAllFrames_();
  this.rdm_section_frame.setAsBusy();
  this.rdm_section_frame.Show();
  this.active_section = section;
  this.loadSection_();
};


/**
 * Called when we need to load a section
 * @private
 */
ola.mobile.UniverseTab.prototype.loadSection_ = function() {
  var tab = this;

  this.ola_server.rdmGetSectionInfo(
      this.active_universe,
      this.active_uid,
      this.active_section.id(),
      this.active_section.hint(),
      function(e) { tab.updateSection_(e); });
};


/**
 * Called when a section is ready to be drawn
 * @param {Object} e the event object.
 * @private
 */
ola.mobile.UniverseTab.prototype.updateSection_ = function(e) {
  var section_response = e.target.getResponseJson();

  this.rdm_section_frame.Clear();
  var div = this.rdm_section_frame.element;
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
  form.onsubmit = function() { tab.saveSection_(); return false};
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

  var button = new goog.ui.Button('Back');
  button.render(div);

  goog.events.listen(button,
                     goog.ui.Component.EventType.ACTION,
                     function() { this.uidSelected_(this.active_uid) },
                     false, this);

  if (section_response['refresh']) {
    var button = new goog.ui.Button('Refresh');
    button.render(div);

    goog.events.listen(button,
                       goog.ui.Component.EventType.ACTION,
                       function() { this.loadSection_() },
                       false, this);
  }

  if (editable) {
    var text = section_response['save_button'] || 'Save';
    var button = new goog.ui.Button(text);
    button.render(div);

    goog.events.listen(button,
                       goog.ui.Component.EventType.ACTION,
                       function() { this.saveSection_() },
                       false, this);
  }

  this.items = section_response['items'];
};


/**
 * Called when we need to save a section
 * @private
 */
ola.mobile.UniverseTab.prototype.saveSection_ = function() {
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
      function(e) { tab.saveSectionComplete_(e); });
};


/**
 * Called when the save section completes.
 * @param {Object} e the event object.
 * @private
 */
ola.mobile.UniverseTab.prototype.saveSectionComplete_ = function(e) {
  var response = e.target.getResponseJson();
  if (response['error']) {
    alert(response['error']);
  } else {
    // reload data
    this.loadSection_();
  }
};
