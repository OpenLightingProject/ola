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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * PluginManagerTest.cpp
 * Test fixture for the PluginManager classes
 * Copyright (C) 2005-2009 Simon Newton
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
#include "olad/TestCommon.h"
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
    void VerifyPluginCounts(PluginManager *manager, size_t loaded_plugins,
                            size_t active_plugins, unsigned int line) {
      std::stringstream str;
      str << "Line " << line;
      vector<AbstractPlugin*> plugins;
      manager->Plugins(&plugins);
      CPPUNIT_ASSERT_EQUAL_MESSAGE(str.str(), loaded_plugins, plugins.size());

      plugins.clear();
      manager->ActivePlugins(&plugins);
      CPPUNIT_ASSERT_EQUAL_MESSAGE(str.str(), active_plugins, plugins.size());
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
  ola::PluginAdaptor adaptor(NULL, NULL, NULL, &factory, NULL);

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

  VerifyPluginCounts(&manager, 2, 1, __LINE__);

  OLA_ASSERT(plugin1.WasStarted());
  OLA_ASSERT_FALSE(plugin2.WasStarted());

  manager.UnloadAll();
  VerifyPluginCounts(&manager, 0, 0, __LINE__);
}


/*
 * Check that we detect conflicting plugins
 */
void PluginManagerTest::testConflictingPlugins() {
  ola::MemoryPreferencesFactory factory;
  ola::PluginAdaptor adaptor(NULL, NULL, NULL, &factory, NULL);

  set<ola::ola_plugin_id> conflict_set1;
  conflict_set1.insert(ola::OLA_PLUGIN_ARTNET);
  TestMockPlugin plugin1(&adaptor, ola::OLA_PLUGIN_DUMMY, conflict_set1);
  TestMockPlugin plugin2(&adaptor, ola::OLA_PLUGIN_ARTNET);
  set<ola::ola_plugin_id> conflict_set2;
  conflict_set2.insert(ola::OLA_PLUGIN_ARTNET);
  TestMockPlugin plugin3(&adaptor, ola::OLA_PLUGIN_SHOWNET, conflict_set2);

  vector<AbstractPlugin*> our_plugins;
  our_plugins.push_back(&plugin1);
  our_plugins.push_back(&plugin2);
  our_plugins.push_back(&plugin3);

  MockLoader loader(our_plugins);
  vector<PluginLoader*> loaders;
  loaders.push_back(&loader);

  PluginManager manager(loaders, &adaptor);
  OLA_INFO << "start";
  manager.LoadAll();

  VerifyPluginCounts(&manager, 3, 1, __LINE__);

  OLA_ASSERT_FALSE(plugin1.WasStarted());
  OLA_ASSERT(plugin2.WasStarted());
  OLA_ASSERT_FALSE(plugin3.WasStarted());

  manager.UnloadAll();
  VerifyPluginCounts(&manager, 0, 0, __LINE__);
}
