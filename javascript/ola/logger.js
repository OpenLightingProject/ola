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
 * The class that handles the logging.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.debug.DivConsole');
goog.require('goog.debug.LogManager');
goog.require('goog.debug.Logger');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.math');
goog.require('goog.positioning.Corner');
goog.require('goog.ui.Control');
goog.require('goog.ui.Popup');

goog.provide('ola.LoggerWindow');

/** The logger instance */
ola.logger = goog.debug.Logger.getLogger('ola');


/**
 * Setup the logger window
 * @constructor
 */
ola.LoggerWindow = function() {
  goog.debug.LogManager.getRoot().setLevel(goog.debug.Logger.Level.ALL);

  // setup the log console
  var log_console = new goog.debug.DivConsole(goog.dom.$('log'));
  log_console.setCapturing(true);

  // setup the control which shows the console
  this.log_control = goog.dom.$('log_control');
  var control = new goog.ui.Control();
  control.decorate(this.log_control);
  goog.events.listen(this.log_control, goog.events.EventType.CLICK,
                     this.Show, false, this);

  // setup the popup that the console is contained in
  var popupElt = document.getElementById('log_popup');
  this.popup = new goog.ui.Popup(popupElt);
  this.popup.setHideOnEscape(true);
  this.popup.setAutoHide(true);
};


/**
 * Display the logger window
 */
ola.LoggerWindow.prototype.Show = function() {
  this.popup.setVisible(false);
  this.popup.setPinnedCorner(goog.positioning.Corner.TOP_RIGHT);
  var margin = new goog.math.Box(2, 2, 2, 2);
  this.popup.setMargin(margin);
  this.popup.setPosition(new goog.positioning.AnchoredViewportPosition(
    this.log_control,
    goog.positioning.Corner.BOTTOM_RIGHT));
  this.popup.setVisible(true);
};


/**
 * Set the size of the logger window
 * @param {number} size the size of the main window.
 */
ola.LoggerWindow.prototype.SetSize = function(size) {
  goog.style.setBorderBoxSize(
      goog.dom.$('log_popup'),
      new goog.math.Size(0.75 * size.width, 0.5 * size.height));
  this.popup.setPosition(new goog.positioning.AnchoredViewportPosition(
    this.log_control,
    goog.positioning.Corner.BOTTOM_RIGHT));
};
