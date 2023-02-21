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

/*jshint browser: true, jquery: true*/
/* global ola */
ola.controller('menuCtrl', ['$scope', '$ola', '$interval', '$location',
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
  }]);

/*jshint browser: true, jquery: true*/
/* global ola */
ola.controller('patchUniverseCtrl',
  ['$scope', '$ola', '$routeParams',
    function($scope, $ola, $routeParams) {
      'use strict';
      $scope.Universe = $routeParams.id;

      //TODO(Dave_o): implement me!
    }
  ]);

/*jshint browser: true, jquery: true*/
/* global ola */
ola.controller('rdmUniverseCtrl',
  ['$scope', '$ola', '$routeParams',
    function($scope, $ola, $routeParams) {
      'use strict';
      //get:
      // /json/rdm/set_section_info?id={{id}}&uid={{uid}}&section={{section}}&hint={{hint}}&address={{address}}
      $scope.Universe = $routeParams.id;
    }
  ]);

/*jshint browser: true, jquery: true*/
/* global ola */
ola.controller('universeCtrl',
  ['$scope', '$ola', '$routeParams', '$interval', 'OLA',
    function($scope, $ola, $routeParams, $interval, OLA) {
      'use strict';
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
  ]);

/*jshint browser: true, jquery: true*/
/* global ola */
ola.controller('faderUniverseCtrl',
  ['$scope', '$ola', '$routeParams', '$window', '$interval', 'OLA',
    function($scope, $ola, $routeParams, $window, $interval, OLA) {
      'use strict';
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
        var pageCount = $scope.getPageCount();
        var offset = $scope.offset + d;
        if (offset + 1 > pageCount) {
          offset -= pageCount;
        } else if (offset < 0) {
          offset += pageCount;
        }
        $scope.offset = offset;
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

      $scope.getPageCount = function() {
        var count = OLA.MAX_CHANNEL_NUMBER / $scope.limit;
        return $window.Math.ceil(count);
      };

      $scope.limit = $scope.getLimit();

      $scope.width = {
        'width': $scope.getWidth()
      };

      $window.$($window).resize(function() {
        $scope.$apply(function() {
          $scope.limit = $scope.getLimit();
          var pageCount = $scope.getPageCount();
          if ($scope.offset + 1 > pageCount) {
            $scope.offset = pageCount - 1;
          }
          $scope.width = {
            width: $scope.getWidth()
          };
        });
      });

      $scope.$on('$destroy', function() {
        $interval.cancel(dmxGet);
      });
    }
  ]);

/*jshint browser: true, jquery: true*/
/* global ola */
ola.controller('keypadUniverseCtrl',
  ['$scope', '$ola', '$routeParams', 'OLA',
    function($scope, $ola, $routeParams, OLA) {
      'use strict';
      $scope.Universe = $routeParams.id;
      var regexkeypad;
      /*jscs:disable maximumLineLength */
      /*jshint ignore:start */
      // The following regex doesn't fit in the 80 character limit so to avoid
      // errors from the linters they are disabled on this part
      regexkeypad =
        /^(?:([0-9]{1,3})(?:\s(THRU)\s(?:([0-9]{1,3}))?)?(?:\s(@)\s(?:([0-9]{1,3}|FULL))?)?)/;
      /*jscs:enable maximumLineLength */
      /* jshint ignore:end */

      var check = {
        channelValue: function(value) {
          return OLA.MIN_CHANNEL_VALUE <= value &&
            value <= OLA.MAX_CHANNEL_VALUE;
        },
        channelNumber: function(value) {
          return OLA.MIN_CHANNEL_NUMBER <= value &&
            value <= OLA.MAX_CHANNEL_NUMBER;
        },
        regexGroups: function(result) {
          if (result[1] !== undefined) {
            var check1 = this.channelNumber(parseInt(result[1], 10));
            if (!check1) {
              return false;
            }
          }
          if (result[3] !== undefined) {
            var check2 = this.channelNumber(parseInt(result[3], 10));
            if (!check2) {
              return false;
            }
          }
          if (result[5] !== undefined && result[5] !== 'FULL') {
            var check3 = this.channelValue(parseInt(result[5], 10));
            if (!check3) {
              return false;
            }
          }
          return true;
        }
      };

      $scope.field = '';
      $scope.input = function(input) {
        var tmpField;
        if (input === 'backspace') {
          tmpField = $scope.field.substr(0, $scope.field.length - 1);
        } else {
          tmpField = $scope.field + input;
        }
        var fields = regexkeypad.exec(tmpField);
        if (fields === null) {
          $scope.field = '';
        } else if (check.regexGroups(fields)) {
          $scope.field = fields[0];
        }
        $scope.focusInput = true;
      };

      $scope.keypress = function($event) {
        var key = $event.key;

        // don't handle keyboard shortcuts and function keys
        if ($event.altKey || $event.ctrlKey || $event.metaKey ||
            ($event.which === 0 && key !== 'Enter' && key !== 'Backspace')) {
          // $event.which is 0 for non-printable keys (like the F1 - F12 keys)
          return;
        }

        $event.preventDefault();

        switch (key) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            $scope.input(key);
            break;
          case '@':
          case 'a':
            $scope.input(' @ ');
            break;
          case '>':
          case 't':
            $scope.input(' THRU ');
            break;
          case 'f':
            $scope.input('FULL');
            break;
          case 'Backspace':
            $scope.input('backspace');
            break;
          case 'Enter':
            $scope.submit();
            break;
        }
      };

      $scope.submit = function() {
        $scope.focusInput = true;

        var dmx = [];
        var input = $scope.field;
        var result = regexkeypad.exec(input);
        if (result !== null && check.regexGroups(result)) {
          var begin = parseInt(result[1], 10);
          var end = result[3] ? parseInt(result[3], 10) :
            parseInt(result[1], 10);
          var value = (result[5] === 'FULL') ?
            OLA.MAX_CHANNEL_VALUE : parseInt(result[5], 10);
          if (begin <= end && check.channelValue(value)) {
            $ola.get.Dmx($scope.Universe).then(function(data) {
              for (var i = 0; i < OLA.MAX_CHANNEL_NUMBER; i++) {
                if (i < data.dmx.length) {
                  dmx[i] = data.dmx[i];
                } else {
                  dmx[i] = OLA.MIN_CHANNEL_VALUE;
                }
              }
              for (var j = begin; j <= end; j++) {
                dmx[j - 1] = value;
              }
              $ola.post.Dmx($scope.Universe, dmx);
              $scope.field = '';
            });
            return true;
          } else {
            return false;
          }
        } else {
          return false;
        }
      };

      $scope.focusInput = true;
    }
  ]);

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

