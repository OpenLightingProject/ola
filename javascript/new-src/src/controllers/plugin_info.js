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
ola.controller('pluginInfoCtrl',
  ['$scope', '$routeParams', '$ola', '$sce', 'marked',
    function($scope, $routeParams, $ola, $sce, marked) {
      'use strict';
      $ola.get.InfoPlugin($routeParams.id).then(function(data) {
        $scope.active = data.active;
        $scope.enabled = data.enabled;
        $scope.name = data.name;
        $scope.description = $sce.trustAsHtml(
          marked(data.description.replace(/\\n/g, '\n'))
        );
      });

      $scope.stateColor = function(val) {
        if (val) {
          return {
            'background-color': 'green'
          };
        } else {
          return {
            'background-color': 'red'
          };
        }
      };
    }
  ]);
