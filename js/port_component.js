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
 * The new universe frame.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.ui.Checkbox');
goog.require('goog.ui.Component');

goog.provide('ola.AvailablePortComponent');

var ola = ola || {}


/**
 * A line in the active universe list.
 * @class
 */
ola.AvailablePortComponent = function(data, opt_domHelper) {
  goog.ui.Component.call(this, opt_domHelper);
  this.data = data;
};
goog.inherits(ola.AvailablePortComponent, goog.ui.Component);


/**
 * This component can't be used to decorate
 */
ola.AvailablePortComponent.prototype.canDecorate = function() {
  return false;
};


/**
 * Create the dom for this component
 */
ola.AvailablePortComponent.prototype.createDom = function() {
  var tr = this.dom_.createDom('tr', {});
  tr.style.cursor = 'pointer';
  var td = goog.dom.createDom('td', {}, '');
  this.dom_.appendChild(tr, td);
  this.checkbox = new goog.ui.Checkbox();
  this.checkbox.render(td);
  this.dom_.appendChild(tr, goog.dom.createDom('td', {}, this.data['device']));
  this.dom_.appendChild(tr, goog.dom.createDom('td', {},
      this.data['is_output'] ?  'Output' : 'Input'));
  this.dom_.appendChild(tr, goog.dom.createDom('td', {},
      this.data['description']));
  this.setElementInternal(tr);

  goog.events.listen(tr,
                     goog.events.EventType.CLICK,
                     function () { this.checkbox.toggle(); },
                     false, this);
};


/**
 * Get the id of this item
 */
ola.AvailablePortComponent.prototype.Id = function() {
  return this.data['id'];
};


/**
 * Check is this was selected
 */
ola.AvailablePortComponent.prototype.IsSelected = function() {
  return this.checkbox.isChecked();
};


/**
 * Update this item with from new data
 */
ola.AvailablePortComponent.prototype.Update = function(new_data) {
  var element = this.getElement();
  var td = goog.dom.getFirstElementChild(element);
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['device'];
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['is_output'] ? 'Output' : 'Input';
  td = goog.dom.getNextElementSibling(td);
  td.innerHTML = new_data['description'];
};


/**
 * The base class for a factory which produces AvailablePortComponents
 * @class
 */
ola.AvailablePortComponentFactory = function() {
};


/**
 * @returns an instance of a AvailablePortComponent
 */
ola.AvailablePortComponentFactory.prototype.newComponent = function(data) {
  return new ola.AvailablePortComponent(data);
};
