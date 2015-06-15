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
 * Render an item
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.ui.Checkbox');
goog.require('goog.ui.Control');
goog.require('goog.ui.Option');
goog.require('goog.ui.Select');

goog.provide('ola.common.SectionRenderer');


/**
 * Generate the html for an item
 * @param {Element} table the table object to add the row to.
 * @param {Object} item_info the data for the item.
 */
ola.common.SectionRenderer.RenderItem = function(table, item_info) {
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
