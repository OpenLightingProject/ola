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
  ['$scope', '$routeParams', '$ola',
    function($scope, $routeParams, $ola) {
      'use strict';
      $ola.get.InfoPlugin($routeParams.id).then(function(data) {
        $scope.active = data.active;
        $scope.enabled = data.enabled;
        $scope.name = data.name;
        //TODO(Dave_o): clean this up and use proper angular
        var description = document.getElementById('description');
        description.textContent = data.description;
        description.innerHTML =
          description.innerHTML.replace(/\\n/g, '<br />');
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