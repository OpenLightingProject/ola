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
ola.controller('addUniverseCtrl', ['$scope', '$ola', '$window', '$location',
  function($scope, $ola, $window, $location) {
    'use strict';
    $scope.Ports = {};
    $scope.addPorts = [];
    $scope.Universes = [];
    $scope.Class = '';
    $scope.Data = {
      id: 0,
      name: '',
      add_ports: ''
    };

    $ola.get.ItemList().then(function(data) {
      for (var u in data.universes) {
        if (data.universes.hasOwnProperty(u)) {
          if ($scope.Data.id === parseInt(data.universes[u].id, 10)) {
            $scope.Data.id++;
          }
          $scope.Universes.push(parseInt(data.universes[u].id, 10));
        }
      }
    });

    $scope.Submit = function() {
      if (typeof $scope.Data.id === 'number' &&
        $scope.Data.add_ports !== '' &&
        $scope.Universes.indexOf($scope.Data.id) === -1) {
        if ($scope.Data.name === undefined || $scope.Data.name === '') {
          $scope.Data.name = 'Universe ' + $scope.Data.id;
        }
        $ola.post.AddUniverse($scope.Data);
        $location.path('/universe/' + $scope.Data.id);
      } else if ($scope.Universes.indexOf($scope.Data.id) !== -1) {
        $ola.error.modal('Universe ID already exists.');
      } else if ($scope.Data.add_ports === undefined ||
        $scope.Data.add_ports === '') {
        $ola.error.modal('There are no ports selected for the universe. ' +
          'This is required.');
      }
    };

    $ola.get.Ports().then(function(data) {
      $scope.Ports = data;
    });

    $scope.getDirection = function(direction) {
      if (direction) {
        return 'Output';
      } else {
        return 'Input';
      }
    };

    $scope.updateId = function() {
      if ($scope.Universes.indexOf($scope.Data.id) !== -1) {
        $scope.Class = 'has-error';
      } else {
        $scope.Class = '';
      }
    };

    $scope.TogglePort = function() {
      $scope.Data.add_ports =
        $window.$.grep($scope.addPorts, Boolean).join(',');
    };
  }
]);
