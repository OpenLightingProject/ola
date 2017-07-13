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
ola.controller('settingUniverseCtrl',
  ['$scope', '$ola', '$routeParams',
    function($scope, $ola, $routeParams) {
      'use strict';
      $scope.loadData = function() {
        $scope.Data = {
          old: {},
          model: {},
          Remove: [],
          Add: []
        };
        $scope.Data.old.id = $scope.Data.model.id = $routeParams.id;
        $ola.get.PortsId($routeParams.id).then(function(data) {
          $scope.DeactivePorts = data;
        });
        $ola.get.UniverseInfo($routeParams.id).then(function(data) {
          $scope.Data.old.name = $scope.Data.model.name = data.name;
          $scope.Data.old.merge_mode = data.merge_mode;
          $scope.Data.model.merge_mode = data.merge_mode;
          $scope.ActivePorts = data.output_ports.concat(data.input_ports);
          $scope.Data.old.ActivePorts =
            data.output_ports.concat(data.input_ports);
          for (var i = 0; i < $scope.ActivePorts.length; ++i) {
            $scope.Data.Remove[i] = '';
          }
        });
      };
      $scope.loadData();
      $scope.Save = function() {
        var a = {};
        a.id = $scope.Data.model.id;
        a.name = $scope.Data.model.name;
        a.merge_mode = $scope.Data.model.merge_mode;
        a.add_ports = $.grep($scope.Data.Add, Boolean).join(',');
        a.remove_ports = $.grep($scope.Data.Remove, Boolean).join(',');
        var modified = [];
        $scope.ActivePorts.forEach(function(element, index) {
          if ($scope.Data.Remove.indexOf($scope.ActivePorts[index].id) === -1) {
            var port = $scope.ActivePorts[index];
            var port_old = $scope.Data.old.ActivePorts[index];
            if (port.priority.current_mode === 'static') {
              if (0 < port.priority.value < 100) {
                a[port.id + '_priority_value'] = port.priority.value;
                if (modified.indexOf(port.id) === -1) {
                  modified.push(port.id);
                }
              }
            }
            if (port_old.priority.current_mode !== port.priority.current_mode) {
              a[port.id + '_priority_mode'] = port.priority.current_mode;
              if (modified.indexOf(port.id) === -1) {
                modified.push(port.id);
              }
            }
          }
        });
        a.modify_ports = $.grep(modified, Boolean).join(',');
        $ola.post.ModifyUniverse(a);
        $scope.loadData();
      };
    }
  ]);
