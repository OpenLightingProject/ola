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
 * The custom OLA dragger.
 * This overrides the initializeDrag_to update deltaX & deltaY after START has
 * been fired. This is needed because we re-parent the target when we start
 * dragging.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.fx.Dragger');

goog.provide('ola.CustomDragger');


ola.CustomDragger = function(target, opt_handle, opt_limits) {
  goog.fx.Dragger.call(this, target, opt_handle, opt_limits);
};
goog.inherits(ola.CustomDragger, goog.fx.Dragger);


/**
 * Event handler that is used to start the drag
 * @param {goog.events.BrowserEvent|goog.events.Event} e Event object.
 * @private
 */
ola.CustomDragger.prototype.initializeDrag_ = function(e) {
  var rv = this.dispatchEvent(new goog.fx.DragEvent(
      goog.fx.Dragger.EventType.START, this, e.clientX, e.clientY,
                                /** @type {goog.events.BrowserEvent} */(e)));
  if (rv !== false) {
    this.deltaX = this.target.offsetLeft;
    this.deltaY = this.target.offsetTop;
    this.dragging_ = true;
  }

  ola.logger.info('delta from parent: ' + this.deltaX + ', ' + this.deltaY);
};


ola.CustomDragger.prototype.calculatePosition_ = function(dx, dy) {
  // Update the position for any change in body scrolling
  var pageScroll = new goog.math.Coordinate(this.scrollTarget_.scrollLeft,
                                            this.scrollTarget_.scrollTop);
  dx += pageScroll.x - this.pageScroll.x;
  dy += pageScroll.y - this.pageScroll.y;
  this.pageScroll = pageScroll;

  this.deltaX += dx;
  this.deltaY += dy;

  var x = this.limitX(this.deltaX);
  var y = this.limitY(this.deltaY);
  return new goog.math.Coordinate(x, y);
};
