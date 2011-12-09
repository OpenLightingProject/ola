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
 * The keypad parser.
 * Copyright (C) 2011 Chris Stranex
 *
 * The parser takes commands (see below) and executes them.
 *
 * Syntax:
 *   command ::= channel-part "@" value
 *
 *   channel-part ::= channel | channel_range
 *
 *   channel ::= *digit
 *     Channels range between 1 and 512.
 *
 *   channel_range ::= "ALL" | "*" | channel "THRU" channel | channel > channel
 *
 *   value ::= *digit | "FULL" | "+"
 *      Values range between 0 and 255.
 */

goog.require('ola.common.Server');
goog.require('ola.common.Server.EventType');

goog.provide('ola.mobile.KeypadParser');


/**
 * The KeypadParser class
 * @param {number} universe_id Universe ID
 * @constructor
 */
ola.mobile.KeypadParser = function(universe_id) {
  this.universe_id = universe_id;
  this.values = [-1, -1, -1]; // Start, End, Value
  this.regex = new RegExp(/(([0-9]{1,3})( THRU ([0-9]{0,3}))?)( @ ([0-9]{0,3}))?$/);
}

/**
 * parse function. Can parse incomplete commands.
 * Returns false on parser error
 */
ola.mobile.KeypadParser.prototype.parse = function(str) {
  // It's empty so we can return true really
  if (str.length == 0) {
    this.values = [-1, -1, -1];
    return true;
  }

  // Syntax
  var result = this.regex.exec(this._aliases(str));
  if (result == null) {
    return false;
  }

  // Values
  var start = parseInt(result[2]);
  if (isNaN(start) && result[2] != null) {
    return false
  }
  if (this._constraint(start, 1, 512)) {
    this.values[0] = start;
  } else {
    return false;
  }

  if (typeof result[4] != 'undefined' && result[4] != '') {
    var end = parseInt(result[4]);
    if (isNaN(end)) {
      return false;
    }
    if (end <= start) {
      return false;
    }
    if (this._constraint(i, 1, 512)) {
      this.values[1] = end;
    } else {
      return false;
    }
  }

  if (typeof result[6] != 'undefined' && result[6] != '') {
    var value = parseInt(result[6]);
    if (isNaN(value)) {
      return false;
    }
    if (this._constraint(value, 0, 255)) {
      this.values[2] = value;
    } else {
      return false;
    }
  }
  return true;
}


/**
 * Checks if a number is within a constraint
 */
ola.mobile.KeypadParser.prototype._constraint = function(n, lower, upper) {
  return n >= lower && n <= upper;
}


/**
 * Converts aliases
 */
ola.mobile.KeypadParser.prototype._aliases = function(str) {
  str = str.replace(">", "THRU");
  str = str.replace("*", "1 THRU 512");
  str = str.replace("ALL", "1 THRU 512");
  str = str.replace("@ +", "@ 255");
  str = str.replace("@ FULL", "@ 255");
  return str;
}


/**
 * Execute the command parsed by parser previously.
 * Note: parse() must always be called before execute()
 * This just asks ola for current dmx values before
 * running _execute() where the real work happens
 */
ola.mobile.KeypadParser.prototype.execute = function() {
  if (this.values[0] == -1 || this.values[1] == -1 || this.values[2] == -1) {
    return false;
  }

  var tab = this;
  ola.common.Server.getInstance().getChannelValues(
      this.universe_id,
      function(e){ tab._execute(e); });
  return true;
}


/**
 * Executes the command parsed by the parser. This method
 * is called once DMX values are retrieved by the server.
 * Resets the command parameters so execute() cannot be
 * called again
 */
ola.mobile.KeypadParser.prototype._execute = function(e) {
  var dmx_values = e['dmx'];

  if (this.values[1] == -1) {
    dmx_values[this.values[0] - 1] = this.values[2];
  } else {
    for (i = this.values[0]; i <= this.values[1]; ++i) {
      dmx_values[i-1] = this.values[2];
    }
  }

  // Send the values to OLA
  ola.common.Server.getInstance().setChannelValues(this.universe_id,
                                                   dmx_values);
  this.values = [-1, -1, -1];
}
