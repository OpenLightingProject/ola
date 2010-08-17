goog.require('goog.Timer');
goog.require('goog.debug.DivConsole');
goog.require('goog.debug.LogManager');
goog.require('goog.dom');
goog.require('goog.dom.ViewportSizeMonitor');
goog.require('goog.events');
goog.require('goog.math');
goog.require('goog.net.XhrIo');
goog.require('goog.net.XhrIoPool');
goog.require('goog.positioning.Corner');
goog.require('goog.ui.AnimatedZippy');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.CustomButton');
goog.require('goog.ui.Dialog');
goog.require('goog.ui.Popup');
goog.require('goog.ui.SplitPane');
goog.require('goog.ui.TabPane');
goog.require('goog.ui.SplitPane.Orientation');

//goog.require('ola.ControlList');

goog.provide('ola');

var ola = {}
ola.logger;

ola.PLUGIN_EVENT = 'plugin_change';
ola.PLUGIN_LIST_EVENT = 'plugin_list_change';
ola.SERVER_INFO_EVENT = 'server_info_change';
ola.UNIVERSE_LIST_EVENT = 'universe_list_change';
ola.UNIVERSE_EVENT = 'universe_change';

ola.HOME_FRAME_ID = 'home_frame';
ola.UNIVERSE_FRAME_ID = 'universe_frame';
ola.PLUGIN_FRAME_ID = 'plugin_frame';
ola.SPLIT_PANE_ID = 'split_pane';
ola.NEW_UNIVERSE_FRAME_ID = 'new_universe_frame';

ola.SERVER_INFO_URL = '/json/server_stats';
ola.PLUGIN_INFO_URL = '/json/plugin_info';
ola.UNIVERSE_INFO_URL = '/json/universe_info';
ola.PLUGIN_UNIVERSE_LIST_URL = '/json/universe_plugin_list';

ola.UNIVERSE_TAB_PANE_ID = 'universe_tab_pane';

ola.LIST_UPDATE_INTERVAL_MS = 5000;


/**
 * Setup the logger window
 * @constructor
 */
ola.LoggerWindow = function() {
  goog.debug.LogManager.getRoot().setLevel(goog.debug.Logger.Level.ALL);
  ola.logger = goog.debug.Logger.getLogger('ola');
  var logconsole = new goog.debug.DivConsole(goog.dom.$('log'));
  logconsole.setCapturing(true);

  this.log_control = goog.dom.$('log_control');

  var control = new goog.ui.Control("log_control");
  control.decorate(this.log_control);
  goog.events.listen(this.log_control, goog.events.EventType.CLICK,
                     this.Show, false, this);

  var popupElt = document.getElementById('log_popup');
  this.popup = new goog.ui.Popup(popupElt);
  this.popup.setHideOnEscape(true);
  this.popup.setAutoHide(true);
}


/**
 * Display the logger window
 */
ola.LoggerWindow.prototype.Show = function() {
  this.popup.setVisible(false);
  this.popup.setPinnedCorner(goog.positioning.Corner.TOP_RIGHT);
  var margin = new goog.math.Box(2, 2, 2, 2);
  this.popup.setMargin(margin);
  this.popup.setPosition(new goog.positioning.AnchoredViewportPosition(
    this.log_control,
    goog.positioning.Corner.BOTTOM_RIGHT));
  this.popup.setVisible(true);
}


/**
 * This event is fired when the server info changes
 */
ola.ServerInfoChangeEvent = function(server_info) {
  goog.events.Event.call(this, ola.SERVER_INFO_EVENT);
  this.server_info = server_info;
}
goog.inherits(ola.ServerInfoChangeEvent, goog.events.Event);


/**
 * This event is fired when the plugin list changes
 */
ola.PluginListChangeEvent = function(new_list) {
  goog.events.Event.call(this, ola.PLUGIN_LIST_EVENT);
  this.plugins = new_list;
}
goog.inherits(ola.PluginListChangeEvent, goog.events.Event);


