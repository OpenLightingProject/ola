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
 * The universe controls
 * Copyright (C) 2010 Simon Newton
 */

goog.require('ola.common.GenericControl');

goog.provide('ola.UniverseControl');


/**
 * An Universe navigation control element.
 * @constructor
 * @param {Object} item the item to add.
 * @param {function()} callback the function to run when the item is clicked.
 * @param {goog.ui.ControlRenderer=} opt_renderer Renderer used to render or
 *     decorate the component; defaults to {@link goog.ui.ControlRenderer}.
 * @param {goog.dom.DomHelper=} opt_domHelper An optional DOM helper.
 */
ola.UniverseControl = function(item, callback, opt_renderer, opt_domHelper) {
  ola.common.GenericControl.call(this,
                                 item,
                                 callback,
                                 opt_renderer,
                                 opt_domHelper);
  this.setContent(item.name());
};
goog.inherits(ola.UniverseControl, ola.common.GenericControl);


/**
 * Setup the event handler for this object.
 */
ola.UniverseControl.prototype.enterDocument = function() {
  ola.UniverseControl.superClass_.enterDocument.call(this);
  this.getElement().title = 'Universe ' + this._item.id();
};


/**
 * A factory which produces UniverseControls
 * @param {function()} callback the function called when the control is clicked.
 * @constructor
 */
ola.UniverseControlFactory = function(callback) {
  this.callback = callback;
};


/**
 * Create a new UniverseControl
 * @param {Object} item The item for the control.
 * @return {ola.UniverseControl} an instance of a UniverseRow.
 */
ola.UniverseControlFactory.prototype.newComponent = function(item) {
  return new ola.UniverseControl(item, this.callback);
};
