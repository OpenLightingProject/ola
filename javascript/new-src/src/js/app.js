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
angular
 .module('olaApp', ['ngRoute'])
 .constant('OLA', {
  'MIN_CHANNEL_NUMBER': 1,
  'MAX_CHANNEL_NUMBER': 512,
  'MIN_CHANNEL_VALUE': 0,
  'MAX_CHANNEL_VALUE': 255
 })
 .factory('$ola', ['$http', '$window', 'OLA',
  function ($http, $window, OLA) {
   'use strict';
   // once olad supports json post data postEncode can go away
   // and the header in post requests too.
   var postEncode = function (data) {
    var PostData = [];
    for (var key in data) {
     if (key === 'd') {
      // this is here so dmx posts dont get broken because of removed comma's
      PostData.push(key + '=' + data[key]);
     } else {
      PostData.push(key + '=' + encodeURIComponent(data[key]));
     }
    }
    return PostData.join('&');
   };
   var dmxConvert = function (dmx) {
    var length = 1,
     integers = [];
    for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
     integers[i] = parseInt(dmx[i], 10);
     if (integers[i] > OLA.MIN_CHANNEL_VALUE) {
      length = (i + 1);
     }
    }
    integers.length = length;
    return integers.join(',');
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
      return $http.get('/json/get_ports?u=' +
      id).then(function (response) {
       return response.data;
      });
     },
     InfoPlugin: function (id) {
      return $http.get('/json/plugin_info?id=' +
      id).then(function (response) {
       return response.data;
      });
     },
     Dmx: function (id) {
      return $http.get('/get_dmx?u=' +
      id).then(function (response) {
       return response.data;
      });
     },
     UniverseInfo: function (id) {
      return $http.get('/json/universe_info?id=' +
      id).then(function (response) {
       return response.data;
      });
     }
    },
    post: {
     ModifyUniverse: function (data) {
      return $http({
       method: 'POST',
       url: '/modify_universe',
       data: postEncode(data),
       headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
       }
      }).then(function (response) {
       return response.data;
      });
     },
     AddUniverse: function (data) {
      return $http({
       method: 'POST',
       url: '/new_universe',
       data: postEncode(data),
       headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
       }
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
       method: 'POST',
       url: '/set_dmx',
       data: postEncode(data),
       headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
       }
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
     // /json/rdm/section_info?id=[universe]&uid=[uid]&section=[section]
     GetSectionInfo: function (universe, uid, section) {
      var url = '/json/rdm/section_info?id=' + universe +
                '&uid=' +  uid + '&section=' + section;
      return $http.get(url).then(function (response) {
       return response.data;
      });
     },
     // /json/rdm/set_section_info?id=[universe]&uid=[uid]&section=[section]
     SetSection: function (universe, uid, section, hint, option) {
      var url = '/json/rdm/set_section_info?id=' + universe +
                '&uid=' + uid + '&section=' + section +
                '&hint=' + hint + '&int=' + option;
      return $http.get(url).then(function (response) {
       return response.data;
      });
     },
     // /json/rdm/supported_pids?id=[universe]&uid=[uid]
     GetSupportedPids: function (universe, uid) {
      var url = '/json/rdm/supported_pids?id=' + universe + '&uid=' + uid;
      return $http.get(url).then(function (response) {
       return response.data;
      });
     },
     // /json/rdm/supported_sections?id=[universe]&uid=[uid]
     GetSupportedSections: function (universe, uid) {
      var url = '/json/rdm/supported_sections?id=' + universe + '&uid=' + uid;
      return $http.get(url).then(function (response) {
       return response.data;
      });
     },
     // /json/rdm/uid_identify?id=[universe]&uid=[uid]
     UidIdentify: function (universe, uid) {
      var url = '/json/rdm/uid_identify?id=' + universe + '&uid=' + uid;
      return $http.get(url).then(function (response) {
       return response.data;
      });
     },
     // /json/rdm/uid_info?id=[universe]&uid=[uid]
     UidInfo: function (universe, uid) {
      var url = '/json/rdm/uid_info?id=' + universe + '&uid=' + uid;
      return $http.get(url).then(function (response) {
       return response.data;
      });
     },
     // /json/rdm/uid_personalities?id=[universe]&uid=[uid]
     UidPersonalities: function (universe, uid) {
      var url = '/json/rdm/uid_personalities?id=' + universe + '&uid=' + uid;
      return $http.get(url).then(function (response) {
       return response.data;
      });
     },
     // /json/rdm/uids?id=[universe]
     Uids: function (universe) {
      var url = '/json/rdm/uids?id=' + universe;
      return $http.get(url).then(function (response) {
       return response.data;
      });
     },
     // /rdm/run_discovery?id=[universe]&incremental=true
     RunDiscovery: function (universe, incremental) {
      var url = '/rdm/run_discovery?id=' + universe;
      if (incremental === true) {
       url = url + '&incremental=true';
      }
      return $http.get(url).then(function (response) {
       return response.data;
      });
     }
    },
    tabs: function (tab, id) {
     $window.$('ul#ola-nav-tabs').html('' +
     '<li id="overview" role="presentation"><a href="/new/#/universe/' +
     id +
     '/\">Overview</a></li>' +
     '<li id="faders" role="presentation"><a href="/new/#/universe/' +
     id +
     '/faders">Faders</a></li>' +
     '<li id="keypad" role="presentation"><a href="/new/#/universe/' +
     id +
     '/keypad">Keypad</a></li>' +
     '<li id="rdm" role="presentation"><a href="/new/#/universe/' +
     id +
     '/rdm">RDM</a></li>' +
     '<li id="patch" role="presentation"><a href="/new/#/universe/' +
     id +
     '/patch">RDM Patcher</a></li>' +
     '<li id="settings"role="presentation"><a href="/new/#/universe/' +
     id +
     '/settings">Settings</a></li>');
     $window.$('ul#ola-nav-tabs > li#' + tab).addClass('active');
    },
    header: function (name, id) {
     $('div#header-universe').html('<h4>' + name + '</h4><div>id: ' +
     id + '</div>');
    },
    error: {
     modal: function (body, title) {
      if (typeof body !== 'undefined') {
       $('#errorModalBody').text(body);
      }
      if (typeof title !== 'undefined') {
       $('#errorModalLabel').text(title);
      }
      $('#errorModal').modal('show');
     }
    }
   };
  }
 ])
 .controller('menuCtrl', ['$scope', '$ola', '$interval',
  function ($scope, $ola, $interval) {
   'use strict';
   $scope.Items = {};
   $scope.Info = {};
   $ola.get.ItemList().then(function (data) {
    $scope.Items = data;
   });
   $ola.get.ServerInfo().then(function (data) {
    $scope.Info = data;
    document.title = data.instance_name + ' - ' + data.ip;
   });
   $interval(function () {
    $ola.get.ItemList().then(function (data) {
     $scope.Items = data;
    });
    $ola.get.ServerInfo().then(function (data) {
     $scope.Info = data;
    });
   }, 10000);
  }])
 .controller('overviewCtrl', ['$scope', '$ola', '$location',
  function ($scope, $ola, $location) {
   'use strict';
   $scope.Info = {};
   $scope.Universes = {};
   $ola.get.ItemList().then(function (data) {
    $scope.Universes = data.universes;
   });
   $ola.get.ServerInfo().then(function (data) {
    $scope.Info = data;
   });
   $scope.Shutdown = function () {
    $ola.action.Shutdown().then();
   };
   $scope.goUniverse = function (id) {
    $location.path('/universe/' + id);
   };
  }])
 .controller('addUniverseCtrl', ['$scope', '$ola', '$window', '$location',
  function ($scope, $ola, $window, $location) {
   'use strict';
   $scope.Ports = {};
   $scope.addPorts = [];
   $scope.Universes = [];
   $scope.buttonState = true;
   $scope.Class = '';
   $scope.Data = {
    id: 0,
    name: '',
    add_ports: ''
   };
   $scope.$watchCollection('Data', function () {
    if ($scope.Universes.indexOf($scope.Data.id) !== -1 ||
        $scope.Data.name === '' ||
        $scope.Data.add_ports === '') {
     $scope.buttonState = true;
     return;
    }
    if ($scope.Data.id === null ||
        $scope.Data.name === null ||
        $scope.Data.add_ports === null) {
     $scope.buttonState = true;
     return;
    }
    if (typeof ($scope.Data.id) !== 'number' ||
        typeof ($scope.Data.name) !== 'string' ||
        typeof ($scope.Data.add_ports) !== 'string') {
     $scope.buttonState = true;
     return;
    }
    $scope.buttonState = false;
   });
   $ola.get.ItemList().then(function (data) {
    for (var u in data.universes) {
     if ($scope.Data.id === parseInt(data.universes[u].id, 10)) {
      $scope.Data.id++;
     }
     $scope.Universes.push(parseInt(data.universes[u].id, 10));
    }
   });
   $scope.Submit = function () {
    $ola.post.AddUniverse($scope.Data);
    $location.path('/universe/' + $scope.Data.id);
   };
   $ola.get.Ports().then(function (data) {
    $scope.Ports = data;
   });
   $scope.getDirection = function (bool) {
    if (bool) {
     return 'Output';
    } else {
     return 'Input';
    }
   };
   $scope.updateId = function () {
    if ($scope.Universes.indexOf($scope.Data.id) !== -1) {
     $scope.Class = 'has-error';
    } else {
     $scope.Class = '';
    }
   };
   $scope.TogglePort = function () {
    $scope.Data.add_ports =
     $window.$.grep($scope.addPorts, Boolean).join(',');
   };
  }])
 .controller('universeCtrl',
 ['$scope', '$ola', '$routeParams', '$interval', 'OLA',
  function ($scope, $ola, $routeParams, $interval, OLA) {
   'use strict';
   $ola.tabs('overview', $routeParams.id);
   $ola.get.UniverseInfo($routeParams.id).then(function (data) {
    $ola.header(data.name, $routeParams.id);
   });
   $scope.dmx = [];
   $scope.Universe = $routeParams.id;
   var interval = $interval(function () {
    $ola.get.Dmx($scope.Universe).then(function (data) {
     for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
      $scope.dmx[i] =
       (typeof data.dmx[i] === 'number') ? data.dmx[i] : OLA.MIN_CHANNEL_VALUE;
     }
    });
   }, 100);
   $scope.$on('$destroy', function () {
    $interval.cancel(interval);
   });
   $scope.getColor = function (i) {
    if (i > 140) {
     return 'black';
    } else {
     return 'white';
    }
   };
  }
 ])
 .controller('faderUniverseCtrl',
 ['$scope', '$ola', '$routeParams', '$window', '$interval', 'OLA',
  function ($scope, $ola, $routeParams, $window, $interval, OLA) {
   'use strict';
   $ola.tabs('faders', $routeParams.id);
   $ola.get.UniverseInfo($routeParams.id).then(function (data) {
    $ola.header(data.name, $routeParams.id);
   });
   $scope.get = [];
   $scope.list = [];
   $scope.last = 0;
   $scope.offset = 0;
   $scope.send = false;
   $scope.OLA = OLA;
   $scope.Universe = $routeParams.id;
   for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
    $scope.list[i] = i;
    $scope.get[i] = OLA.MIN_CHANNEL_VALUE;
   }
   $scope.light = function (j) {
    for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
     $scope.get[i] = j;
    }
    $scope.change();
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
   $scope.change = function () {
    $ola.post.Dmx($scope.Universe, $scope.get);
   };
   $scope.page = function (d) {
    if (d === 1) {
     var offsetLimit = $window.Math.ceil(OLA.MAX_CHANNEL_NUMBER / $scope.limit);
     if (($scope.offset + 1) !== offsetLimit) {
      $scope.offset++;
     }
    } else if (d === OLA.MIN_CHANNEL_VALUE) {
     if ($scope.offset !== OLA.MIN_CHANNEL_VALUE) {
      $scope.offset--;
     }
    }
   };
   $scope.getWidth = function () {
    var width = $window.Math.floor(($window.innerWidth * 0.99) / $scope.limit);
    var amount = width - (52 / $scope.limit);
    return amount + 'px';
   };
   $scope.getLimit = function () {
    var width = ($window.innerWidth * 0.99) / 66;
    return $window.Math.floor(width);
   };
   $scope.limit = $scope.getLimit();
   $scope.width = {
    'width': $scope.getWidth()
   };
   $window.$($window).resize(function () {
    $scope.$apply(function () {
     $scope.limit = $scope.getLimit();
     $scope.width = {
      width: $scope.getWidth()
     };
    });
   });
   $scope.$on('$destroy', function () {
    $interval.cancel(dmxGet);
   });
  }
 ])
 .controller('keypadUniverseCtrl', ['$scope', '$ola', '$routeParams',
  function ($scope, $ola, $routeParams) {
   'use strict';
   $ola.tabs('keypad', $routeParams.id);
   $scope.Universe = $routeParams.id;
   $ola.get.UniverseInfo($routeParams.id).then(function (data) {
    $ola.header(data.name, $routeParams.id);
   });
  }])
 .controller('rdmUniverseCtrl', ['$scope', '$ola', '$routeParams',
  function ($scope, $ola, $routeParams) {
   'use strict';
   $ola.tabs('rdm', $routeParams.id);
   //get:
   // /json/rdm/set_section_info?id={{id}}&uid={{uid}}&section={{section}}&hint={{hint}}&address={{address}}
   $scope.Universe = $routeParams.id;
   $ola.get.UniverseInfo($routeParams.id).then(function (data) {
    $ola.header(data.name, $routeParams.id);
   });
  }])
 .controller('patchUniverseCtrl', ['$scope', '$ola', '$routeParams',
  function ($scope, $ola, $routeParams) {
   'use strict';
   $ola.tabs('patch', $routeParams.id);
   $scope.Universe = $routeParams.id;
   $ola.get.UniverseInfo($routeParams.id).then(function (data) {
    $ola.header(data.name, $routeParams.id);
   });
  }])
 .controller('settingUniverseCtrl', ['$scope', '$ola', '$routeParams',
  function ($scope, $ola, $routeParams) {
   'use strict';
   $ola.tabs('settings', $routeParams.id);
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
   $scope.Info = {};
   $scope.PortsId = {};
   $scope.ActivePorts = {};
   $scope.Remove = [];
   $scope.Data = {};
   $ola.get.PortsId($routeParams.id).then(function (data) {
    $scope.PortsId = data;
   });
   $ola.get.UniverseInfo($routeParams.id).then(function (data) {
    $ola.header(data.name, $routeParams.id);
    $scope.Info = data;
    $scope.ActivePorts = data.output_ports.concat(data.input_ports);
    for (var i = 0; i < $scope.ActivePorts.length; i++) {
     $scope.Remove[i] = '';
    }
   });
  }])
 .controller('pluginInfoCtrl', ['$scope', '$routeParams', '$ola', '$window',
  function ($scope, $routeParams, $ola, $window) {
   'use strict';
   $ola.get.InfoPlugin($routeParams.id).then(function (data) {
    $scope.active = data.active;
    $scope.enabled = data.enabled;
    $scope.name = data.name;
    var description = document.getElementById('description');
    description.textContent = data.description;
    description.innerHTML =
     description.innerHTML.replace(/\\n/g,
      '<br />');
    $window.console.log(data);
   });
   $scope.stateColor = function (val) {
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
 ])
 .controller('pluginsCtrl', ['$scope', '$ola', '$location',
  function ($scope, $ola, $location) {
   'use strict';
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
   $scope.Reload = function () {
    $ola.action.Reload().then();
   };
   $scope.go = function (id) {
    $location.path('/plugin/' + id);
   };
   $scope.getStyleActive = function (bool, id) {
    if (bool) {
     $scope.active[id] = {
      'background-color': 'green'
     };
    } else {
     $scope.active[id] = {
      'background-color': 'red'
     };
    }
   };
   $scope.getStyleEnabled = function (bool, id) {
    if (bool) {
     $scope.enabled[id] = {
      'background-color': 'green'
     };
    } else {
     $scope.enabled[id] = {
      'background-color': 'red'
     };
    }
   };
  }])
 .config(['$routeProvider', function ($routeProvider) {
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
 }])
 .filter('startFrom', function () {
  'use strict';
  return function (input, start) {
   start = parseInt(start, 10);
   return input.slice(start);
  };
 });
