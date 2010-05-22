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
 * PluginManagerTest.cpp
 * Test fixture for the PluginManager classes
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "olad/Plugin.h"
#include "olad/PluginAdaptor.h"
#include "olad/PluginLoader.h"
#include "olad/PluginManager.h"
#include "olad/Preferences.h"
#include "olad/TestCommon.h"

using ola::AbstractPlugin;
using ola::PluginLoader;
using ola::PluginManager;
using std::string;
using std::vector;


class PluginManagerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PluginManagerTest);
  CPPUNIT_TEST(testPluginManager);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testPluginManager();

  private:
};


CPPUNIT_TEST_SUITE_REGISTRATION(PluginManagerTest);

/*
 * A Mock Loader
 */
class MockLoader: public ola::PluginLoader {
  public:
    MockLoader():
      PluginLoader() {
    }

    vector<AbstractPlugin*> LoadPlugins() {
      m_plugins.push_back(new TestMockPlugin(m_plugin_adaptor,
                                             ola::OLA_PLUGIN_ARTNET));
      m_plugins.push_back(new TestMockPlugin(m_plugin_adaptor,
                                             ola::OLA_PLUGIN_ESPNET,
                                             false));
      vector<AbstractPlugin*> plugins;
      vector<TestMockPlugin*>::iterator iter;
      for (iter = m_plugins.begin(); iter != m_plugins.end(); ++iter)
        plugins.push_back(*iter);
      return plugins;
    }
    void UnloadPlugins() {}

    void VerifyStartStates() {
      CPPUNIT_ASSERT_EQUAL((size_t) 2, m_plugins.size());
      CPPUNIT_ASSERT(m_plugins[0]->WasStarted());
      CPPUNIT_ASSERT(!m_plugins[1]->WasStarted());
    }

  private:
    vector<TestMockPlugin*> m_plugins;
};


/*
 * Check that we can load & unload plugins correctly.
 */
void PluginManagerTest::testPluginManager() {
  vector<PluginLoader*> loaders;
  MockLoader loader;
  loaders.push_back(&loader);

  ola::MemoryPreferencesFactory factory;
  ola::PluginAdaptor adaptor(NULL, NULL, &factory);
  PluginManager manager(loaders, &adaptor);

  manager.LoadAll();
  vector<AbstractPlugin*> plugins;
  manager.Plugins(&plugins);
  CPPUNIT_ASSERT_EQUAL((size_t) 2, plugins.size());
  CPPUNIT_ASSERT_EQUAL(string("foo"), plugins[0]->Name());
  CPPUNIT_ASSERT_EQUAL(string("foo"), plugins[1]->Name());

  loader.VerifyStartStates();

  manager.UnloadAll();
  manager.Plugins(&plugins);
  CPPUNIT_ASSERT_EQUAL((size_t) 0, plugins.size());
}
