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
goog.require('goog.ui.Component');
goog.require('ola.BaseFrame');
goog.require('ola.LoggerWindow');
goog.require('ola.Server');
goog.require('ola.Server.EventType');
goog.require('ola.SortedList');

goog.provide('ola.NewUniverseFrame');

var ola = ola || {}


/**
 * The class representing the Universe frame
 * @constructor
 */
ola.NewUniverseFrame = function(element_id, ola_server) {
  ola.BaseFrame.call(this, element_id);
  this.ola_server = ola_server

  var cancel_button = goog.dom.$('cancel_new_universe_button');
  goog.ui.decorate(cancel_button);

  var confirm_button = goog.dom.$('confirm_new_universe_button');
  goog.ui.decorate(confirm_button);
  goog.events.listen(confirm_button,
                     goog.events.EventType.CLICK,
                     this._AddButtonClicked,
                     false, this);


  /*
  this.current_universe = undefined;
  goog.events.listen(ola_server, ola.Server.EventType.UNIVERSE_EVENT,
                     this._UpdateFromData,
                     false, this);

  var tabPane = new goog.ui.TabPane(
    document.getElementById(ola.UNIVERSE_TAB_PANE_ID));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_1'), "Settings"));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_2'), 'RDM'));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_3'), 'Console'));
  tabPane.setSelectedIndex(0);
  this.selected_tab = 0;

  goog.events.listen(tabPane, goog.ui.TabPane.Events.CHANGE,
                     this.TabChanged, false, this);

  var save_button = goog.dom.$('new_universe_save_button');
  goog.ui.decorate(save_button);
  */
}

goog.inherits(ola.NewUniverseFrame, ola.BaseFrame);


/**
 * Called when the stop button is clicked
 */
ola.NewUniverseFrame.prototype._AddButtonClicked = function(e) {
  var dialog = ola.Dialog.getInstance();
  dialog.SetAsBusy();
  dialog.setVisible(true);
}
