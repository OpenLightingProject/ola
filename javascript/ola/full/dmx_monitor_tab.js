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
 * The DMX monitor tab.
 * Copyright (C) 2012 Simon Newton
 */

goog.require('goog.events');
goog.require('goog.ui.Toolbar');
goog.require('ola.common.BaseUniverseTab');
goog.require('ola.common.DmxMonitor');

goog.provide('ola.DmxMonitorTab');

/**
 * The DMX monitor tab.
 * @constructor
 */
ola.DmxMonitorTab = function(element) {
  ola.common.BaseUniverseTab.call(this, element);
  this.dmx_monitor = new ola.common.DmxMonitor(
      goog.dom.$('monitor_values'));
  this.setup = false;
};
goog.inherits(ola.DmxMonitorTab, ola.common.BaseUniverseTab);


/**
 * Called when the tab changes visibility.
 */
ola.DmxMonitorTab.prototype.setActive = function(state) {
  ola.DmxMonitorTab.superClass_.setActive.call(this, state);

  if (this.isActive() && !this.setup) {
    // setup the toolbar
    var toolbar = new goog.ui.Toolbar();
    toolbar.decorate(goog.dom.$('monitor_toolbar'));
    var view_button = toolbar.getChild('monitor_view_button');
    view_button.setTooltip('Change the DMX Monitor layout');
    goog.events.listen(view_button,
                       goog.ui.Component.EventType.ACTION,
                       this.viewChanged_,
                       false,
                       this);
    this.setup = true;
  }
  this.dmx_monitor.setState(state, this.getUniverse());
};


/**
 * Called when the view changes
 * @param {Object} e the event object.
 * @private
 */
ola.DmxMonitorTab.prototype.viewChanged_ = function(e) {
  var value = e.target.getCaption();
  if (value == 'Full') {
    goog.dom.$('monitor_values').className = 'monitor_full';
  } else {
    goog.dom.$('monitor_values').className = 'monitor_compact';
  }
};
