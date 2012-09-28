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
 * Copyright (C) 2012 Ravindra Nath Kakarla
 */

/**
 * RDMTests class
 */
RDMTests = function() {
  // init tabs
  $("#tabs").tabs({});

  // setup dialog windows, one for general notifications and one for the
  // download options.
  this.notification = $('#rdm-tests-notification');
  this.notification.dialog({
      autoOpen: false,
      closeOnEscape: false,
      dialogClass: 'no-close',
      draggable: false,
      height: 160,
      modal: true,
      resizable: false,
  });
  this.save_options = $('#rdm-tests-save-options');
  var save_buttons = [
    {
      text: "Cancel",
      click: function() { $(this).dialog("close"); }
    },
    {
      text: "Download",
      click: function() { rdmtests.save_results() }
    },
  ];
  this.save_options.dialog({
      autoOpen: false,
      draggable: false,
      height: 200,
      modal: true,
      resizable: false,
      title: 'Download Options',
      buttons: save_buttons,
  });
  this.error_notification = $('#rdm-error-notification');
  var error_buttons = [
    {
      text: "Dismiss",
      click: function() { $(this).dialog("close"); }
    },
    {
      text: "Report Bug",
      click: function() { rdmtests.report_bug() }
    },
  ];
  this.error_notification.dialog({
      autoOpen: false,
      buttons: error_buttons,
      draggable: true,
      height: 300,
      modal: true,
      resizable: true,
      width: 550,
  });

  //Populate the filter with test categories
  $('#rdm-tests-results-summary-filter-by_catg')
  .append($('<option />').val('All').html('All'));

  // and the other list in the download options dialog
  $('#rdm-tests-save-catg')
  .append($('<option />').val('All').html('All'));
};


/**
 * AJAX loader image
 * @this {RDMTests}
 */
RDMTests.ajax_loader = "<img src='/static/images/loader.gif' />";

/**
 * How long to wait between polls of the test status. In milliseconds
 * @this {RDMTests}
 */
RDMTests.poll_delay = 500;


/**
 * Maintains a list of all the tests along with their states, categories,
 * definitions with keys sorted in the order they are run.
 * @this {RDMTests}
 */
RDMTests.TEST_RESULTS = new Array();


rdmtests = undefined;


/**
 * Prepares the notification div and displays it on the page.
 * @this {RDMTests}
 * @param {Objec} options An object containing title and message to be
 * displayed.
 *
 *  {
 *    'title': 'Title to display on notification',
 *    'message': 'Notification Message',
 *    'is_dismissable': true,
 *    'buttons: [{'label': , 'on_click': {}],
 *  }
 */
RDMTests.prototype.set_notification = function(options) {
  var title = "";
  if (options.title != undefined || options.title != null) {
    title = options.title;
  }
  this.notification.dialog('option', 'title', title);

  var message = "";
  if (options.message != undefined || options.message != null) {
    message = options.message;
  }
  this.notification.html(message);

  var button_list = options.buttons || [];
  var buttons = []
  if (options.is_dismissable != undefined || options.is_dismissable != null) {
    buttons.push({
      text: "Ok",
      click: function() { $(this).dialog("close"); }
    });
  } else {
    for (var i = 0; i < button_list.length; ++i) {
      var button = button_list[i];
      buttons.push({
        text: button['label'],
        click: function() { button['on_click'](); },
      });
    }
  }
  this.notification.dialog('option', 'buttons', buttons);
  this.notification.dialog('open');
};


/**
 * Clears the notification text and hides it from page.
 * @this {RDMTests}
 */
RDMTests.prototype.clear_notification = function() {
  this.notification.html();
  this.notification.dialog('close');
};


/**
 * Binds click, keypress and other events to DOMs and
 * triggers the their respective handlers.
 * @this {RDMTests}
 */
