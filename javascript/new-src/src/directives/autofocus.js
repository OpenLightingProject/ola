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
 * Allows an element to obtain focus whenever a variable is set to true.
 * Copyright (C) 2016 Florian Edelmann
 */
/*jshint browser: true, jquery: true*/
/* global ola */
ola.directive('autofocus', ['$timeout', '$parse',
  function($timeout, $parse) {
    'use strict';
    return {
      restrict: 'A',
      link: function($scope, $element, $attrs) {
        var model = $parse($attrs.autofocus);
        $scope.$watch(model, function(value) {
          if (value === true) {
            $timeout(function() {
              $element[0].focus();
            });
          }
        });
        $element.bind('blur', function() {
          $scope.$apply(model.assign($scope, false));
        });
      }
    };
  }
]);
