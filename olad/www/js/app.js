/**
* Created by dave on 11-1-15.
*/
angular
.module('olaApp', ['ngRoute'])
.factory('$ola', ['$http', function ($http) {
  var postEncode = function(data) {
    var PostData = [];
    for (var key in data) {
      PostData.push(key + '=' + data[key]);
    }
    return PostData.join('&');
  };
  var $ola = {
    get: {
      ItemList: function () {
        var promise = $http.get('/json/universe_plugin_list').then(function (response) {
          return response.data;
        });
        return promise;
      },
      ServerInfo: function () {
        var promise = $http.get('/json/server_stats').then(function (response) {
          return response.data;
        });
        return promise;
      },
      Ports: function () {
        var promise = $http.get('/json/get_ports').then(function (response) {
          return response.data;
        });
        return promise;
      },
      PortsId: function (id) {
        var promise = $http.get('/json/get_ports?u=' + id).then(function (response) {
          return response.data;
        });
        return promise;
      },
      InfoPlugin: function (id) {
        var promise = $http.get('/json/plugin_info?id=' + id).then(function (response) {
          return response.data;
        });
        return promise;
      },
      Dmx: function (id) {
        var promise = $http.get('/get_dmx?u=' + id).then(function (response) {
          return response.data;
        });
        return promise;
      },
      UniverseInfo: function(id) {
        var promise = $http.get('/json/universe_info?id=' + id).then(function (response) {
          return response.data;
        });
        return promise;
      }
    },
    post: {
      ModifyUniverse: function(data) {
        var promise = $http({
          method: "POST",
          url:'/modify_universe',
          data: postEncode(data),
          headers:{'Content-Type': 'application/x-www-form-urlencoded'}
        }).then(function (respose) {
          return respose.data;
        });
        return promise;
      },
      AddUniverse: function(data){
        var promise = $http({
          method: "POST",
          url: '/new_universe',
          data: postEncode(data),
          headers:{'Content-Type': 'application/x-www-form-urlencoded'}
        }).then(function (response){
          return response.data;
        });
        return promise;
      },
      Dmx: function(universe, dmx){
        var data = {
          u: universe,
          d: ''
        };
        var length = 0;
        for(var i = 0; i<512; i++){
          dmx[i] = parseInt(dmx[i]);
          if(dmx[i] !== 0){
            length = i;
          }
        }
        dmx.length = length;
        if(dmx.length === 0){
          data.d = '[]';
        }else{
          data.d = '['+dmx.join(',')+']';
        }
        var promise = $http({
          method: "POST",
          url: '/set_dmx',
          data: postEncode(data),
          headers:{'Content-Type': 'application/x-www-form-urlencoded'}
        }).then(function(response){
          return response.data;
        });
        return promise;
      }
    },
    action: {
      Shutdown: function() {
        var promise = $http.get('/quit').then(function (response){
          return response.data;
        });
        return promise;
      },
      Reload: function(){
        var promise = $http.get('/reload').then(function (response){
          return response.data;
        });
        return promise;
      },
      ReloadPlugins: function() {
        var promise = $http.get('/reload_pids').then(function (response){
          return response.data;
        });
        return promise;
      }
    },
    rdm:{

    }
  };
  return $ola;
}])
.controller('menuCtrl', ['$scope', '$ola', function ($scope, $ola) {
  $scope.Items = {};

  $ola.get.ItemList().then(function (data) {
    $scope.Items = data;
  });
}])
.controller('overviewCtrl', ['$scope', '$ola', function ($scope, $ola) {
  $scope.Info = {};
  $scope.Universes = {};

  $ola.get.ItemList().then(function (data) {
    $scope.Universes = data.universes;
  });
  $ola.get.ServerInfo().then(function (data) {
    $scope.Info = data;
  });
}])
.controller('addUniverseCtrl', ['$scope', '$ola', function ($scope, $ola) {
  //post : /new_universe
  //id:{{id}}
  //name:{{name}}
  //add_ports:{{ports}}

  $scope.Ports = {};
  $ola.get.Ports().then(function (data) {
    $scope.Ports = data;
  });
}])
.controller('universeCtrl', ['$scope', '$ola', '$routeParams', function ($scope, $ola, $routeParams) {
  $scope.dmx = [];
  $scope.Universe = $routeParams.id;
  $ola.get.Dmx($scope.Universe).then(function (data) {
    $scope.dmx = data.dmx;
  });
}])
.controller('sliderUniverseCtrl', ['$scope', '$ola', '$routeParams', '$window', '$interval', function ($scope, $ola, $routeParams, $window, $interval) {
  $scope.get = [];
  $scope.list = [];
  $scope.last = 0;
  $scope.offset = 0;
  $scope.Universe = $routeParams.id;
  for (var i = 0; i < 512; i++) {
    $scope.list[i] = i;
  }
  $scope.light = function (j) {
    for (i in $scope.get) {
      $scope.get[i] = j;
    }
  };
  $scope.light(0);
  var dmxGet = $interval(function(){
    $ola.get.Dmx($scope.Universe).then(function (data) {
      for(var i = 0; i<data.dmx.length;i++){
        $scope.get[i] = (typeof(data.dmx[i]) === 'number') ? parseInt(data.dmx[i]) : 0;
      }
      $window.console.log(data);
    });
  }, 1000);
  $scope.getColor = function(i){
    if(i>140){
      return 'black';
    }else{
      return 'white';
    }
  };
  $scope.ceil = function(i){
    return $window.Math.ceil(i);
  };
  $scope.$watchCollection("get", function () {
    $ola.post.Dmx($scope.Universe, $scope.get);
  });
  $scope.up = function () {
    if (($scope.offset + 1) != $window.Math.ceil(512 / $scope.limit)) {
      $scope.offset++;
    }
  };
  $scope.down = function () {
    if ($scope.offset !== 0) {
      $scope.offset--;
    }
  };
  $scope.limit = ($window.Math.floor(($window.innerWidth * 0.99 )/ 66));
  $scope.width = {'width': ($window.Math.floor(($window.innerWidth * 0.99 ) / $scope.limit) - (52 / $scope.limit)) + 'px'};

  $window.$(window).resize(function () {
    $scope.$apply(function () {
      $scope.limit = $window.Math.floor(($window.innerWidth * 0.99 ) / 66);
      $scope.width = {width: (($window.Math.floor(($window.innerWidth * 0.99 ) / $scope.limit) - (52 / $scope.limit))) + 'px'};
    });
  });
  $scope.$on('$destroy', function() {
    $interval.cancel(dmxGet);
  });

}])
.controller('keypadUniverseCtrl', ['$scope', '$ola', '$routeParams',function($scope, $ola, $routeParams){
  $scope.Universe = $routeParams.id;
}])
.controller('rdmUniverseCtrl', ['$scope', '$ola', '$routeParams', function ($scope, $ola, $routeParams) {
  //get: /json/rdm/set_section_info?id={{id}}&uid={{uid}}&section={{section}}&hint={{hint}}&address={{address}}
  $scope.Universe = $routeParams.id;
}])
.controller('patchUniverseCtrl', ['$scope', '$ola', '$routeParams', function ($scope, $ola, $routeParams) {
  $scope.Universe = $routeParams.id;

}])
.controller('settingUniverseCtrl', ['$scope', '$ola', '$routeParams', function ($scope, $ola, $routeParams) {
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
.controller('infoPlugin', ['$scope', '$routeParams', '$ola', function ($scope, $routeParams, $ola) {
  $scope.InfoPlugin = {};
  $ola.get.InfoPlugin($routeParams.id).then(function (data) {
    $scope.InfoPlugin = data;
  });
}])
.controller('infoPlugins', ['$scope', '$ola', function ($scope, $ola) {

}])
.config(['$routeProvider', function ($routeProvider) {
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
  when('/plugin/status', {
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
}]).filter('startFrom', function () {
  return function (input, start) {
    start = parseInt(start); //parse to int
    return input.slice(start);
  };
});