RDMTests.prototype.bind_events_to_doms = function() {
  $('#universe_options').change(function() {
    rdmtests.update_device_list();
  });

  var discovery_button = $('#rdm-discovery-button').button();
  discovery_button.click(function() {
      rdmtests.run_discovery();
  });

  var run_tests_button = $('#rdm-tests-selection-run_tests').button();
  run_tests_button.click(function() {
      rdmtests.validate_form();
  });

  $('#rdm-tests-send_dmx_in_bg').change(function() {
    $('#rdm-tests-dmx_options').toggle('fast');
  });

  $(document).keydown(function(e) {
    var key = e.keyCode || e.which;
    var results_div = $('#rdm-tests-results');
    var test_frame = $('#tests_control_frame');

    if (results_div.css('display') == 'block' && key == 27) {
      results_div.hide();
      $('#tests_control_frame').show();
    }

    if (key == 13 && test_frame.css('display') == 'block') {
      rdmtests.validate_form();
    }
  });

  var dismiss_tests_button = $('#rdm-tests-results-button-dismiss').button();
  dismiss_tests_button.click(function() {
    $('#rdm-tests-results').hide();
    $('#tests_control_frame').show();
  });

  var run_again_button = $('#rdm-tests-results-button-run_again').button();
  run_again_button.click(function() {
    rdmtests.run_tests(rdmtests.selected_tests);
  });

  var download_options_button =
    $('#rdm-tests-results-button-save-options').button();
  download_options_button.click(function() {
    rdmtests.save_options.dialog('open');
  });

  var publisher_collect_button = $('#publisher-collect-button').button();
  publisher_collect_button.click(function() {
    rdmtests.collect_data();
  });

  var publisher_collect_button = $('#publisher-clear-button').button();
  publisher_collect_button.click(function() {
    $('#publisher-output').html('');
  });

  $.each([
    '#rdm-tests-results-summary-by_catg',
    '#rdm-tests-results-warnings',
    '#rdm-tests-results-advisories'
  ], function(i, div) {
    $(div).find('legend').click(function() {
      $(div).find('legend')
      .toggleClass('ola-expander-collapsed ola-expander-expanded');
      $(div).find('div').toggle();
    });
  });

  $.each([
    $('#rdm-tests-results-summary-filter-by_catg'),
    $('#rdm-tests-results-summary-filter-by_state')
  ], function(i, div) {
    $(div).change(function() {
      rdmtests.filter_results($('#rdm-tests-results-list'), {
        'category': $('#rdm-tests-results-summary-filter-by_catg').val(),
        'state': $('#rdm-tests-results-summary-filter-by_state').val()
      });
    });
  });
};


/**
 * Download the saved results.
 * @this {RDMTests}
 */
RDMTests.prototype.save_results = function() {
  this.save_options.dialog('close');
  var uid = $('#devices_list').val();
  var url = ('/DownloadResults?uid=' + uid + '&timestamp=' + this.timestamp +
             '&state=' + $('#rdm-tests-save-state').val() + '&category=' +
             $('#rdm-tests-save-catg').val());
  if ($('#rdm-tests-include-debug').attr('checked')) {
    url += '&debug=1';
  }
  $('#rdm-tests-download').attr('src', url);
};


/**
 * Prepares a list item with appropriate color, definition and value.
 * This will be appended to the results list.
 * @this {RDMTests}
 * @param {String} definition Key (or test definition) in TEST_RESULTS object.
 * @return {Object} A jQuery object representation for a prepared list item.
 */
RDMTests.prototype.make_results_list_item = function(definition) {
  var test_option = $('<option />').val(definition).text(definition);
  rdmtests.add_state_class(
                           RDMTests.TEST_RESULTS[definition]['state'],
                           test_option);
  return test_option;
};


/**
 * Filters definitions in results list by category and state of test result.
 * @this {RDMTests}
 * @param {Object} results_dom A jQuery object representation of HTML <ul>
 * which holds test result definitions.
 * @param {Object} filter_options An Object containing selected filter options.
 */
