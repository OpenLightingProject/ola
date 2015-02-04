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
*/
/* jshint: global: angular */
angular
.module('olaApp', ['ngRoute'])
.constant('Channels', 512)
.constant('TopValue', 255)
.constant('nul', 0)
.factory('$ola', ['$http', 'nul', 'Channels', function ($http, nul, Channels) {
 "use strict";
 var postEncode = function (data) {
  var PostData = [];
  for (var key in data) {
   PostData.push(key + '=' + data[key]);
  }
  return PostData.join('&');
 };
 var dmxConvert = function (dmx) {
  var length = 0, integers = [];
  for (var i = 0; i < Channels; i++) {
   integers[i] = parseInt(dmx[i]);
   if (integers[i] > nul) {
    length = (i + 1);
   }
  }
  integers.length = length;
  return '[' + integers.join(',') + ']';
 };
 return {
  get: {
   ItemList: function () {
    return $http.get('/json/universe_plugin_list').then(function (response) {
     return response.data;
    });
   },
   ServerInfo: function () {
    return $http.get('/json/server_stats').then(function (response) {
     return response.data;
    });
   },
   Ports: function () {
    return $http.get('/json/get_ports').then(function (response) {
     return response.data;
    });
   },
   PortsId: function (id) {
    return $http.get('/json/get_ports?u=' + id).then(function (response) {
     return response.data;
    });
   },
   InfoPlugin: function (id) {
    return $http.get('/json/plugin_info?id=' + id).then(function (response) {
     return response.data;
    });
   },
   Dmx: function (id) {
    return $http.get('/get_dmx?u=' + id).then(function (response) {
     return response.data;
    });
   },
   UniverseInfo: function (id) {
    return $http.get('/json/universe_info?id=' + id).then(function (response) {
     return response.data;
    });
   }
  },
  post: {
   ModifyUniverse: function (data) {
    return $http({
     method: "POST",
     url: '/modify_universe',
     data: postEncode(data),
     headers: {'Content-Type': 'application/x-www-form-urlencoded'}
    }).then(function (response) {
     return response.data;
    });
   },
   AddUniverse: function (data) {
    return $http({
     method: "POST",
     url: '/new_universe',
     data: postEncode(data),
     headers: {'Content-Type': 'application/x-www-form-urlencoded'}
    }).then(function (response) {
     return response.data;
    });
   },
   Dmx: function (universe, dmx) {
    var data = {
     u: universe,
     d: dmxConvert(dmx)
    };
    return $http({
     method: "POST",
     url: '/set_dmx',
     data: postEncode(data),
     headers: {'Content-Type': 'application/x-www-form-urlencoded'}
    }).then(function (response) {
     return response.data;
    });
   }
  },
  action: {
   Shutdown: function () {
    return $http.get('/quit').then(function (response) {
     return response.data;
    });
   },
   Reload: function () {
    return $http.get('/reload').then(function (response) {
     return response.data;
    });
   },
   ReloadPids: function () {
    return $http.get('/reload_pids').then(function (response) {
     return response.data;
    });
   }
  },
  rdm: {
   /*
   /json/rdm/section_info
   /json/rdm/set_section_info
   /json/rdm/supported_pids
   /json/rdm/supported_sections
   /json/rdm/uid_identify
   /json/rdm/uid_info
   /json/rdm/uid_personalities
   /json/rdm/uids
   /rdm/run_discovery
   */

  }
 };
}])
.controller('menuCtrl', ['$scope', '$ola', function ($scope, $ola) {
 "use strict";
 $scope.Items = {};
 $scope.Info = {};
 $ola.get.ItemList().then(function (data) {
  $scope.Items = data;
 });
 $ola.get.ServerInfo().then(function (data) {
  $scope.Info = data;
 });
}])
.controller('overviewCtrl', ['$scope', '$ola', '$location', function ($scope, $ola, $location) {
 "use strict";
 $scope.Info = {};
 $scope.Universes = {};
 $ola.get.ItemList().then(function (data) {
  $scope.Universes = data.universes;
 });
 $ola.get.ServerInfo().then(function (data) {
  $scope.Info = data;
 });
 $scope.Shutdown = function () {
  $ola.action.Shutdown().then(function (data) {
  });
 };
 $scope.goUniverse = function (id) {
  $location.path('/universe/' + id);
 };
}])
.controller('addUniverseCtrl', ['$scope', '$ola', function ($scope, $ola) {
 "use strict";
 $scope.Ports = {};
 $ola.get.Ports().then(function (data) {
  $scope.Ports = data;
 });
}])
.controller('universeCtrl', ['$scope', '$ola', '$routeParams', '$interval', 'nul', 'Channels',
function ($scope, $ola, $routeParams, $interval, nul, Channels) {
 "use strict";
 $scope.dmx = [];
 $scope.Universe = $routeParams.id;
 $interval(function () {
  $ola.get.Dmx($scope.Universe).then(function (data) {
   for (var i = 0; i < Channels; i++) {
    $scope.dmx[i] = (typeof(data.dmx[i]) === "number") ? data.dmx[i] : 0;
   }
  });
 }, 100);
 $scope.$on('$destroy', function () {
  $interval.cancel();
 });
 $scope.getColor = function (i) {
  if (i > 140) {
   return 'black';
  } else {
   return 'white';
  }
 };
}])
.controller('sliderUniverseCtrl', ['$scope', '$ola', '$routeParams', '$window', '$interval', 'nul', 'Channels',
function ($scope, $ola, $routeParams, $window, $interval, nul, Channels) {
 "use strict";
 $scope.get = [];
 $scope.list = [];
 $scope.last = 0;
 $scope.offset = 0;
 $scope.send = false;
 $scope.Universe = $routeParams.id;
 for (var i = 0; i < Channels; i++) {
  $scope.list[i] = i;
  $scope.get[i] = 0;
 }
 $scope.light = function (j) {
  for (i in $scope.get) {
   $scope.get[i] = j;
  }
 };
 var dmxGet = $interval(function () {
  $ola.get.Dmx($scope.Universe).then(function (data) {
   for (var i = 0; i < data.dmx.length; i++) {
    $scope.get[i] = data.dmx[i];
   }
   $scope.send = true;
  });
 }, 1000);
 $scope.getColor = function (i) {
  if (i > 140) {
   return 'black';
  } else {
   return 'white';
  }
 };
 $scope.ceil = function (i) {
  return $window.Math.ceil(i);
 };
 $scope.$watchCollection("get", function () {
  if ($scope.send) {
   $ola.post.Dmx($scope.Universe, $scope.get);
  }
 });
 $scope.page = function (d) {
  if (d === 1) {
   if (($scope.offset + 1) != $window.Math.ceil(Channels / $scope.limit)) {
    $scope.offset++;
   }
  } else if (d === nul) {
   if ($scope.offset !== nul) {
    $scope.offset--;
   }
  }
 };
 $scope.limit = ($window.Math.floor(($window.innerWidth * 0.99 ) / 66));
 $scope.width = {'width': ($window.Math.floor(($window.innerWidth * 0.99 ) / $scope.limit) - (52 / $scope.limit)) + 'px'};
 $window.$($window).resize(function () {
  $scope.$apply(function () {
   $scope.limit = $window.Math.floor(($window.innerWidth * 0.99 ) / 66);
   $scope.width = {width: (($window.Math.floor(($window.innerWidth * 0.99 ) / $scope.limit) - (52 / $scope.limit))) + 'px'};
  });
 });
 $scope.$on('$destroy', function () {
  $interval.cancel();
 });
}])
.controller('keypadUniverseCtrl', ['$scope', '$ola', '$routeParams', function ($scope, $ola, $routeParams) {
 "use strict";
 $scope.Universe = $routeParams.id;
}])
.controller('rdmUniverseCtrl', ['$scope', '$ola', '$routeParams', function ($scope, $ola, $routeParams) {
 "use strict";
 //get: /json/rdm/set_section_info?id={{id}}&uid={{uid}}&section={{section}}&hint={{hint}}&address={{address}}
 $scope.Universe = $routeParams.id;
}])
.controller('patchUniverseCtrl', ['$scope', '$ola', '$routeParams', function ($scope, $ola, $routeParams) {
 "use strict";
 $scope.Universe = $routeParams.id;
}])
.controller('settingUniverseCtrl', ['$scope', '$ola', '$routeParams', function ($scope, $ola, $routeParams) {
 "use strict";
 //post: /modify_universe
 //id:{{id}}
 //name:{{name}}
 //merge_mode:(LTP|HTP)
 //add_ports:{{port}}
 //remove_ports:{{port}}
 //{{port}}_priority_value:100
 //{{port}}_priority_mode:(inherit|static)
 //modify_ports:{{port}}
 $scope.Universe = $routeParams.id;
 $scope.PortsId = {};
 $ola.get.PortsId($routeParams.id).then(function (data) {
  $scope.PortsId = data;
 });
}])
.controller('infoPlugin', ['$scope', '$routeParams', '$ola', '$document', '$window', function ($scope, $routeParams, $ola, $document, $window) {
 "use strict";
 $ola.get.InfoPlugin($routeParams.id).then(function (data) {
  $scope.active = data.active;
  $scope.enabled = data.enabled;
  $scope.name = data.name;
  $document.getElementById('description').innerHTML = '';
  data.description.split('\\n').forEach(function (line) {
   $document.getElementById('description').innerText += line;
   $document.getElementById('description').innerHTML += '<br />';
  });
  $window.console.log(data);
 });
 $scope.stateColor = function (val) {
  if (val) {
   return {'background-color': 'green'};
  } else {
   return {'background-color': 'red'};
  }
 };
}])
.controller('infoPlugins', ['$scope', '$ola', '$location', function ($scope, $ola, $location) {
 "use strict";
 $scope.Items = {};
 $scope.active = [];
 $scope.enabled = [];
 $ola.get.ItemList().then(function (data) {
  $scope.Items = data;
  data.plugins.forEach(function (plugin) {
   $ola.get.InfoPlugin(plugin.id).then(function (data) {
    $scope.getStyleActive(data.active, plugin.id);
    $scope.getStyleEnabled(data.enabled, plugin.id);
   });
  });
 });
 $scope.go = function (id) {
  $location.path("/plugin/" + id);
 };
 $scope.getStyleActive = function (bool, id) {
  if (bool) {
   $scope.active[id] = {'background-color': 'green'};
  } else {
   $scope.active[id] = {'background-color': 'red'};
  }
 };
 $scope.getStyleEnabled = function (bool, id) {
  if (bool) {
   $scope.enabled[id] = {'background-color': 'green'};
  } else {
   $scope.enabled[id] = {'background-color': 'red'};
  }
 };
}])
.config(['$routeProvider', function ($routeProvider) {
 "use strict";
 $routeProvider.
 when('/', {
  templateUrl: '/views/overview.html',
  controller: 'overviewCtrl'
 }).
 when('/universes/', {
  templateUrl: '/views/overview-universes.html',
  controller: 'overviewCtrl'
 }).
 when('/universe/add', {
  templateUrl: '/views/add-universe.html',
  controller: 'addUniverseCtrl'
 }).
 when('/universe/:id', {
  templateUrl: '/views/overview-universe.html',
  controller: 'universeCtrl'
 }).
 when('/universe/:id/keypad', {
  templateUrl: '/views/keypad-universe.html',
  controller: 'keypadUniverseCtrl'
 }).
 when('/universe/:id/sliders', {
  templateUrl: '/views/sliders-universe.html',
  controller: 'sliderUniverseCtrl'
 }).
 when('/universe/:id/rdm', {
  templateUrl: '/views/rdm-universe.html',
  controller: 'rdmUniverseCtrl'
 }).
 when('/universe/:id/patch', {
  templateUrl: '/views/patch-universe.html',
  controller: 'patchUniverseCtrl'
 }).
 when('/universe/:id/settings', {
  templateUrl: '/views/settings-universe.html',
  controller: 'settingUniverseCtrl'
 }).
 when('/plugins', {
  templateUrl: '/views/info-plugins.html',
  controller: 'infoPlugins'
 }).
 when('/plugin/:id', {
  templateUrl: '/views/info-plugin.html',
  controller: 'infoPlugin'
 }).
 otherwise({
  redirectTo: '/'
 });
}])
.filter('startFrom', function () {
 "use strict";
 return function (input, start) {
  start = parseInt(start);
  return input.slice(start);
 };
});
