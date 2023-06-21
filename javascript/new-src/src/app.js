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
/*jshint browser: true, jquery: true */
/* global angular */
var ola = angular.module('olaApp', ['ngRoute', 'hc.marked']);

// AngularJS 1.6 now uses an '!' as the default hashPrefix, breaking
// our routes. Following lines revert the default to the previous empty
// string.
// See https://docs.angularjs.org/guide/migration#commit-aa077e8
ola.config(['$locationProvider', function($locationProvider) {
  'use strict';
  $locationProvider.hashPrefix('');
}]);

ola.config(['$routeProvider',
  function($routeProvider) {
    'use strict';
    $routeProvider.when('/', {
      templateUrl: '/new/views/overview.html',
      controller: 'overviewCtrl'
    }).when('/universes/', {
      templateUrl: '/new/views/universes.html',
      controller: 'overviewCtrl'
    }).when('/universe/add', {
      templateUrl: '/new/views/universe-add.html',
      controller: 'addUniverseCtrl'
    }).when('/universe/:id', {
      templateUrl: '/new/views/universe-overview.html',
      controller: 'universeCtrl'
    }).when('/universe/:id/keypad', {
      templateUrl: '/new/views/universe-keypad.html',
      controller: 'keypadUniverseCtrl'
    }).when('/universe/:id/faders', {
      templateUrl: '/new/views/universe-faders.html',
      controller: 'faderUniverseCtrl'
    }).when('/universe/:id/rdm', {
      templateUrl: '/new/views/universe-rdm.html',
      controller: 'rdmUniverseCtrl'
    }).when('/universe/:id/patch', {
      templateUrl: '/new/views/universe-patch.html',
      controller: 'patchUniverseCtrl'
    }).when('/universe/:id/settings', {
      templateUrl: '/new/views/universe-settings.html',
      controller: 'settingUniverseCtrl'
    }).when('/plugins', {
      templateUrl: '/new/views/plugins.html',
      controller: 'pluginsCtrl'
    }).when('/plugin/:id', {
      templateUrl: '/new/views/plugin-info.html',
      controller: 'pluginInfoCtrl'
    }).otherwise({
      redirectTo: '/'
    });
  }
]);

ola.config(['markedProvider',
  function(markedProvider) {
    'use strict';
    markedProvider.setOptions({
      gfm: true,
      tables: true
    });
  }
]);