/**
 * This event is fired when the universe list changes
 */
ola.UniverseListChangeEvent = function(new_list) {
  goog.events.Event.call(this, ola.UNIVERSE_LIST_EVENT);
  this.universes = new_list;
}
goog.inherits(ola.PluginListChangeEvent, goog.events.Event);


/**
 * This event is fired when the plugin info is available
 */
ola.PluginChangeEvent = function(plugin) {
  goog.events.Event.call(this, ola.PLUGIN_EVENT);
  this.plugin = plugin;
}
goog.inherits(ola.PluginChangeEvent, goog.events.Event);


/**
 * This event is fired when the universe info is available
 */
ola.UniverseChangeEvent = function(universe) {
  goog.events.Event.call(this, ola.UNIVERSE_EVENT);
  this.universe = universe;
}
goog.inherits(ola.UniverseChangeEvent, goog.events.Event);


/**
 * Create a new Server object, this is used to communicate with the OLA server
 * and fires events when the state changes.
 * @constructor
 */
ola.Server = function() {
  goog.events.EventTarget.call(this);
  this.pool = new goog.net.XhrIoPool({}, 1);
}
goog.inherits(ola.Server, goog.events.EventTarget);


/**
 * Update the server info data
 */
ola.Server.prototype.UpdateServerInfo = function() {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.ServerInfoChangeEvent(obj));
    this._CleanupRequest(e.target);
  }
  this._InitiateRequest(ola.SERVER_INFO_URL, on_complete);
}

ola.Server.prototype.ReloadPlugins = function() {

}

ola.Server.prototype.StopServer = function () {

}


/**
 * Fetch the list of plugins & universes active on the server
 */
ola.Server.prototype.FetchUniversePluginList = function() {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.PluginListChangeEvent(obj.plugins));
    this.dispatchEvent(new ola.UniverseListChangeEvent(obj.universes));
    this._CleanupRequest(e.target);
  }
  this._InitiateRequest(ola.PLUGIN_UNIVERSE_LIST_URL, on_complete);
}


/**
 * Fetch the info for a plugin
 */
ola.Server.prototype.FetchPluginInfo = function(plugin_id) {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.PluginChangeEvent(obj));
    this._CleanupRequest(e.target);
  }
  var url = ola.PLUGIN_INFO_URL + '?id=' + plugin_id;
  this._InitiateRequest(url, on_complete);
}


/**
 * Fetch the info for a universe
 */
ola.Server.prototype.FetchUniverseInfo = function(universe_id) {
  var on_complete = function(e) {
    var obj = e.target.getResponseJson();
    this.dispatchEvent(new ola.UniverseChangeEvent(obj));
    this._CleanupRequest(e.target);
  }
  var url = ola.UNIVERSE_INFO_URL + '?id=' + universe_id;
  this._InitiateRequest(url, on_complete);
}


/**
 * Initiate a JSON request
 * @param url the url to fetch
 * @param callback the callback to invoke when the request completes
 * @private
 */
ola.Server.prototype._InitiateRequest = function(url, callback) {
  var xhr = this.pool.getObject(undefined, 1);
  goog.events.listen(xhr, goog.net.EventType.COMPLETE, callback, false, this);
  xhr.send(url);
}


/**
 * Clean up from a request, this removes the listener and returns the channel
 * to the pool.
 * @private
 */
ola.Server.prototype._CleanupRequest = function(xhr) {
  goog.events.removeAll(xhr);
  this.pool.releaseObject(xhr);
}


/**
 * The base frame class
 * @param element_id the id of the div to use for the home frame
 * @constructor
 */
ola.BaseFrame = function(element_id) {
  this.element = goog.dom.$(element_id);
}


/**
 * Is this frame visible?
 */
ola.BaseFrame.prototype.IsVisible = function() {
  return this.element.style.display == 'block';
}


/**
 * Show this frame
 */
