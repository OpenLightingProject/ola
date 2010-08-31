goog.require('goog.Timer');
goog.require('goog.dom');
goog.require('goog.dom.ViewportSizeMonitor');
goog.require('goog.events');
goog.require('goog.math');
goog.require('goog.ui.AnimatedZippy');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.ui.Control');
goog.require('goog.ui.Dialog');
goog.require('goog.ui.SplitPane');
goog.require('goog.ui.SplitPane.Orientation');

goog.require('ola.Dialog');
goog.require('ola.HomeFrame');
goog.require('ola.LoggerWindow');
goog.require('ola.NewUniverseFrame');
goog.require('ola.PluginFrame');
goog.require('ola.Server');
goog.require('ola.Server.EventType');
goog.require('ola.SortedList');
goog.require('ola.UniverseFrame');

goog.provide('ola.OlaUi');
goog.provide('ola.Setup');

var ola = ola || {};

ola.LIST_UPDATE_INTERVAL_MS = 5000;

ola.HOME_FRAME_ID = 'home_frame';
ola.UNIVERSE_FRAME_ID = 'universe_frame';
ola.PLUGIN_FRAME_ID = 'plugin_frame';
ola.SPLIT_PANE_ID = 'split_pane';
ola.NEW_UNIVERSE_FRAME_ID = 'new_universe_frame';


/**
 * Setup the OLA ola_ui widgets
 * @constructor
 */
ola.OlaUI = function() {
  this.logger_window = new ola.LoggerWindow();
  this.ola_server = ola.Server.getInstance();
  this.home_frame = new ola.HomeFrame(ola.HOME_FRAME_ID);
  this.universe_frame = new ola.UniverseFrame(ola.UNIVERSE_FRAME_ID, this);
  this.plugin_frame = new ola.PluginFrame(ola.PLUGIN_FRAME_ID, this.ola_server);
  this.new_universe_frame = new ola.NewUniverseFrame(ola.NEW_UNIVERSE_FRAME_ID,
                                                     this);

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
  this.splitpane1.setInitialSize(130);
  this.splitpane1.setHandleSize(2);
  this.splitpane1.decorate(goog.dom.$(ola.SPLIT_PANE_ID));

  // redraw on resize events
  this.vsm = new goog.dom.ViewportSizeMonitor();
  this._UpdateUI();
  goog.events.listen(this.vsm, goog.events.EventType.RESIZE, this._UpdateUI,
    false, this);

  this._SetupNavigation();
  this.ShowHome();

  // show the main frame now
  goog.dom.$(ola.SPLIT_PANE_ID).style.visibility = 'visible';
};


/**
 * The base class for an item in the control list
 * @constructor
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
  return this.id;
};


/**
 * Setup the event handler for this object.
 */
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
};


/**
 * The base class for a factory which produces control items
 * @constructor
 */
ola.ControlFactory = function(callback) {
  this.callback = callback;
};


/**
 * @returns an instance of a SortedListComponent
 */
ola.ControlFactory.prototype.newComponent = function(data) {
  return new ola.SortedListControl(data, this.callback);
};


/**
 * Setup the navigation section of the UI
 * @private
 */
ola.OlaUI.prototype._SetupNavigation = function() {
  var home_control = goog.dom.$('home_control');
  goog.ui.decorate(home_control);
  goog.events.listen(home_control,
                     goog.events.EventType.CLICK,
                     this.ShowHome,
                     false,
                     this);

  var z1 = new goog.ui.AnimatedZippy('plugin_list_control',
                                     'plugin_container');
  var z2 = new goog.ui.AnimatedZippy('universe_list_control',
                                     'universe_container');

  // setup the plugin & universe lists
  var ui = this;
  var plugin_container = new goog.ui.Container();
  plugin_container.decorate(goog.dom.$('plugin_container'));
  this.plugin_list = new ola.SortedList(
      plugin_container,
      new ola.ControlFactory(function(id) { ui._ShowPlugin(id); }));

  goog.events.listen(this.ola_server,
                     ola.Server.EventType.PLUGIN_LIST_EVENT,
                     function(e) { this.UpdateFromData(e.plugins); },
                     false, this.plugin_list);

  var universe_container = new goog.ui.Container();
  universe_container.decorate(goog.dom.$('universe_container'));
  this.universe_list = new ola.SortedList(
      universe_container,
      new ola.ControlFactory(function(id) { ui.ShowUniverse(id); }));

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
};


/**
 * Update universe list
 */
ola.OlaUI.prototype._UpdateUniverseList = function(e) {
  this.universe_list.UpdateFromData(e.universes);
  var active_universe = this.universe_frame.getActiveUniverse();
  var found = false;

  ola.logger.info('Got ' + e.universes.length + ' universes');
  for (var i = 0; i != e.universes.length; i++) {
    if (e.universes[i]['id'] == active_universe) {
      found = true;
      break;
    }
  }

  if (this.universe_frame.IsVisible() && !found) {
    var dialog = ola.Dialog.getInstance();
    dialog.setTitle('Universe ' + active_universe + ' Removed');
    dialog.setButtonSet(goog.ui.Dialog.ButtonSet.OK);
    dialog.setContent('This universe has been removed by another user.');
    dialog.setVisible(true);
    this.ShowHome();
  }
};


/**
 * Display the home frame
 * @private
 */
ola.OlaUI.prototype.ShowHome = function() {
  this._HideAllFrames();
  this.home_frame.Show();
};


/**
 * Display the universe frame
 * @param universe_id {number} the ID of the universe to load in the frame.
 * @param opt_select_main_tab {boolean} set to true to display the main tab.
 */
ola.OlaUI.prototype.ShowUniverse = function(universe_id, opt_select_main_tab) {
  this._HideAllFrames();
  this.universe_frame.Show(universe_id, opt_select_main_tab);
};


/**
 * Display the new universe frame
 * @private
 */
ola.OlaUI.prototype._ShowNewUniverse = function() {
  this._HideAllFrames();
  this.new_universe_frame.Show();
};


/**
 * Display the plugin frame
 * @param plugin_id the ID of the plugin to show in the frame.
 * @private
 */
ola.OlaUI.prototype._ShowPlugin = function(plugin_id) {
  this.ola_server.FetchPluginInfo(plugin_id);
  this._HideAllFrames();
  this.plugin_frame.Show();
};


/**
 * Hide all the frames
 * @private
 */
ola.OlaUI.prototype._HideAllFrames = function() {
  this.home_frame.Hide();
  this.universe_frame.Hide();
  this.plugin_frame.Hide();
  this.new_universe_frame.Hide();
};


/**
 * Update the UI size. This is called when the window size changes
 * @private
 */
ola.OlaUI.prototype._UpdateUI = function(e) {
  var size = this.vsm.getSize();
  this.splitpane1.setSize(new goog.math.Size(size.width, size.height - 80));
  this.logger_window.SetSize(size);
  this.universe_frame.SetSplitPaneSize();
};


ola.Setup = function() {
  var ola_ui = new ola.OlaUI();
};
goog.exportSymbol('ola.Setup', ola.Setup);
