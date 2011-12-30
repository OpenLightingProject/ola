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
 * The custom OLA DragScrollSupport.
 * This adds an enable(bool) method so we can turn the scrolling behaviour on
 * and off.
 * Copyright (C) 2010 Simon Newton
 */

goog.require('goog.fx.DragScrollSupport');

goog.provide('ola.CustomDragScrollSupport');


ola.CustomDragScrollSupport = function(containerNode, opt_verticalMargin) {
  goog.fx.DragScrollSupport.call(this, containerNode, opt_verticalMargin,
                                 true);
};
goog.inherits(ola.CustomDragScrollSupport, goog.fx.DragScrollSupport);


ola.CustomDragScrollSupport.prototype.setEnabled = function(state) {
  if (state) {
    this.eventHandler_.listen(goog.dom.getOwnerDocument(this.containerNode_),
                              goog.events.EventType.MOUSEMOVE,
                              this.onMouseMove);
  } else {
    this.eventHandler_.unlisten(goog.dom.getOwnerDocument(this.containerNode_),
                                goog.events.EventType.MOUSEMOVE,
                                this.onMouseMove);
  }
};


ola.CustomDragScrollSupport.prototype.updateBoundaries = function() {
  this.containerBounds_ = goog.style.getBounds(this.containerNode_);
  this.scrollBounds_ = this.verticalMargin_ ?
      this.constrainBounds_(this.containerBounds_.clone()) :
      this.containerBounds_;
};