ola.BaseFrame.prototype.Show = function() {
  this.element.style.display = 'block';
}


/**
 * Hide this frame
 */
ola.BaseFrame.prototype.Hide = function() {
  this.element.style.display = 'none';
}


/**
 * A class representing the home frame
 * @param element_id the id of the div to use for the home frame
 * @constructor
 */
ola.HomeFrame = function(element_id, ola_server) {
  ola.BaseFrame.call(this, element_id);
  var reload_button = goog.dom.$('reload_button');
  goog.ui.decorate(reload_button);

  var stop_button = goog.dom.$('stop_button');
  goog.ui.decorate(stop_button);
  goog.events.listen(stop_button,
                     goog.events.EventType.CLICK,
                     this._StopButtonClicked,
                     false, this);

  var new_universe_button = goog.dom.$('new_universe_button');
  goog.ui.decorate(new_universe_button);

  goog.events.listen(reload_button,
                     goog.events.EventType.CLICK,
                     ola_server.FetchUniversePluginList,
                     false,
                     ola_server);

  goog.events.listen(ola_server, ola.SERVER_INFO_EVENT,
                     this._UpdateFromData,
                     false, this);
  goog.events.listen(ola_server, ola.UNIVERSE_LIST_EVENT,
                     this._UniverseListChanged,
                     false, this);
  ola_server.UpdateServerInfo();

  this.dialog = new goog.ui.Dialog(null, true);

  this.universe_list = new ola.ControlList(
      'universe_list',
      function(item) {
        var new_tr = goog.dom.createDom(
            'tr', {},
            goog.dom.createDom('td', {}, item.id.toString()),
            goog.dom.createDom('td', {}, item.name),
            goog.dom.createDom('td', {}, item.input_ports.toString()),
            goog.dom.createDom('td', {}, item.output_ports.toString()),
            goog.dom.createDom('td', {}, item.rdm_devices.toString()));
        return new_tr;
      },
      function(element, data) {
        var td = goog.dom.getFirstElementChild(element);
        td = goog.dom.getNextElementSibling(td);
        td.innerHTML = data.name;
        td = goog.dom.getNextElementSibling(td);
        td.innerHTML = data.input_ports.toString();
        td = goog.dom.getNextElementSibling(td);
        td.innerHTML = data.output_ports.toString();
        td = goog.dom.getNextElementSibling(td);
        td.innerHTML = data.rdm_devices.toString();
      });
}
goog.inherits(ola.HomeFrame, ola.BaseFrame);


/**
 * Update the home frame with new server data
 */
ola.HomeFrame.prototype._UpdateFromData = function(e) {
  goog.dom.$('server_hostname').innerHTML = e.server_info.hostname;
  goog.dom.$('server_ip').innerHTML = e.server_info.ip;
  goog.dom.$('server_version').innerHTML = e.server_info.version;
  goog.dom.$('server_uptime').innerHTML = e.server_info.up_since;
}


/**
 * Update the universe set
 */
ola.HomeFrame.prototype._UniverseListChanged = function(e) {
  this.universe_list.UpdateFromData(e.universes);
}


/**
 * Called when the stop button is clicked
 */
ola.HomeFrame.prototype._StopButtonClicked = function(e) {
  this.dialog.setTitle('Please confirm');
  this.dialog.setButtonSet(goog.ui.Dialog.ButtonSet.YES_NO);
  this.dialog.setContent('Are you sure? OLA may not be configured to restart '
                         + 'automatically');
  this.dialog.setVisible(true);
}


/**
 * The class representing the Universe frame
 * @constructor
 */
