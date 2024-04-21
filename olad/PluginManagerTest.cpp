/*
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
 * PluginManagerTest.cpp
 * Test fixture for the PluginManager classes
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>
#include <vector>

#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/PluginLoader.h"
#include "olad/PluginManager.h"
#include "olad/Preferences.h"
#include "olad/plugin_api/TestCommon.h"
#include "ola/testing/TestUtils.h"


using ola::AbstractPlugin;
using ola::PluginLoader;
using ola::PluginManager;
using std::set;
using std::string;
using std::vector;


class PluginManagerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PluginManagerTest);
  CPPUNIT_TEST(testPluginManager);
  CPPUNIT_TEST(testConflictingPlugins);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testPluginManager();
    void testConflictingPlugins();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

 private:
    void VerifyPluginCounts(PluginManager *manager,
                            size_t loaded_plugins,
                            size_t active_plugins,
                            const ola::testing::SourceLine &source_line) {
      vector<AbstractPlugin*> plugins;
      manager->Plugins(&plugins);
      ola::testing::_AssertEquals(source_line,
                                  loaded_plugins,
                                  plugins.size(),
                                  "Loaded plugin count differs");

      plugins.clear();
      manager->ActivePlugins(&plugins);
      ola::testing::_AssertEquals(source_line,
                                  active_plugins,
                                  plugins.size(),
                                  "Active plugin count differs");
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(PluginManagerTest);

/*
 * A Mock Loader
 */
class MockLoader: public ola::PluginLoader {
 public:
    explicit MockLoader(const vector<AbstractPlugin*> &plugins):
      PluginLoader(),
      m_plugins(plugins) {
    }

    vector<AbstractPlugin*> LoadPlugins() {
      return m_plugins;
    }
    void UnloadPlugins() {}

 private:
    vector<AbstractPlugin*> m_plugins;
};


/*
 * Check that we can load & unload plugins correctly.
 */
void PluginManagerTest::testPluginManager() {
  ola::MemoryPreferencesFactory factory;
  ola::PluginAdaptor adaptor(NULL, NULL, NULL, &factory, NULL, NULL, NULL);

  TestMockPlugin plugin1(&adaptor, ola::OLA_PLUGIN_ARTNET);
  TestMockPlugin plugin2(&adaptor, ola::OLA_PLUGIN_ESPNET, false);
  vector<AbstractPlugin*> our_plugins;
  our_plugins.push_back(&plugin1);
  our_plugins.push_back(&plugin2);

  MockLoader loader(our_plugins);
  vector<PluginLoader*> loaders;
  loaders.push_back(&loader);

  PluginManager manager(loaders, &adaptor);
  manager.LoadAll();

  VerifyPluginCounts(&manager, 2, 1, OLA_SOURCELINE());

  OLA_ASSERT(plugin1.IsRunning());
  OLA_ASSERT_FALSE(plugin2.IsRunning());

  manager.UnloadAll();
  VerifyPluginCounts(&manager, 0, 0, OLA_SOURCELINE());
}


/*
 * Check that we detect conflicting plugins
 */
void PluginManagerTest::testConflictingPlugins() {
  ola::MemoryPreferencesFactory factory;
  ola::PluginAdaptor adaptor(NULL, NULL, NULL, &factory, NULL, NULL, NULL);

  set<ola::ola_plugin_id> conflict_set1, conflict_set2, conflict_set3;
  conflict_set1.insert(ola::OLA_PLUGIN_ARTNET);
  TestMockPlugin plugin1(&adaptor, ola::OLA_PLUGIN_DUMMY, conflict_set1);
  TestMockPlugin plugin2(&adaptor, ola::OLA_PLUGIN_ARTNET);
  conflict_set2.insert(ola::OLA_PLUGIN_ARTNET);
  TestMockPlugin plugin3(&adaptor, ola::OLA_PLUGIN_SHOWNET, conflict_set2);

  conflict_set3.insert(ola::OLA_PLUGIN_DUMMY);
  TestMockPlugin plugin4(&adaptor, ola::OLA_PLUGIN_SANDNET, conflict_set3);

  vector<AbstractPlugin*> our_plugins;
  our_plugins.push_back(&plugin1);
  our_plugins.push_back(&plugin2);
  our_plugins.push_back(&plugin3);
  our_plugins.push_back(&plugin4);

  MockLoader loader(our_plugins);
  vector<PluginLoader*> loaders;
  loaders.push_back(&loader);

  PluginManager manager(loaders, &adaptor);
  manager.LoadAll();

  VerifyPluginCounts(&manager, 4, 2, OLA_SOURCELINE());

  OLA_ASSERT_TRUE(plugin1.IsRunning());
  OLA_ASSERT_FALSE(plugin2.IsRunning());
  OLA_ASSERT_TRUE(plugin3.IsRunning());

  // Try to enable the sandnet plugin, that conflicts with the dummy plugin
  OLA_ASSERT_FALSE(manager.EnableAndStartPlugin(ola::OLA_PLUGIN_SANDNET));
  VerifyPluginCounts(&manager, 4, 2, OLA_SOURCELINE());

  // Now disable the dummy plugin
  manager.DisableAndStopPlugin(ola::OLA_PLUGIN_DUMMY);
  VerifyPluginCounts(&manager, 4, 1, OLA_SOURCELINE());
  OLA_ASSERT_FALSE(plugin1.IsRunning());

  // Try to load sandnet again
  OLA_ASSERT_TRUE(manager.EnableAndStartPlugin(ola::OLA_PLUGIN_SANDNET));
  VerifyPluginCounts(&manager, 4, 2, OLA_SOURCELINE());
  OLA_ASSERT_TRUE(plugin4.IsRunning());

  manager.UnloadAll();
  VerifyPluginCounts(&manager, 0, 0, OLA_SOURCELINE());
}
