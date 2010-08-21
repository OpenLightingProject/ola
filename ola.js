goog.require('goog.Timer');
goog.require('goog.dom');
goog.require('goog.dom.ViewportSizeMonitor');
goog.require('goog.events');
goog.require('goog.math');
goog.require('goog.net.XhrIo');
goog.require('goog.net.XhrIoPool');
goog.require('goog.ui.AnimatedZippy');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.CustomButton');
goog.require('goog.ui.Dialog');
goog.require('goog.ui.SplitPane');
goog.require('goog.ui.TabPane');
goog.require('goog.ui.SplitPane.Orientation');

goog.require('ola.LoggerWindow');
goog.require('ola.SortedList');
goog.require('ola.Server');
goog.require('ola.Server.EventType');
goog.require('ola.HomeFrame');

goog.provide('ola');

var ola = ola || {}

ola.LIST_UPDATE_INTERVAL_MS = 5000;

ola.HOME_FRAME_ID = 'home_frame';
ola.UNIVERSE_FRAME_ID = 'universe_frame';
ola.PLUGIN_FRAME_ID = 'plugin_frame';
ola.SPLIT_PANE_ID = 'split_pane';
ola.NEW_UNIVERSE_FRAME_ID = 'new_universe_frame';
ola.UNIVERSE_TAB_PANE_ID = 'universe_tab_pane';


/**
 * The class representing the Universe frame
 * @constructor
 */
ola.UniverseFrame = function(element_id, ola_server) {
  ola.BaseFrame.call(this, element_id);
  this.ola_server = ola_server
  this.current_universe = undefined;
  goog.events.listen(ola_server, ola.Server.EventType.UNIVERSE_EVENT,
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
 * Show this frame. We extend the base method so we can populate the correct
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
  goog.events.listen(ola_server, ola.Server.EventType.UNIVERSE_EVENT,
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
  goog.events.listen(ola_server, ola.Server.EventType.PLUGIN_EVENT,
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


/**
 * The base class for an item in the control list
 */
ola.SortedListControl = function(data, callback, opt_renderer, opt_domHelper) {
  goog.ui.Control.call(this, data['name'], opt_renderer, opt_domHelper);
  this.id = data['id'];
  this.callback = callback;
};
goog.inherits(ola.SortedListControl, goog.ui.Control);


/**
 * Update this item with from new data
 */
ola.SortedListControl.prototype.Id = function() {
  return this.id
}


ola.SortedListControl.prototype.enterDocument = function() {
  goog.ui.Control.superClass_.enterDocument.call(this);
  this.getElement().title = 'Universe ' + this.Id();
  goog.events.listen(this.getElement(),
                     goog.events.EventType.CLICK,
                     function() { this.callback(this.id); },
                     false,
                     this);
};


/**
 * Update this item with from new data
 */
ola.SortedListControl.prototype.Update = function(new_data) {
  this.setContent(new_data['name']);
}


/**
 * The base class for a factory which produces control items
 * @class
 */
ola.ControlFactory = function(callback) {
  this.callback = callback;
}


/**
 * @returns an instance of a SortedListComponent
 */
ola.ControlFactory.prototype.newComponent = function(data) {
  return new ola.SortedListControl(data, this.callback);
}


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
  this.plugin_list = new ola.SortedList(
      'plugin_container',
      new ola.ControlFactory(function (id) { ui._ShowPlugin(id); }));

  goog.events.listen(this.ola_server,
                     ola.Server.EventType.PLUGIN_LIST_EVENT,
                     function(e) { this.UpdateFromData(e.plugins); },
                     false, this.plugin_list);

  this.universe_list = new ola.SortedList(
      'universe_container',
      new ola.ControlFactory(function(id) { ui._ShowUniverse(id); }));

  goog.events.listen(this.ola_server,
                     ola.Server.EventType.UNIVERSE_LIST_EVENT,
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