ola.UniverseFrame = function(element_id, ola_server) {
  ola.BaseFrame.call(this, element_id);
  this.ola_server = ola_server
  this.current_universe = undefined;
  goog.events.listen(ola_server, ola.UNIVERSE_EVENT,
                     this._UpdateFromData,
                     false, this);

  var tabPane = new goog.ui.TabPane(
    document.getElementById(ola.UNIVERSE_TAB_PANE_ID));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_1'), "Settings"));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_2'), 'RDM'));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_3'), 'Console'));
  tabPane.setSelectedIndex(0);
  this.selected_tab = 0;

  goog.events.listen(tabPane, goog.ui.TabPane.Events.CHANGE,
                     this.TabChanged, false, this);

  var save_button = goog.dom.$('universe_save_button');
  goog.ui.decorate(save_button);
}

goog.inherits(ola.UniverseFrame, ola.BaseFrame);


/**
 * Get the current selected universe
 */
ola.UniverseFrame.prototype.ActiveUniverse = function() {
  return this.current_universe;
}


/**
 * Show this frame. We extent the base method so we can populate the correct
 * tab.
 */
ola.UniverseFrame.prototype.Show = function(universe_id) {
  this.current_universe = universe_id;
  this._UpdateSelectedTab();
  ola.UniverseFrame.superClass_.Show.call(this);
}


/**
 * Called when the select tab changes
 */
ola.UniverseFrame.prototype.TabChanged = function(e) {
  this.selected_tab = e.page.getIndex();
  this._UpdateSelectedTab();
}

ola.UniverseFrame.prototype._UpdateSelectedTab = function() {
  if (this.selected_tab == 0) {
    this.ola_server.FetchUniverseInfo(this.current_universe);
  } else if (this.selected_tab == 1) {
    // update RDM
  }
}

/**
 * Update this universe frame from a Universe object
 */
ola.UniverseFrame.prototype._UpdateFromData = function(e) {
  this.current_universe = e.universe.id;
  goog.dom.$('universe_id').innerHTML = e.universe.id;
  goog.dom.$('universe_name').innerHTML = e.universe.name;
  goog.dom.$('universe_merge_mode').innerHTML = e.universe.merge_mode;
}



/**
 * The class representing the Universe frame
 * @constructor
 */
ola.NewUniverseFrame = function(element_id, ola_server) {
  ola.BaseFrame.call(this, element_id);
  this.ola_server = ola_server

  /*
  this.current_universe = undefined;
  goog.events.listen(ola_server, ola.UNIVERSE_EVENT,
                     this._UpdateFromData,
                     false, this);

  var tabPane = new goog.ui.TabPane(
    document.getElementById(ola.UNIVERSE_TAB_PANE_ID));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_1'), "Settings"));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_2'), 'RDM'));
  tabPane.addPage(new goog.ui.TabPane.TabPage(
    goog.dom.$('tab_page_3'), 'Console'));
  tabPane.setSelectedIndex(0);
  this.selected_tab = 0;

  goog.events.listen(tabPane, goog.ui.TabPane.Events.CHANGE,
                     this.TabChanged, false, this);

  var save_button = goog.dom.$('new_universe_save_button');
  goog.ui.decorate(save_button);
  */
}

goog.inherits(ola.NewUniverseFrame, ola.BaseFrame);


/**
 * Show this frame. We extent the base method so we can populate the correct
 * tab.
 */
ola.NewUniverseFrame.prototype.Show = function(universe_id) {
  ola.NewUniverseFrame.superClass_.Show.call(this);
}


/**
 * Update this universe frame from a Universe object
 */
ola.UniverseFrame.prototype._UpdateFromData = function(e) {
  this.current_universe = e.universe.id;
  goog.dom.$('universe_id').innerHTML = e.universe.id;
  goog.dom.$('universe_name').innerHTML = e.universe.name;
  goog.dom.$('universe_merge_mode').innerHTML = e.universe.merge_mode;
}


/**
 * The class representing the Plugin frame
 * @constructor
 */
ola.PluginFrame = function(element_id, ola_server) {
  ola.BaseFrame.call(this, element_id);
  goog.events.listen(ola_server, ola.PLUGIN_EVENT,
                     this._UpdateFromData,
                     false, this);
}

goog.inherits(ola.PluginFrame, ola.BaseFrame);


