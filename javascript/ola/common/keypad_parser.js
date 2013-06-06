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

goog.require('ola.common.BaseTypes');
goog.provide('ola.common.KeypadCommand');
goog.provide('ola.common.KeypadParser');

/**
 * Create a new KeypadCommand
 * @param {number} start the starting slot.
 * @param {number} end the end slot, or undefined if this is a single slot
 *   command.
 * @param {number} value the slot value.
 * @constructor
 */
ola.common.KeypadCommand = function(start, end, value) {
  this.start = start;
  this.end = end == undefined ? start : end;
  this.value = value;
};


/**
 * Check if this command is valid
 * @return {bool} true if the command if valid, false otherwise.
 */
ola.common.KeypadCommand.prototype.isValid = function() {
  return (
      (this.start >= ola.common.BaseTypes.MIN_CHANNEL_NUMBER &&
       this.start <= ola.common.BaseTypes.MAX_CHANNEL_NUMBER) &&
      (this.value >= ola.common.BaseTypes.MIN_CHANNEL_VALUE &&
       this.value <= ola.common.BaseTypes.MAX_CHANNEL_VALUE) &&
      (this.end == undefined ||
       (this.end >= ola.common.BaseTypes.MIN_CHANNEL_NUMBER &&
        this.end <= ola.common.BaseTypes.MAX_CHANNEL_NUMBER &&
        this.end >= this.start))
  );
};


/**
 * The KeypadParser class
 * @constructor
 */
ola.common.KeypadParser = function() {
  this.full_command_regex = new RegExp(
      /(?:([0-9]{1,3})(?:\s+THRU\s+([0-9]{0,3}))?)\s+@\s+([0-9]{0,3})$/);
  // similar to above, but allows for partially completed commands
  this.partial_command_regex = new RegExp(
     /(?:([0-9]{1,3})(?:\s+THRU\s+([0-9]{0,3}))?)(?:\s+@\s+([0-9]{0,3}))?$/);
};


/**
 * Parse a full command
 * @param {string} str the input string.
 * @return {bool} true if the command is valid, false otherwise.
 */
ola.common.KeypadParser.prototype.parsePartialCommand = function(str) {
  if (str.length == 0) {
    return false;
  }

  var result = this.partial_command_regex.exec(this.aliases_(str));
  if (result == null) {
    return false;
  }

  var start_token = result[1];
  if (start_token != undefined) {
    var start = this.intOrUndefined_(result[1]);
    if (start == undefined || start == 0 ||
        start > ola.common.BaseTypes.MAX_CHANNEL_NUMBER) {
      return false;
    }
  }

  var end_token = result[2];
  if (end_token != undefined && end_token != '') {
    var end = this.intOrUndefined_(result[2]);
    if (end == undefined || end == 0 ||
        end > ola.common.BaseTypes.MAX_CHANNEL_NUMBER) {
      return false;
    }
  }

  var value_token = result[3];
  if (value_token != undefined && value_token != '') {
    var value = this.intOrUndefined_(result[3]);
    if (value == undefined || value > ola.common.BaseTypes.MAX_CHANNEL_VALUE) {
      return false;
    }
  }
  return true;
};


/**
 * Parse a full command
 * @param {string} str the input string.
 * @return {KeypadCommand|undefined} Returns a KeypadCommand or undefined on
 * error. If returned, the KeypadCommand is guarenteed to be valid.
 */
ola.common.KeypadParser.prototype.parseFullCommand = function(str) {
  // It's empty so we can return true really
  if (str.length == 0) {
    return undefined;
  }

  var result = this.full_command_regex.exec(this.aliases_(str));
  if (result == null) {
    return undefined;
  }

  var start = this.intOrUndefined_(result[1]);
  var end = this.intOrUndefined_(result[2]);
  var value = this.intOrUndefined_(result[3]);

  if (start == undefined || value == undefined) {
    return undefined;
  }

  if (result[2] != undefined) {
    // was a range
    if (end == undefined)
      return false;
  }

  var command = new ola.common.KeypadCommand(start, end, value);
  return command.isValid() ? command : undefined;
};


/**
 * Convert a string to an int, or return undefined
 * @param {string} token the string to convert.
 * @return {number|undefined} The integer, or undefined.
 * @private
 */
ola.common.KeypadParser.prototype.intOrUndefined_ = function(token) {
  if (token == null || token == undefined)
    return undefined;
  var i = parseInt(token);
  return isNaN(i) ? undefined : i;
};


/**
 * Converts aliases.
 * @param {string} str the input string.
 * @return {string} the output string.
 * @private
 */
ola.common.KeypadParser.prototype.aliases_ = function(str) {
  str = str.replace('>', 'THRU');
  str = str.replace('*', ola.common.BaseTypes.MIN_CHANNEL_NUMBER + ' THRU ' +
                    ola.common.BaseTypes.MAX_CHANNEL_NUMBER);
  str = str.replace('ALL', ola.common.BaseTypes.MIN_CHANNEL_NUMBER + ' THRU ' +
                    ola.common.BaseTypes.MAX_CHANNEL_NUMBER);
  str = str.replace('@ +', '@ ' + ola.common.BaseTypes.MAX_CHANNEL_VALUE);
  str = str.replace('@ FULL', '@ ' + ola.common.BaseTypes.MAX_CHANNEL_VALUE);
  return str;
};
