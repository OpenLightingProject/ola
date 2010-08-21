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

var ola = ola || {}
ola.logger = goog.debug.Logger.getLogger('ola');


/**
 * Setup the logger window
 * @constructor
 */
ola.LoggerWindow = function() {
  goog.debug.LogManager.getRoot().setLevel(goog.debug.Logger.Level.ALL);
  var logconsole = new goog.debug.DivConsole(goog.dom.$('log'));
  logconsole.setCapturing(true);

  this.log_control = goog.dom.$('log_control');

  var control = new goog.ui.Control("log_control");
  control.decorate(this.log_control);
  goog.events.listen(this.log_control, goog.events.EventType.CLICK,
                     this.Show, false, this);

  var popupElt = document.getElementById('log_popup');
  this.popup = new goog.ui.Popup(popupElt);
  this.popup.setHideOnEscape(true);
  this.popup.setAutoHide(true);
}


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
}