/**
 * Update this plugin frame from a Plugin object
 */
ola.PluginFrame.prototype._UpdateFromData = function(e) {
  goog.dom.$('plugin_description').innerHTML = e.plugin.description;
}


/**
 * Setup the OLA ola_ui widgets
 * @constructor
 */
ola.OlaUI = function(server) {
  this.ola_server = server;
  this.home_frame = new ola.HomeFrame(ola.HOME_FRAME_ID, server);
  this.universe_frame = new ola.UniverseFrame(ola.UNIVERSE_FRAME_ID, server);
  this.plugin_frame = new ola.PluginFrame(ola.PLUGIN_FRAME_ID, server);
  this.new_universe_frame = new ola.NewUniverseFrame(ola.NEW_UNIVERSE_FRAME_ID,
                                                     server);

  goog.events.listen(goog.dom.$('new_universe_button'),
                     goog.events.EventType.CLICK,
                     this._ShowNewUniverse,
                     false,
                     this);

  // setup the main split pane
  var lhs = new goog.ui.Component();
  var rhs = new goog.ui.Component();
  this.splitpane1 = new goog.ui.SplitPane(lhs, rhs,
      goog.ui.SplitPane.Orientation.HORIZONTAL);
  this.splitpane1.setInitialSize(150);
  this.splitpane1.setHandleSize(2);
  this.splitpane1.decorate(goog.dom.$(ola.SPLIT_PANE_ID));


  // show the main frame now
  goog.dom.$(ola.SPLIT_PANE_ID).style.display = 'block';

  // redraw on resize events
  this.vsm = new goog.dom.ViewportSizeMonitor();
  this._UpdateUI(goog.dom.getViewportSize());
  goog.events.listen(this.vsm, goog.events.EventType.RESIZE, this._UpdateUI,
    false, this);

  this._SetupNavigation();
  this._ShowHome();

}



//---------------------


/**
 * A ControlItemFactory that produces DivControlItems
 */
ola.DivControlItemFactory = function() {
}
goog.inherits(ola.DivControlItemFactory, ola.ControlItemFactory);

/**
 * Create a new DivControlItem
 * @returns a new DivControlItem from the data
 */
ola.ControlItemFactory.prototype.newItem = function(data) {
  return new ola.DivControlItem(data);
}


/**
 * A ControlItem that uses a div
 */
ola.DivControlItem = function(data) {
  this.div = goog.dom.createDom(
      'div',
      {'title': 'Universe ' + data.id},
      data.name);
  /*
  goog.events.listen(this.div,
                     goog.events.EventType.CLICK,
                     function() { ui._ShowUniverse(data.id); });
  */
}
goog.inherits(ola.DivControlItem, ola.ControlItem);

/**
 * Update this item with new data
 */
