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
 * 
 * Copyright (C) 2012 Ravindra Nath Kakarla
 */

RDMTests = function() {
};

rdmtests = new RDMTests();

RDMTests.prototype.bind_events_to_doms = function() {
  $('#universe_options').change(function() {
    rdmtests.update_device_list();
  });

  $('#rdm-tests-selection-run_tests').click(function() {
    rdmtests.validate_form();
  });

  $('#rdm-tests-send_dmx_in_bg').change(function() {
    $('#rdm-tests-dmx_options').toggle('fast');
  });
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
      $('#universe_options').empty();
      $.each(universes, function(item) {
        universe_options.append($('<option />').val(universes[item]._id)
                                .text(universes[item]._name));
      });
      rdmtests.update_device_list();
    }
  });
};

RDMTests.prototype.update_device_list = function() {
  var universe_options = $('#universe_options');
  var devices_list = $('#devices_list');
  this.query_server('../GetDevices', { 'u': universe_options.val() }, function(data) {
    if (data['status'] == true) {
      devices_list.empty();
      var uids = data.uids;
      $.each(uids, function(item) {
        devices_list.append($('<option />').val(uids[item])
                                .text(uids[item]));
      });
    }
  });
};

RDMTests.prototype.fetch_test_defs = function() {
  this.query_server('../GetTestDefs', {'c': 0}, function(data) {
    var tests_selector = $('#rdm-tests-selection-tests_list');
    var test_defs = data.test_defs;
    $.each(data.test_defs, function(item) {
      tests_selector.append($('<option />').val(test_defs[item])
                                           .text(test_defs[item]));
    });
    $('#rdm-tests-selection-tests_list').multiselect();
    $('#rdm-tests-selection-tests_list').multiselect({sortable: false, searchable: true});
  });
};

RDMTests.prototype.run_tests = function(test_filter) {
  var ajax_loader = $('#rdm-tests-ajax_loader');
  ajax_loader.show();
  this.query_server('../RunTests', {
                                     'u': $(universe_options).val(),
                                     'uid': $(devices_list).val(),
                                     'w': $(write_delay).val(),
                                     'f': $(dmx_frame_rate).val(),
                                     'c': $(slot_count).val(),
                                     't': test_filter.join(','),
                                    }, function(data) {
                                    var failed_tests = $('#rdm-tests-selection-failed_tests');
                                    var failed_defs = new Array();
                                    for (i in data['test_results']) {
                                      switch (data['test_results'][i].state) {
                                        case 'Failed':
                                        case 'Broken':
                                        case 'Not Run':
                                          failed_defs.push(i);
                                          break;
                                      }
                                    }
                                    for (item in failed_defs) {
                                      failed_tests.append($('<option />').val(failed_defs[item]).text(failed_defs[item]));
                                    }
                                    failed_tests.multiselect();
                                    ajax_loader.hide();
  });
};

RDMTests.prototype.validate_form = function() {
  this.isNumberField = function(dom) {
    var value = $(dom).val();
    if (value === '') {
      return true;
    }
    if (isNaN(parseFloat(value)) || !isFinite(value)) {
      alert(dom.attr('id').replace('_', ' ').toUpperCase() + ' must be a number!');
      return false;
    } else {
      return true;
    }
  };

  if ($('#devices_list option').size() < 1) {
    alert('There are no devices available in selected universe!');
    return false;
  }

  if (!(this.isNumberField($(write_delay)) &&
        this.isNumberField($(dmx_frame_rate)) &&
        this.isNumberField($(slot_count)))) {
    return false;
  }

  var slot_count_val = parseFloat($(slot_count).val());
  if (slot_count_val < 1 || slot_count_val > 512) {
    alert('Invalid number of slots (expected: [1-512])');
    return false;
  }

  var test_filter = ['all'];

  if ($('#rdm-tests-selection-subset').attr('checked')) {
    if ($('select[name="subset_test_defs"]').val() == null) {
      alert('No tests were selected!');
      return false;
    } else {
      test_filter = $('select[name="subset_test_defs"]').val();
    }
  } else if ($('#rdm-tests-selection-previously_failed').attr('checked')) {
    if ($('select[name="failed_test_defs"]').val() == null) {
      alert('Select failed tests to run again!');
      return false;
    } else {
      test_filter = $('select[name="failed_test_defs"]').val();
    }
  }

  rdmtests.run_tests(test_filter);
};

$(document).ready(function() {
  rdmtests.bind_events_to_doms();
  rdmtests.update_universe_list();
  rdmtests.fetch_test_defs();
});