RDMTests.prototype.filter_results = function(results_dom, filter_options) {
  $(results_dom).html('');
  var filter_category = filter_options['category'];
  var filter_state = filter_options['state'];

  if (filter_category == 'All') {
    if (filter_state == 'All') {
      for (var definition in RDMTests.TEST_RESULTS) {
        $(results_dom).append(rdmtests.make_results_list_item(definition));
      }
    } else {
      for (definition in RDMTests.TEST_RESULTS) {
        if (RDMTests.TEST_RESULTS[definition]['state'] == filter_state) {
          $(results_dom).append(rdmtests.make_results_list_item(definition));
        }
      }
    }
  } else {
    if (filter_state == 'All') {
      for (definition in RDMTests.TEST_RESULTS) {
        if (RDMTests.TEST_RESULTS[definition]['category'] == filter_category) {
          $(results_dom).append(rdmtests.make_results_list_item(definition));
        }
      }
    } else {
      for (definition in RDMTests.TEST_RESULTS) {
        if (RDMTests.TEST_RESULTS[definition]['category'] == filter_category &&
            RDMTests.TEST_RESULTS[definition]['state'] == filter_state) {
          $(results_dom).append(rdmtests.make_results_list_item(definition));
        }
      }
    }
  }
};


/**
 * Sends an AJAX request to the RDM Tests Server and
 * triggers callback with the response.
 * Automatically displays ERROR notifications when a request fails.
 * @this {RDMTests}
 * @param {String} request Request string.
 * @param {Object} params An Object containing parameters to send to the server.
 * @param {Object} callback Handler to trigger.
 */
RDMTests.prototype.query_server = function(request, params, callback) {
  $.ajax({
    url: request,
    type: 'GET',
    data: params,
    dataType: 'json',
    success: function(data) {
      if (data['status'] == true) {
        callback(data);
      } else {
        rdmtests.clear_notification();
        rdmtests.set_notification({
          'title': 'Server Error',
          'message': data['error'],
          'is_dismissable': true
        });
      }
    },
    cache: false
  });
};


/**
 * Updates the universe selector with available universes fetched
 * from RDM Tests Server.
 */
RDMTests.prototype.update_universe_list = function() {
  var t = this;
  this.query_server('/GetUnivInfo', {},
                    function(data) { t.new_universe_list(data); });
};


/**
 * Called when we receive a list of universes. This updates the universe lists.
 */
RDMTests.prototype.new_universe_list = function(data) {
  var universes = data.universes;

  this._update_universe_select($('#universe_options'), universes);
  this._update_universe_select($('#publisher-universe-list'), universes);

  if (universes.length == 0) {
    rdmtests.set_notification({
      'title': 'No universes found',
      'message':'Go to the <a href="http://'+location.hostname+':9090" target="_blank">OLA Web UI</a> and patch a device to a universe',
      'buttons': [{'label': 'Retry',
                    'on_click': function() {
                      rdmtests.clear_notification();
                      rdmtests.update_universe_list()
                    },
                  }
                 ],
    });
  } else {
    rdmtests.update_device_list();
  }
};


/**
 * Triggers Full Discovery of devices via AJAX request
 * and updates the universe list.
 */
RDMTests.prototype.run_discovery = function() {
  var universe = $('#universe_options').val();
  if (universe == null || universe == undefined) {
    rdmtests.set_notification({
      'title': 'No universe selected',
      'message': 'Please select a universe to run discovery on.',
      'is_dismissable': true,
    });
    return;
  }

  rdmtests.set_notification({
    'title': 'Running Full Discovery',
    'message': RDMTests.ajax_loader
  });
  rdmtests.query_server('/RunDiscovery', {'u': universe}, function(data) {
    var devices_list = $('#devices_list');
    devices_list.empty();
    if (data['status'] == true) {
      var uids = data.uids;
      $.each(uids, function(item) {
        devices_list.append($('<option />').val(uids[item])
                              .text(uids[item]));
      });
      rdmtests.clear_notification();
    }
  });
};


/**
 * Updates the patched devices list for selected universe.
 */
RDMTests.prototype.update_device_list = function() {
  var universe_options = $('#universe_options');
  var devices_list = $('#devices_list');
  this.query_server('/GetDevices', {
    'u': universe_options.val() }, function(data) {
    if (data['status'] == true) {
      devices_list.empty();
      var uids = data.uids;
      $.each(uids, function(item) {
        devices_list.append($('<option />').val(uids[item]).text(uids[item]));
      });
    }
  });
};


/**
 * Fetches all Test Definitions available from the RDM Tests Server.
 * Initializes the multiselect widget with definitions.
 */
