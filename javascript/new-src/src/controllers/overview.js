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
ola.controller('overviewCtrl',
  ['$scope', '$ola', '$location',
    function($scope, $ola, $location) {
      'use strict';
      $scope.Info = {};
      $scope.Universes = {};

      $ola.get.ItemList().then(function(data) {
        $scope.Universes = data.universes;
      });

      $ola.get.ServerInfo().then(function(data) {
        $scope.Info = data;
      });

      $scope.Shutdown = function() {
        $ola.action.Shutdown().then();
      };

      $scope.goUniverse = function(id) {
        $location.path('/universe/' + id);
      };
    }
  ]);
