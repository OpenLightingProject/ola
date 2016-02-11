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
ola.controller('faderUniverseCtrl',
  ['$scope', '$ola', '$routeParams', '$window', '$interval', 'OLA',
    function($scope, $ola, $routeParams, $window, $interval, OLA) {
      'use strict';
      $scope.get = [];
      $scope.list = [];
      $scope.last = 0;
      $scope.offset = 0;
      $scope.send = false;
      $scope.OLA = OLA;
      $scope.Universe = $routeParams.id;

      for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
        $scope.list[i] = i;
        $scope.get[i] = OLA.MIN_CHANNEL_VALUE;
      }

      $scope.light = function(j) {
        for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
          $scope.get[i] = j;
        }
        $scope.change();
      };

      var dmxGet = $interval(function() {
        $ola.get.Dmx($scope.Universe).then(function(data) {
          for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
            if (i < data.dmx.length) {
              $scope.get[i] = data.dmx[i];
            } else {
              $scope.get[i] = OLA.MIN_CHANNEL_VALUE;
            }
          }
          $scope.send = true;
        });
      }, 1000);

      $scope.getColor = function(i) {
        if (i > 140) {
          return 'black';
        } else {
          return 'white';
        }
      };

      $scope.ceil = function(i) {
        return $window.Math.ceil(i);
      };

      $scope.change = function() {
        $ola.post.Dmx($scope.Universe, $scope.get);
      };

      $scope.page = function(d) {
        if (d === 1) {
          var offsetLimit =
            $window.Math.ceil(OLA.MAX_CHANNEL_NUMBER / $scope.limit);
          if (($scope.offset + 1) !== offsetLimit) {
            $scope.offset++;
          }
        } else if (d === OLA.MIN_CHANNEL_VALUE) {
          if ($scope.offset !== OLA.MIN_CHANNEL_VALUE) {
            $scope.offset--;
          }
        }
      };

      $scope.getWidth = function() {
        var width =
          $window.Math.floor(($window.innerWidth * 0.99) / $scope.limit);
        var amount = width - (52 / $scope.limit);
        return amount + 'px';
      };

      $scope.getLimit = function() {
        var width = ($window.innerWidth * 0.99) / 66;
        return $window.Math.floor(width);
      };

      $scope.limit = $scope.getLimit();

      $scope.width = {
        'width': $scope.getWidth()
      };

      $window.$($window).resize(function() {
        $scope.$apply(function() {
          $scope.limit = $scope.getLimit();
          $scope.width = {
            width: $scope.getWidth()
          };
        });
      });

      $scope.$on('$destroy', function() {
        $interval.cancel(dmxGet);
      });
    }
  ]);