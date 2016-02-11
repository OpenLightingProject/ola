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
ola.controller('universeCtrl',
  ['$scope', '$ola', '$routeParams', '$interval', 'OLA',
    function($scope, $ola, $routeParams, $interval, OLA) {
      'use strict';
      $scope.dmx = [];
      $scope.Universe = $routeParams.id;

      var interval = $interval(function() {
        $ola.get.Dmx($scope.Universe).then(function(data) {
          for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
            $scope.dmx[i] =
              (typeof data.dmx[i] === 'number') ?
                data.dmx[i] : OLA.MIN_CHANNEL_VALUE;
          }
        });
      }, 100);

      $scope.$on('$destroy', function() {
        $interval.cancel(interval);
      });

      $scope.getColor = function(i) {
        if (i > 140) {
          return 'black';
        } else {
          return 'white';
        }
      };
    }
  ]);