/*jshint browser: true, jquery: true*/
/* global ola */
ola.controller('pluginInfoCtrl',
  ['$scope', '$routeParams', '$ola', '$sce', 'marked',
    function($scope, $routeParams, $ola, $sce, marked) {
      'use strict';
      $ola.get.InfoPlugin($routeParams.id).then(function(data) {
        $scope.active = data.active;
        $scope.enabled = data.enabled;
        $scope.name = data.name;
        $scope.description = $sce.trustAsHtml(
          marked(data.description.replace(/\\n/g, '\n'))
        );
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

/*jshint browser: true, jquery: true */
/* global ola */
ola.controller('headerControl',
  ['$scope', '$ola', '$routeParams', '$window',
    function($scope, $ola, $routeParams, $window) {
      'use strict';
      $scope.header = {
        tab: '',
        id: $routeParams.id,
        name: ''
      };

      $ola.get.UniverseInfo($routeParams.id)
        .then(function(data) {
          $scope.header.name = data.name;
        });

      var hash = $window.location.hash;
      $scope.header.tab = hash.replace(/#\/universe\/[0-9]+\/?/, '');
    }
  ]);

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

/*jshint browser: true, jquery: true*/
/* global ola */
ola.constant('OLA', {
  'MIN_CHANNEL_NUMBER': 1,
  'MAX_CHANNEL_NUMBER': 512,
  'MIN_CHANNEL_VALUE': 0,
  'MAX_CHANNEL_VALUE': 255
});

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

/*jshint browser: true, jquery: true*/
/* global ola */
// TODO(Dave_o): split this up further
ola.factory('$ola', ['$http', '$window', 'OLA',
  function($http, $window, OLA) {
    'use strict';
    // TODO(Dave_o): once olad supports json post data postEncode
    // can go away and the header in post requests too.
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
    var channelValueCheck = function(i) {
      i = parseInt(i, 10);
      if (i < OLA.MIN_CHANNEL_VALUE) {
        i = OLA.MIN_CHANNEL_VALUE;
      } else if (i > OLA.MAX_CHANNEL_VALUE) {
        i = OLA.MAX_CHANNEL_VALUE;
      }
      return i;
    };
    var dmxConvert = function(dmx) {
      var strip = true;
      var integers = [];
      for (var i = OLA.MAX_CHANNEL_NUMBER; i >= OLA.MIN_CHANNEL_NUMBER; i--) {
        var value = channelValueCheck(dmx[i - 1]);
        if (value > OLA.MIN_CHANNEL_VALUE ||
          !strip ||
          i === OLA.MIN_CHANNEL_NUMBER) {
          integers[i - 1] = value;
          strip = false;
        }
      }
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
        },
        PluginState: function(pluginId, state) {
          var data = {
            state: state,
            plugin_id: pluginId
          };
          return $http({
            method: 'POST',
            url: '/set_plugin_state',
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
]);

/*jshint browser: true, jquery: true*/
/* global ola */
ola.filter('startFrom', function() {
  'use strict';
  return function(input, start) {
    start = parseInt(start, 10);
    return input.slice(start);
  };
});