RDMTests.prototype.fetch_test_defs = function() {
  this.query_server('/GetTestDefs', {'c': 0}, function(data) {
    var tests_selector = $('#rdm-tests-selection-tests_list');
    var test_defs = data.test_defs;
    $.each(data.test_defs, function(item) {
      tests_selector.append($('<option />').val(test_defs[item])
                                           .text(test_defs[item]));
    });
    $('#rdm-tests-selection-tests_list').multiselect({
      sortable: false,
      searchable: true
    });
  });
};


/**
 * Triggers an AJAX call to run the tests with given Test Filter.
 * @param {Array} test_filter An array of tests to run.
 */
RDMTests.prototype.run_tests = function(test_filter) {
  this.set_notification({
    'title': 'Running ' + test_filter.length + ' tests',
    'message': '<div id="progressbar"></div>',
  });

  $('#progressbar').progressbar({});
  this.selected_tests = test_filter;

  this.query_server(
      '/RunTests',
      {
          'u': $('#universe_options').val(),
          'uid': $('#devices_list').val(),
          'w': $('#write_delay').val(),
          'f': ($('#rdm-tests-send_dmx_in_bg').attr('checked') ?
                $('#dmx_frame_rate').val() : 0),
          'c': $('#slot_count').val(),
          'c': ($('#rdm-tests-send_dmx_in_bg').attr('checked') ?
                $('#slot_count').val() : 128),
          't': test_filter.join(',')
      },
      function(data) {
        window.setTimeout(function() { rdmtests.stat_tests(); },
                          RDMTests.poll_delay);
      }
  );
};


/**
 * Check if the tests are complete yet.
 */
RDMTests.prototype.stat_tests = function() {
  this.query_server(
      '/StatTests',
      {},
      function(data) {
        rdmtests._stat_tests_response(data);
      });
};


/**
 * Resets the test results screen by clearing all the DOMs contents
 * and hiding them.
 */
RDMTests.prototype.reset_results = function() {
  $.each(['#rdm-tests-results-uid',
    '#rdm-tests-results-stats-figures',
    '#rdm-tests-results-summary-by_catg-content',
    '#rdm-tests-results-warnings-content',
    '#rdm-tests-results-advisories-content',
    '#rdm-tests-results-list'], function(i, dom) {
    $(dom).html('');
  });
  $('#rdm-tests-results-summary-filter-by_state').val('All');
  for (definition in RDMTests.TEST_RESULTS) {
    delete RDMTests.TEST_RESULTS[definition];
  }
};


/**
 * Adds CSS class to the DOM based on the given state.
 * This CSS class usually adds appropriate color to the text in the DOM.
 * @param {String} state A valid test result state
 * -- 'Passed', 'Failed', 'Broken', 'Not Run'.
 * @param {Object} dom A jQuery object representation of the DOM
 * to which the class needs to be added.
 */
RDMTests.prototype.add_state_class = function(state, dom) {
  $(dom).removeClass($(dom).attr('class'));
  switch (state) {
    case 'Passed':
      $(dom).addClass('test-state-passed');
      break;
    case 'Failed':
      $(dom).addClass('test-state-failed');
      break;
    case 'Broken':
      $(dom).addClass('test-state-broken');
      break;
    case 'Not Run':
      $(dom).addClass('test-state-not_run');
      break;
  }
};


/**
 * Initializes all the result screen DOMs and displays the results.
 * @param {Object} results The response Object from the RDM Tests Server.
 * which contains results.
 */
