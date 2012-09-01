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
 * CredentialsTest.cpp
 * Test the credentials functions.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/base/Credentials.h"
#include "ola/Logging.h"

using ola::GetEGID;
using ola::GetEUID;
using ola::GetGID;
using ola::GetGroupGID;
using ola::GetGroupName;
using ola::GetPasswdName;
using ola::GetPasswdUID;
using ola::GetUID;
using ola::GroupEntry;
using ola::PasswdEntry;
using ola::SetGID;
using ola::SetUID;

class CredentialsTest: public CppUnit::TestFixture {
  public:
    CPPUNIT_TEST_SUITE(CredentialsTest);
    CPPUNIT_TEST(testGetUIDs);
    CPPUNIT_TEST(testGetGIDs);
    CPPUNIT_TEST(testSetUID);
    CPPUNIT_TEST(testSetGID);
    CPPUNIT_TEST(testGetPasswd);
    CPPUNIT_TEST(testGetGroup);
    CPPUNIT_TEST_SUITE_END();

  public:
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
    void testGetUIDs();
    void testGetGIDs();
    void testSetUID();
    void testSetGID();
    void testGetPasswd();
    void testGetGroup();
};


CPPUNIT_TEST_SUITE_REGISTRATION(CredentialsTest);


/**
 * Check we're not running as root.
 */
void CredentialsTest::testGetUIDs() {
  uid_t uid = GetUID();
  CPPUNIT_ASSERT_MESSAGE("Don't run the tests as root!", uid);

  uid_t euid = GetEUID();
  CPPUNIT_ASSERT_MESSAGE("Don't run the tests as suid root!", euid);
}


/**
 * Check we're not running as root.
 */
void CredentialsTest::testGetGIDs() {
  gid_t gid = GetGID();
  CPPUNIT_ASSERT_MESSAGE("Don't run the tests as root!", gid);

  gid_t egid = GetEGID();
  CPPUNIT_ASSERT_MESSAGE("Don't run the tests as sgid root!", egid);
}


/**
 * Check SetUID as much as we can.
 */
void CredentialsTest::testSetUID() {
  uid_t euid = GetEUID();

  if (euid) {
    CPPUNIT_ASSERT(SetUID(euid));
    CPPUNIT_ASSERT(!SetUID(0));
    CPPUNIT_ASSERT(!SetUID(euid + 1));
  }
}


/**
 * Check SetGID as much as we can.
 */
void CredentialsTest::testSetGID() {
  gid_t egid = GetEGID();

  if (egid) {
    CPPUNIT_ASSERT(SetGID(egid));
    CPPUNIT_ASSERT(!SetGID(0));
    CPPUNIT_ASSERT(!SetGID(egid + 1));
  }
}


/**
 * Check the GetPasswd functions work.
 */
void CredentialsTest::testGetPasswd() {
  uid_t uid = GetUID();

  PasswdEntry passwd_entry;
  CPPUNIT_ASSERT(GetPasswdUID(uid, &passwd_entry));
  // at the very least we shoud have a name
  CPPUNIT_ASSERT(!passwd_entry.pw_name.empty());
  CPPUNIT_ASSERT_EQUAL(uid, passwd_entry.pw_uid);

  // now fetch by name and check it's the same
  // this could fail. if the accounts were really messed up
  PasswdEntry passwd_entry2;
  CPPUNIT_ASSERT(GetPasswdName(passwd_entry.pw_name, &passwd_entry2));
  CPPUNIT_ASSERT_EQUAL(uid, passwd_entry2.pw_uid);
}


/**
 * Check the GetGroup functions work.
 */
void CredentialsTest::testGetGroup() {
  gid_t gid = GetGID();

  GroupEntry group_entry;
  // not all systems will be configured with a group entry so this isn't a
  // failure.
  bool ok = GetGroupGID(gid, &group_entry);
  if (ok) {
    // at the very least we shoud have a name
    CPPUNIT_ASSERT(!group_entry.gr_name.empty());
    CPPUNIT_ASSERT_EQUAL(gid, group_entry.gr_gid);

    // now fetch by name and check it's the same
    // this could fail. if the accounts were really messed up
    GroupEntry group_entry2;
    CPPUNIT_ASSERT(GetGroupName(group_entry.gr_name, &group_entry2));
    CPPUNIT_ASSERT_EQUAL(gid, group_entry2.gr_gid);
  }
}
