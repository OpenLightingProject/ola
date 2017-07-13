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
 * The new OLA web UI.
 * Copyright (C) 2015 Dave Olsthoorn
 */
/*jshint browser: true, jquery: true*/
/* global ola */
ola.controller('keypadUniverseCtrl',
  ['$scope', '$ola', '$routeParams', 'OLA',
    function($scope, $ola, $routeParams, OLA) {
      'use strict';
      $scope.Universe = $routeParams.id;
      var regexkeypad;
      /*jscs:disable maximumLineLength */
      /*jshint ignore:start */
      // The following regex doesn't fit in the 80 character limit so to avoid
      // errors from the linters they are disabled on this part
      regexkeypad =
        /^(?:([0-9]{1,3})(?:\s(THRU)\s(?:([0-9]{1,3}))?)?(?:\s(@)\s(?:([0-9]{1,3}|FULL))?)?)/;
      /*jscs:enable maximumLineLength */
      /* jshint ignore:end */

      var check = {
        channelValue: function(value) {
          return OLA.MIN_CHANNEL_VALUE <= value &&
            value <= OLA.MAX_CHANNEL_VALUE;
        },
        channelNumber: function(value) {
          return OLA.MIN_CHANNEL_NUMBER <= value &&
            value <= OLA.MAX_CHANNEL_NUMBER;
        },
        regexGroups: function(result) {
          if (result[1] !== undefined) {
            var check1 = this.channelNumber(parseInt(result[1], 10));
            if (!check1) {
              return false;
            }
          }
          if (result[3] !== undefined) {
            var check2 = this.channelNumber(parseInt(result[3], 10));
            if (!check2) {
              return false;
            }
          }
          if (result[5] !== undefined && result[5] !== 'FULL') {
            var check3 = this.channelValue(parseInt(result[5], 10));
            if (!check3) {
              return false;
            }
          }
          return true;
        }
      };

      $scope.field = '';
      $scope.input = function(input) {
        var tmpField;
        if (input === 'backspace') {
          tmpField = $scope.field.substr(0, $scope.field.length - 1);
        } else {
          tmpField = $scope.field + input;
        }
        var fields = regexkeypad.exec(tmpField);
        if (fields === null) {
          $scope.field = '';
        } else if (check.regexGroups(fields)) {
          $scope.field = fields[0];
        }
        $scope.focusInput = true;
      };

      $scope.keypress = function($event) {
        var key = $event.key;

        // don't handle keyboard shortcuts and function keys
        if ($event.altKey || $event.ctrlKey || $event.metaKey ||
            ($event.which === 0 && key !== 'Enter' && key !== 'Backspace')) {
          // $event.which is 0 for non-printable keys (like the F1 - F12 keys)
          return;
        }

        $event.preventDefault();

        switch (key) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            $scope.input(key);
            break;
          case '@':
          case 'a':
            $scope.input(' @ ');
            break;
          case '>':
          case 't':
            $scope.input(' THRU ');
            break;
          case 'f':
            $scope.input('FULL');
            break;
          case 'Backspace':
            $scope.input('backspace');
            break;
          case 'Enter':
            $scope.submit();
            break;
        }
      };

      $scope.submit = function() {
        $scope.focusInput = true;

        var dmx = [];
        var input = $scope.field;
        var result = regexkeypad.exec(input);
        if (result !== null && check.regexGroups(result)) {
          var begin = parseInt(result[1], 10);
          var end = result[3] ? parseInt(result[3], 10) :
            parseInt(result[1], 10);
          var value = (result[5] === 'FULL') ?
            OLA.MAX_CHANNEL_VALUE : parseInt(result[5], 10);
          if (begin <= end && check.channelValue(value)) {
            $ola.get.Dmx($scope.Universe).then(function(data) {
              for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
                if (i < data.dmx.length) {
                  dmx[i] = data.dmx[i];
                } else {
                  dmx[i] = OLA.MIN_CHANNEL_VALUE;
                }
              }
              for (var j = begin; j <= end; j++) {
                dmx[j - 1] = value;
              }
              $ola.post.Dmx($scope.Universe, dmx);
              $scope.field = '';
            });
            return true;
          } else {
            return false;
          }
        } else {
          return false;
        }
      };

      $scope.focusInput = true;
    }
  ]);