ola.DivControlItem.prototype.Update = function(new_data) {
  if (this.div.innerHTML != new_data.name) {
    this.div.innerHTML = new_data.name;
}

/**
 * Return the div for this ControlItem
 */
ola.DivControlItem.prototype.getElement = function() {
  this.div;
}

//---------------------

/**
 * Setup the navigation section of the UI
 * @private
 */
ola.OlaUI.prototype._SetupNavigation = function() {
  var home_control = goog.dom.$('home_control');
  goog.ui.decorate(home_control);
  goog.events.listen(home_control,
                     goog.events.EventType.CLICK,
                     this._ShowHome,
                     false,
                     this);

  var z1 = new goog.ui.AnimatedZippy('plugin_list_control',
                                     'plugin_container');
  var z2 = new goog.ui.AnimatedZippy('universe_list_control',
                                     'universe_container');

  // setup the plugin & universe lists
  var ui = this;
  this.plugin_list = new ola.ControlList(
      'plugin_container',
      new DivControlItemFactory());
  /*
      function(item) {
        var new_div = goog.dom.createDom(
            'div',
            {'title': item.name},
            item.name);
        goog.events.listen(new_div,
                           goog.events.EventType.CLICK,
                           function() { ui._ShowPlugin(item.id); });
        return new_div;
      },
      function(element, data) {
        if (element.innerHTML != data.name) {
          element.innerHTML = data.name;
        }
      });
  */
  goog.events.listen(this.ola_server,
                     ola.PLUGIN_LIST_EVENT,
                     function(e) { this.UpdateFromData(e.plugins); },
                     false, this.plugin_list);

  this.universe_list = new ola.ControlList(
      'universe_container',
      new DivControlItemFactory();
      /*
      function(item) {
        var new_div = goog.dom.createDom(
            'div',
            {'title': 'Universe ' + item.id},
            item.name);
        goog.events.listen(new_div,
                           goog.events.EventType.CLICK,
                           function() { ui._ShowUniverse(item.id); });
        return new_div;
      },
      function(element, data) {
        if (element.innerHTML != data.name) {
          element.innerHTML = data.name;
        }
      });
  */
  goog.events.listen(this.ola_server,
                     ola.UNIVERSE_LIST_EVENT,
                     this._UpdateUniverseList,
                     false, this);

  this.timer = new goog.Timer(ola.LIST_UPDATE_INTERVAL_MS);
  goog.events.listen(this.timer, goog.Timer.TICK,
                     function() { this.FetchUniversePluginList(); },
                     false,
                     this.ola_server);
  this.ola_server.FetchUniversePluginList();
  this.timer.start();
}


/**
 * Update universe list
 */
ola.OlaUI.prototype._UpdateUniverseList = function(e) {
  this.universe_list.UpdateFromData(e.universes);
  var active_universe = this.universe_frame.ActiveUniverse();
  var found = false;

  ola.logger.info('Got ' + e.universes.length + ' universes');
  for (var i = 0; i != e.universes.length; i++) {
    if (e.universes[i].id == active_universe) {
      found = true;
      break;
    }
  }

  if (this.universe_frame.IsVisible() && !found) {
    var dialog = new goog.ui.Dialog(null, true);
    dialog.setTitle('Universe ' + active_universe + ' Removed');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent('This universe has been removed by another user.');
    dialog.setVisible(true);
    this._ShowHome();
  }
}

/**
 * Display the home frame
 * @private
 */
ola.OlaUI.prototype._ShowHome = function() {
  this._HideAllFrames();
  this.home_frame.Show();
}


/**
 * Display the universe frame
 * @param universe_id the ID of the universe to load in the frame
 * @private
 */
ola.OlaUI.prototype._ShowUniverse = function(universe_id) {
  this._HideAllFrames();
  this.universe_frame.Show(universe_id);
}


/**
 * Display the new universe frame
 * @private
 */
ola.OlaUI.prototype._ShowNewUniverse = function() {
  this._HideAllFrames();
  this.new_universe_frame.Show();
}


/**
 * Display the plugin frame
 * @param plugin_id the ID of the plugin to show in the frame
 * @private
 */
ola.OlaUI.prototype._ShowPlugin = function(plugin_id) {
  this.ola_server.FetchPluginInfo(plugin_id);
  this._HideAllFrames();
  this.plugin_frame.Show();
}


/**
 * Hide all the frames
 * @private
 */
ola.OlaUI.prototype._HideAllFrames = function() {
  this.home_frame.Hide();
  this.universe_frame.Hide();
  this.plugin_frame.Hide();
  this.new_universe_frame.Hide();
}


/**
 * Update the UI size. This is called when the window size changes
 * @private
 */
ola.OlaUI.prototype._UpdateUI = function(size) {
  var size = this.vsm.getSize();
  this.splitpane1.setSize(new goog.math.Size(size.width, size.height - 80));
}


ola.Setup = function() {
  var logger_window = new ola.LoggerWindow();
  var server = new ola.Server();
  var ola_ui = new ola.OlaUI(server);
}
goog.exportSymbol('ola.Setup', ola.Setup);
