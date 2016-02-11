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
ola.controller('pluginsCtrl',
  ['$scope', '$ola', '$location',
    function($scope, $ola, $location) {
      'use strict';
      $scope.Items = {};
      $scope.active = [];
      $scope.enabled = [];
      $scope.getInfo = function() {
        $ola.get.ItemList()
          .then(function(data) {
            $scope.Items = data;
          });
      };
      $scope.getInfo();
      $scope.Reload = function() {
        $ola.action.Reload();
        $scope.getInfo();
      };
      $scope.go = function(id) {
        $location.path('/plugin/' + id);
      };
      $scope.changeStatus = function(id, current) {
        $ola.post.PluginState(id, current);
        $scope.getInfo();
      };

      $scope.getStyle = function(style) {
        if (style) {
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
