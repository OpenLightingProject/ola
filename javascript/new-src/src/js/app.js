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
/*jshint browser: true */
/* global angular, $ */
angular
  .module('olaApp', ['ngRoute'])
  .constant('OLA', {
    'MIN_CHANNEL_NUMBER': 1,
    'MAX_CHANNEL_NUMBER': 512,
    'MIN_CHANNEL_VALUE': 0,
    'MAX_CHANNEL_VALUE': 255
  })
  .factory('$ola', ['$http', '$window', 'OLA',
    function($http, $window, OLA) {
      'use strict';
      // once olad supports json post data postEncode can go away
      // and the header in post requests too.
      var postEncode = function(data) {
        var PostData = [];
        for (var key in data) {
          if (data.hasOwnProperty(key)) {
            if (key === 'd' ||
              key === 'remove_ports' ||
              key === 'modify_ports' ||
              key === 'add_ports') {
              // this is here so some posts don't get broken
              // because of removed comma's in arrays
              PostData.push(key + '=' + data[key]);
            } else {
              PostData.push(key + '=' + encodeURIComponent(data[key]));
            }
          }
        }
        return PostData.join('&');
      };
      var dmxConvert = function(dmx) {
        var length = 1;
        var integers = [];
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
          ItemList: function() {
            return $http.get('/json/universe_plugin_list')
              .then(function(response) {
                return response.data;
              });
          },
          ServerInfo: function() {
            return $http.get('/json/server_stats')
              .then(function(response) {
                return response.data;
              });
          },
          Ports: function() {
            return $http.get('/json/get_ports')
              .then(function(response) {
                return response.data;
              });
          },
          PortsId: function(id) {
            return $http({
              method: 'GET',
              url: '/json/get_ports',
              params: {
                'id': id
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          InfoPlugin: function(id) {
            return $http({
              method: 'GET',
              url: '/json/plugin_info',
              params: {
                'id': id
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          Dmx: function(id) {
            return $http({
              method: 'GET',
              url: '/get_dmx',
              params: {
                'u': id
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          UniverseInfo: function(id) {
            return $http({
              method: 'GET',
              url: '/json/universe_info',
              params: {
                'id': id
              }
            })
              .then(function(response) {
                return response.data;
              });
          }
        },
        post: {
          ModifyUniverse: function(data) {
            return $http({
              method: 'POST',
              url: '/modify_universe',
              data: postEncode(data),
              headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
              }
            }).then(function(response) {
              return response.data;
            });
          },
          AddUniverse: function(data) {
            return $http({
              method: 'POST',
              url: '/new_universe',
              data: postEncode(data),
              headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
              }
            }).then(function(response) {
              return response.data;
            });
          },
          Dmx: function(universe, dmx) {
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
            }).then(function(response) {
              return response.data;
            });
          }
        },
        action: {
          Shutdown: function() {
            return $http.get('/quit')
              .then(function(response) {
                return response.data;
              });
          },
          Reload: function() {
            return $http.get('/reload')
              .then(function(response) {
                return response.data;
              });
          },
          ReloadPids: function() {
            return $http.get('/reload_pids')
              .then(function(response) {
                return response.data;
              });
          }
        },
        rdm: {
          // /json/rdm/section_info?id=[universe]&uid=[uid]&section=[section]
          GetSectionInfo: function(universe, uid, section) {
            return $http({
              method: 'GET',
              url: '/json/rdm/section_info',
              params: {
                'id': universe,
                'uid': uid,
                'section': section
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          // /json/rdm/set_section_info?id=[universe]&uid=[uid]&section=[section]
          SetSection: function(universe, uid, section, hint, option) {
            return $http({
              method: 'GET',
              url: '/json/rdm/set_section_info',
              params: {
                'id': universe,
                'uid': uid,
                'section': section,
                'hint': hint,
                'int': option
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          // /json/rdm/supported_pids?id=[universe]&uid=[uid]
          GetSupportedPids: function(universe, uid) {
            return $http({
              method: 'GET',
              url: '/json/rdm/supported_pids',
              params: {
                'id': universe,
                'uid': uid
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          // /json/rdm/supported_sections?id=[universe]&uid=[uid]
          GetSupportedSections: function(universe, uid) {
            return $http({
              method: 'GET',
              url: '/json/rdm/supported_sections',
              params: {
                'id': universe,
                'uid': uid
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          // /json/rdm/uid_identify_device?id=[universe]&uid=[uid]
          UidIdentifyDevice: function(universe, uid) {
            return $http({
              method: 'GET',
              url: '/json/rdm/uid_identify_device',
              params: {
                'id': universe,
                'uid': uid
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          // /json/rdm/uid_info?id=[universe]&uid=[uid]
          UidInfo: function(universe, uid) {
            return $http({
              method: 'GET',
              url: '/json/rdm/uid_info',
              params: {
                'id': universe,
                'uid': uid
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          // /json/rdm/uid_personalities?id=[universe]&uid=[uid]
          UidPersonalities: function(universe, uid) {
            return $http({
              method: 'GET',
              url: '/json/rdm/uid_personalities',
              params: {
                'id': universe,
                'uid': uid
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          // /json/rdm/uids?id=[universe]
          Uids: function(universe) {
            return $http({
              method: 'GET',
              url: '/json/rdm/uids',
              params: {
                'id': universe
              }
            })
              .then(function(response) {
                return response.data;
              });
          },
          // /rdm/run_discovery?id=[universe]&incremental=true
          RunDiscovery: function(universe, incremental) {
            return $http({
              method: 'GET',
              url: '/rdm/run_discovery',
              params: {
                'id': universe,
                'incremental': incremental
              }
            })
              .then(function(response) {
                return response.data;
              });
          }
        },
        tabs: function(tab, id) {
          // TODO(Dave_o): use a directive instead of this
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
        header: function(name, id) {
          $('div#header-universe').html(
            '<h4>' + name + '</h4><div>(id: ' + id + ')</div>'
          );  // TODO(Dave_o): use a directive instead of this
        },
        error: {
          modal: function(body, title) {
            if (typeof body !== 'undefined') {
              $('#errorModalBody').text(body);
            } else {
              $('#errorModalBody').text('There has been an error');
            }
            if (typeof title !== 'undefined') {
              $('#errorModalLabel').text(title);
            } else {
              $('#errorModalLabel').text('Error');
            }
            $('#errorModal').modal('show');
          }
        }
      };
    }
  ])
  .controller('menuCtrl', ['$scope', '$ola', '$interval', '$location',
    function($scope, $ola, $interval, $location) {
      'use strict';
      $scope.Items = {};
      $scope.Info = {};
      $scope.goTo = function(url) {
        $location.path(url);
      };
      var getData = function() {
        $ola.get.ItemList().then(function(data) {
          $scope.Items = data;
        });
        $ola.get.ServerInfo().then(function(data) {
          $scope.Info = data;
          document.title = data.instance_name + ' - ' + data.ip;
        });
      };
      getData();
      $interval(getData, 10000);
    }])
  .controller('overviewCtrl', ['$scope', '$ola', '$location',
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
    }])
  .controller('addUniverseCtrl', ['$scope', '$ola', '$window', '$location',
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
    }])
  .controller('universeCtrl',
  ['$scope', '$ola', '$routeParams', '$interval', 'OLA',
    function($scope, $ola, $routeParams, $interval, OLA) {
      'use strict';
      $ola.tabs('overview', $routeParams.id);
      $ola.get.UniverseInfo($routeParams.id).then(function(data) {
        $ola.header(data.name, $routeParams.id);
      });
      $scope.dmx = [];
      $scope.Universe = $routeParams.id;
      var interval = $interval(function() {
        $ola.get.Dmx($scope.Universe).then(function(data) {
          for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
            $scope.dmx[i] =
              (typeof data.dmx[i] === 'number') ?
                data.dmx[i] : OLA.MIN_CHANNEL_VALUE;
          }
        });
      }, 100);
      $scope.$on('$destroy', function() {
        $interval.cancel(interval);
      });
      $scope.getColor = function(i) {
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
    function($scope, $ola, $routeParams, $window, $interval, OLA) {
      'use strict';
      $ola.tabs('faders', $routeParams.id);
      $ola.get.UniverseInfo($routeParams.id).then(function(data) {
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
      $scope.light = function(j) {
        for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
          $scope.get[i] = j;
        }
        $scope.change();
      };
      var dmxGet = $interval(function() {
        $ola.get.Dmx($scope.Universe).then(function(data) {
          for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
            if (i < data.dmx.length) {
              $scope.get[i] = data.dmx[i];
            } else {
              $scope.get[i] = OLA.MIN_CHANNEL_VALUE;
            }
          }
          $scope.send = true;
        });
      }, 1000);
      $scope.getColor = function(i) {
        if (i > 140) {
          return 'black';
        } else {
          return 'white';
        }
      };
      $scope.ceil = function(i) {
        return $window.Math.ceil(i);
      };
      $scope.change = function() {
        $ola.post.Dmx($scope.Universe, $scope.get);
      };
      $scope.page = function(d) {
        if (d === 1) {
          var offsetLimit =
            $window.Math.ceil(OLA.MAX_CHANNEL_NUMBER / $scope.limit);
          if (($scope.offset + 1) !== offsetLimit) {
            $scope.offset++;
          }
        } else if (d === OLA.MIN_CHANNEL_VALUE) {
          if ($scope.offset !== OLA.MIN_CHANNEL_VALUE) {
            $scope.offset--;
          }
        }
      };
      $scope.getWidth = function() {
        var width =
          $window.Math.floor(($window.innerWidth * 0.99) / $scope.limit);
        var amount = width - (52 / $scope.limit);
        return amount + 'px';
      };
      $scope.getLimit = function() {
        var width = ($window.innerWidth * 0.99) / 66;
        return $window.Math.floor(width);
      };
      $scope.limit = $scope.getLimit();
      $scope.width = {
        'width': $scope.getWidth()
      };
      $window.$($window).resize(function() {
        $scope.$apply(function() {
          $scope.limit = $scope.getLimit();
          $scope.width = {
            width: $scope.getWidth()
          };
        });
      });
      $scope.$on('$destroy', function() {
        $interval.cancel(dmxGet);
      });
    }
  ])
  .controller('keypadUniverseCtrl', ['$scope', '$ola', '$routeParams',
    function($scope, $ola, $routeParams) {
      'use strict';
      $ola.tabs('keypad', $routeParams.id);
      $scope.Universe = $routeParams.id;
      $ola.get.UniverseInfo($routeParams.id).then(function(data) {
        $ola.header(data.name, $routeParams.id);
      });
    }])
  .controller('rdmUniverseCtrl', ['$scope', '$ola', '$routeParams',
    function($scope, $ola, $routeParams) {
      'use strict';
      $ola.tabs('rdm', $routeParams.id);
      //get:
      // /json/rdm/set_section_info?id={{id}}&uid={{uid}}&section={{section}}&hint={{hint}}&address={{address}}
      $scope.Universe = $routeParams.id;
      $ola.get.UniverseInfo($routeParams.id).then(function(data) {
        $ola.header(data.name, $routeParams.id);
      });
    }])
  .controller('patchUniverseCtrl', ['$scope', '$ola', '$routeParams',
    function($scope, $ola, $routeParams) {
      'use strict';
      $ola.tabs('patch', $routeParams.id);
      $scope.Universe = $routeParams.id;
      $ola.get.UniverseInfo($routeParams.id).then(function(data) {
        $ola.header(data.name, $routeParams.id);
      });
    }])
  .controller('settingUniverseCtrl', ['$scope', '$ola', '$routeParams',
    function($scope, $ola, $routeParams) {
      'use strict';
      $ola.tabs('settings', $routeParams.id);
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
          window.console.log(data);
          $ola.header(data.name, $routeParams.id);
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
        $ola.post.ModifyUniverse(a).then(function(data) {
          window.console.log(data);
        });
        $scope.loadData();
      };
    }])
  .controller('pluginInfoCtrl', ['$scope', '$routeParams', '$ola', '$window',
    function($scope, $routeParams, $ola, $window) {
      'use strict';
      $ola.get.InfoPlugin($routeParams.id).then(function(data) {
        $scope.active = data.active;
        $scope.enabled = data.enabled;
        $scope.name = data.name;
        var description = document.getElementById('description');
        description.textContent = data.description;
        description.innerHTML =
          description.innerHTML.replace(/\\n/g, '<br />');
        $window.console.log(data);
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
  ])
  .controller('pluginsCtrl', ['$scope', '$ola', '$location',
    function($scope, $ola, $location) {
      'use strict';
      $scope.Items = {};
      $scope.active = [];
      $scope.enabled = [];
      $scope.getInfo = function() {
        $ola.get.ItemList().then(function(data) {
          $scope.Items = data;
        });
      };
      $scope.getInfo();
      $scope.Reload = function() {
        $ola.action.Reload().then();
        $scope.getInfo();
      };
      $scope.go = function(id) {
        $location.path('/plugin/' + id);
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
    }])
  .config(['$routeProvider', function($routeProvider) {
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
  .filter('startFrom', function() {
    'use strict';
    return function(input, start) {
      start = parseInt(start, 10);
      return input.slice(start);
    };
  });
