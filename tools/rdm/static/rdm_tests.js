/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RDMTests interacts with TestServer
 * Copyright (C) 2012 Ravindra Nath Kakarla
 */

RDMTests = function() {
};

RDMTests.prototype.query_server = function(request, params, callback) {
  $.ajax({
    url: request, 
    type: 'GET',
    data: params, 
    dataType: 'json',
    success: function(data) {
      callback(data);
    },
    cache: false
  });
};

RDMTests.prototype.update_universe_list = function() {
  this.query_server('../GetUnivInfo', {}, function(data) {
    if (data['status'] == true) {
      var universes = data.universes;
      var universe_options = $('#universe_options');
      $.each(universes, function(item) {
        universe_options.append($('<option />').val(universes[item]._id)
                                .text(universes[item]._name));
      });
    }
  });
};


$(document).ready(function() {
  rdmtests = new RDMTests();
  rdmtests.update_universe_list();
});
