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
 * The OLA dialog box
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.ui.Dialog');

goog.provide('ola.Dialog');


/**
 * The OLA Dialog class
 * @constructor
 */
ola.Dialog = function() {
  goog.ui.Dialog.call(this, null, true);
};
goog.inherits(ola.Dialog, goog.ui.Dialog);

// This is a singleton, call ola.Dialog.getInstance() to access it.
goog.addSingletonGetter(ola.Dialog);


/**
 * Make this dialog show the spinner
 */
ola.Dialog.prototype.setAsBusy = function() {
  this.setTitle('Waiting for server response....');
  this.setButtonSet(null);
  this.setContent('<div align="center"><img src="/loader.gif">' +
                  '</div>');
};
