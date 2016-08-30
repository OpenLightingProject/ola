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
