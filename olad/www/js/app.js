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
  var dmxConvert = function(dmx){
    var length = 0, ints = [];
    for(var i = 0; i<512; i++){
      ints[i] = parseInt(dmx[i]);
      if(ints[i] > 0){
        length = (i+1);
      }
    }
    ints.length = length;
    return '['+ints.join(',')+']';
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
          d: dmxConvert(dmx)
        };
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
      ReloadPids: function() {
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
.controller('universeCtrl', ['$scope', '$ola', '$routeParams', '$interval', function ($scope, $ola, $routeParams, $interval) {
  $scope.dmx = [];
  $scope.Universe = $routeParams.id;
  var dmxGet = $interval(function(){
    $ola.get.Dmx($scope.Universe).then(function (data) {
      for(var i = 0; i<512;i++){
        $scope.dmx[i] = (typeof(data.dmx[i]) === "number") ? data.dmx[i] : 0;
      }
    });
  }, 100);
  $scope.$on('$destroy', function() {
    $interval.cancel(dmxGet);
  });
  $scope.getColor = function(i){
    if(i>140){
      return 'black';
    }else{
      return 'white';
    }
  };
}])
.controller('sliderUniverseCtrl', ['$scope', '$ola', '$routeParams', '$window', '$interval', function ($scope, $ola, $routeParams, $window, $interval) {
  $scope.get = [];
  $scope.list = [];
  $scope.last = 0;
  $scope.offset = 0;
  $scope.send = false;
  $scope.Universe = $routeParams.id;
  for (var i = 0; i < 512; i++) {
    $scope.list[i] = i;
    $scope.get[i] = 0;
  }

  $scope.light = function (j) {
    for (i in $scope.get) {
      $scope.get[i] = j;
    }
  };

  var dmxGet = $interval(function(){
    $ola.get.Dmx($scope.Universe).then(function (data) {
      for(var i = 0; i<data.dmx.length;i++){
        $scope.get[i] = data.dmx[i];
      }
      $scope.send = true;
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
    if($scope.send){
      $ola.post.Dmx($scope.Universe, $scope.get);
    }
  });
  $scope.page = function (d) {
    if(d === 1){
      if (($scope.offset + 1) != $window.Math.ceil(512 / $scope.limit)) {
        $scope.offset++;
      }
    }else if(d === 0){
      if ($scope.offset !== 0) {
        $scope.offset--;
      }
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
  $ola.get.InfoPlugin($routeParams.id).then(function (data) {
    $scope.active = data.active;
    $scope.enabled = data.enabled;
    $scope.name = data.name;
    document.getElementById('description').innerHTML = '';
    data.description.split('\\n').forEach(function (line){
      document.getElementById('description').innerText += line;
      document.getElementById('description').innerHTML += '<br />';
    });
    console.log(data);
  });

  $scope.stateColor = function(val){
    if(val){
      return{'background-color': 'green'};
    }else{
      return{'background-color': 'red'};
    }
  };

}])
.controller('infoPlugins', ['$scope', '$ola', '$location', function ($scope, $ola, $location) {
  $scope.Items = {};
  $scope.active = [];
  $scope.enabled = [];

  $ola.get.ItemList().then(function (data) {
    $scope.Items = data;
    data.plugins.forEach(function(plugin){
      $ola.get.InfoPlugin(plugin.id).then(function(data) {
        $scope.getStyleActive(data.active, plugin.id);
        $scope.getStyleEnabled(data.enabled, plugin.id);
      });
    });
  });

  $scope.go = function(id){
    $location.path("/plugin/" + id);
  };

  $scope.getStyleActive = function(bool, id){
    if(bool){
      $scope.active[id] = {'background-color': 'green'};
    }else{
      $scope.active[id] = {'background-color': 'red'};
    }
  };

  $scope.getStyleEnabled = function(bool, id){
    if(bool){
      $scope.enabled[id] = {'background-color': 'green'};
    }else{
      $scope.enabled[id] = {'background-color': 'red'};
    }
  };

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
  return function (input, start) {
    start = parseInt(start); //parse to int
    return input.slice(start);
  };
});