RDMTests.prototype.display_results = function(results) {
  $('#tests_control_frame').hide();
  rdmtests.reset_results();

  $('#rdm-tests-results-uid').html(results['UID']);

  for (key in results['stats']) {
    $('#rdm-tests-results-stats-figures')
    .append($('<td />').html(results['stats'][key]));
  }

  var category_lists = [$('#rdm-tests-results-summary-filter-by_catg'),
                        $('#rdm-tests-save-catg')];
  $.each(category_lists, function(i, dom) { dom.html(''); });

  // Summary of results by category
  for (key in results['stats_by_catg']) {
    var passed = results['stats_by_catg'][key]['passed'];
    var total = results['stats_by_catg'][key]['total'];
    if (total != 0) {
      var percent = '&nbsp;&nbsp;(' +
                    Math.ceil(passed / total * 100).toString() +
                    '%) </span>';
    } else {
      var percent = '&nbsp;&nbsp;- </span>';
    }

    $('#rdm-tests-results-summary-by_catg-content')
      .append($('<li />')
      .html('<span>' +
          key +
          '</span>' +
          '<span class="stats_by_catg">' +
          passed.toString() +
          '&nbsp;/&nbsp;' +
          total.toString() +
          percent));

    // update the lists as well
    $.each(category_lists,
           function(i, dom) {
             dom.append($('<option />').val(key).html(key));
           });
  }

  var number_of_warnings = 0;
  var number_of_advisories = 0;

  for (index in results['test_results']) {
    var warnings = results['test_results'][index]['warnings'];
    var advisories = results['test_results'][index]['advisories'];
    var definition = results['test_results'][index]['definition'];
    var state = results['test_results'][index]['state'];

    //Populating a global variable with test results for faster lookups
    RDMTests.TEST_RESULTS[definition] = results['test_results'][index];

    number_of_warnings += warnings.length;
    for (var i = 0; i < warnings.length; i++) {
      $('#rdm-tests-results-warnings-content')
      .append($('<li />')
      .html(definition + ': ' + warnings[i]));
    }
    number_of_advisories += advisories.length;
    for (var i = 0; i < advisories.length; i++) {
      $('#rdm-tests-results-advisories-content')
      .append($('<li />')
      .html(definition + ': ' + advisories[i]));
    }
    var test_option = $('<option />').val(definition).text(definition);

    rdmtests.add_state_class(state, test_option);

    $('#rdm-tests-results-list').append(test_option);
  }

  //Update the Warnings and Advisories counter
  $('#rdm-tests-results-warning-count').html(number_of_warnings.toString());
  $('#rdm-tests-results-advisory-count').html(number_of_advisories.toString());

  $('#rdm-tests-results-list').change(function() {
    rdmtests.result_list_changed();
  });

  // select the first item
  $('#rdm-tests-results-list').val(results['test_results'][0]['definition']);
  rdmtests.result_list_changed();
  $('#rdm-tests-results').show();

  //Hide/Show Download Logs button
  if (results['logs_disabled'] == true) {
    $('#rdm-tests-results-button-save-options').hide();
  } else {
    $('#rdm-tests-results-button-save-options').show();
  }
};


/**
 * This is triggered when a definition in results summary is selected.
 * The corresponding 'doc', 'debug' and category is updated in the info div.
 */
RDMTests.prototype.result_list_changed = function() {
  var definition = $('#rdm-tests-results-list option:selected').text();
  var state = RDMTests.TEST_RESULTS[definition]['state'];
  $('#rdm-tests-results-info-title').html(definition);
  rdmtests.add_state_class(state, $('#rdm-tests-results-info-state')
  .html(state));

  $('#rdm-tests-results-info-catg')
  .html(RDMTests.TEST_RESULTS[definition]['category']);

  $('#rdm-tests-results-info-doc')
  .html(RDMTests.TEST_RESULTS[definition]['doc']);

  var debug = RDMTests.TEST_RESULTS[definition]['debug'];
  $('#rdm-tests-results-info-debug').html(debug.join('<br />'));
};


/**
 * Validates the user input on Test Control Frame upon submission.
 * Checks for proper threshold values and other parameters.
 * @return {Boolean} True on successful validation.
 */
RDMTests.prototype.validate_form = function() {
  this.isNumberField = function(dom) {
    var value = $(dom).val();
    if (value === '') {
      return true;
    }
    if (isNaN(parseFloat(value)) || !isFinite(value)) {
      alert(
            $(dom).attr('id').replace('_', ' ').toUpperCase() +
            ' must be a number!'
      );
      return false;
    } else {
      return true;
    }
  };

  if ($('#devices_list option').size() < 1) {
    alert('There are no devices patched to the selected universe!');
    return false;
  }

  if (!(this.isNumberField($('#write_delay')) &&
        this.isNumberField($('#dmx_frame_rate')) &&
        this.isNumberField($('#slot_count')))) {
    return false;
  }

  var slot_count_val = parseFloat($('#slot_count').val());
  if (slot_count_val < 1 || slot_count_val > 512) {
    alert('Invalid number of slots (expected: [1-512])');
    return false;
  }

  var test_filter = ['all'];

  if ($('#rdm-tests-selection-subset').attr('checked')) {
    if ($('select[name="subset_test_defs"]').val() == null) {
      rdmtests.set_notification({
        'title': 'No tests specified',
        'message': "Add some tests by clicking the + sign, or click 'Add all'",
        'is_dismissable': true,
      });
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


/**
 * Collect information for the RDM devices present on a universe.
 */
RDMTests.prototype.collect_data = function() {
  this.query_server(
      '/RunCollector',
      { 'u': $('#publisher-universe-list').val(),
        'skip_queued': $('#publisher-skip-queued-messages').attr('checked') ?  true : false,
      },
      function(data) {
        window.setTimeout(function() { rdmtests.stat_collector(); },
                          RDMTests.poll_delay);
      }
  );
};


/**
 * Check if the collector is complete yet.
 */
RDMTests.prototype.stat_collector = function() {
  this.query_server(
      '/StatCollector',
      {},
      function(data) {
        rdmtests._stat_collector_response(data);
      });
};


/**
 * Handle the response from a /StatCollector call
 */
RDMTests.prototype._stat_collector_response = function(data) {
  if (data['completed']) {
    rdmtests.clear_notification();
    var exception = data['exception'];
    if (exception != undefined) {
      this.error_notification.dialog('option', 'title', 'Server Error');
      $('#error-message').html(exception)
      $('#traceback').html(data['traceback'])
      this.error_notification.dialog('open');
      this.exception = exception;
      this.traceback = data['traceback'];
      return;
    }
    var output = $('#publisher-output');
    output.html(data['output']);
  } else {
    window.setTimeout(function() { rdmtests.stat_collector()},
                      RDMTests.poll_delay);
  }
};


/**
 * Open the bug report window using the most recent exception & traceback.
 */
RDMTests.prototype.report_bug = function() {
  var comments = 'Error: ' + this.exception + '\n\n' + this.traceback;
  var url = (
      'http://code.google.com/p/open-lighting/issues/entry?' +
      'summary=Bug%20Report%20From%20RDM%20Tests' +
      '&comment=' + encodeURIComponent(comments));
  window.open(url, '_blank');
};


/**
 * Handle the response from a /StatTests call
 */
RDMTests.prototype._stat_tests_response = function(data) {
  if (data['completed']) {
    var exception = data['exception'];
    if (exception != undefined) {
      rdmtests.clear_notification();
      this.error_notification.dialog('option', 'title', 'Server Error');
      $('#error-message').html(exception)
      $('#traceback').html(data['traceback'])
      this.error_notification.dialog('open');
      this.exception = exception;
      this.traceback = data['traceback'];
      return;
    }
    this.timestamp = data['timestamp'];
    var failed_tests = $('#rdm-tests-selection-failed_tests');
    failed_tests.html('');
    var failed_defs = new Array();
    for (i in data['test_results']) {
      switch (data['test_results'][i]['state']) {
        case 'Failed':
          failed_defs.push(data['test_results'][i]['definition']);
          break;
      }
    }
    if ($(failed_tests).next().length > 0) {
      failed_tests.multiselect('destroy');
    }
    for (item in failed_defs) {
      failed_tests.append($('<option />')
                  .val(failed_defs[item])
                  .text(failed_defs[item]));
    }

    failed_tests.multiselect();
    rdmtests.clear_notification();
    rdmtests.display_results(data);
  } else {
    // update progress here
    var percent = data['tests_completed'] /  data['total_tests'] * 100;
    $('#progressbar').progressbar("option", "value", percent);
    window.setTimeout(function() { rdmtests.stat_tests()},
                      RDMTests.poll_delay);
  }
};


/**
 * Update a Universe <select> with new data.
 */
RDMTests.prototype._update_universe_select = function(select, universes) {
  select.empty();
  $.each(universes, function(i) {
    var text = universes[i]._name + ' (' + universes[i]._id + ')';
    select.append(
      $('<option />').val(universes[i]._id).text(text));
  });
};


$(document).ready(function() {
  rdmtests = new RDMTests();
  rdmtests.bind_events_to_doms();
  rdmtests.update_universe_list();
  rdmtests.fetch_test_defs();
});
