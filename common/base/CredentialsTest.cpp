/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * CredentialsTest.cpp
 * Test the credentials functions.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/base/Credentials.h"
#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"

using ola::SupportsUIDs;
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
  if (SupportsUIDs()) {
    uid_t uid;
    OLA_ASSERT_TRUE(GetUID(&uid));
    OLA_ASSERT_TRUE_MSG(uid, "Don't run the tests as root!");

    uid_t euid;
    OLA_ASSERT_TRUE(GetEUID(&euid));
    OLA_ASSERT_TRUE_MSG(euid, "Don't run the tests as suid root!");
  } else {
    // Make sure the Get*UID functions actually return false.
    uid_t uid;
    OLA_ASSERT_FALSE(GetUID(&uid));

    uid_t euid;
    OLA_ASSERT_FALSE(GetEUID(&euid));
  }
}


/**
 * Check we're not running as root.
 */
void CredentialsTest::testGetGIDs() {
  if (SupportsUIDs()) {
    gid_t gid;
    OLA_ASSERT_TRUE(GetGID(&gid));
    OLA_ASSERT_TRUE_MSG(gid, "Don't run the tests as root!");

    gid_t egid;
    OLA_ASSERT_TRUE(GetEGID(&egid));
    OLA_ASSERT_TRUE_MSG(egid, "Don't run the tests as sgid root!");
  } else {
    // Make sure the Get*GID functions actually return false.
    gid_t gid;
    OLA_ASSERT_FALSE(GetGID(&gid));

    gid_t egid;
    OLA_ASSERT_FALSE(GetEGID(&egid));
  }
}


/**
 * Check SetUID as much as we can.
 */
void CredentialsTest::testSetUID() {
  if (SupportsUIDs()) {
    uid_t euid;
    OLA_ASSERT_TRUE(GetEUID(&euid));

    if (euid) {
      OLA_ASSERT_TRUE(SetUID(euid));
      OLA_ASSERT_FALSE(SetUID(0));
      OLA_ASSERT_FALSE(SetUID(euid + 1));
    }
  }
}


/**
 * Check SetGID as much as we can.
 */
void CredentialsTest::testSetGID() {
  if (SupportsUIDs()) {
    gid_t egid;
    OLA_ASSERT_TRUE(GetEGID(&egid));

    if (egid) {
      OLA_ASSERT_TRUE(SetGID(egid));
      OLA_ASSERT_FALSE(SetGID(0));
      OLA_ASSERT_FALSE(SetGID(egid + 1));
    }
  }
}


/**
 * Check the GetPasswd functions work.
 */
void CredentialsTest::testGetPasswd() {
  if (SupportsUIDs()) {
    uid_t uid;
    OLA_ASSERT_TRUE(GetUID(&uid));

    PasswdEntry passwd_entry;
    OLA_ASSERT_TRUE(GetPasswdUID(uid, &passwd_entry));
    // at the very least we shoud have a name
    OLA_ASSERT_FALSE(passwd_entry.pw_name.empty());
    OLA_ASSERT_EQ(uid, passwd_entry.pw_uid);

    // now fetch by name and check it's the same
    // this could fail. if the accounts were really messed up
    PasswdEntry passwd_entry2;
    OLA_ASSERT_TRUE(GetPasswdName(passwd_entry.pw_name, &passwd_entry2));
    OLA_ASSERT_EQ(uid, passwd_entry2.pw_uid);
  } else {
    // Make sure GetPasswd* actually return false.
    PasswdEntry unused;
    OLA_ASSERT_FALSE(GetPasswdUID(0, &unused));

    // Check with an account name that likely exists.
    OLA_ASSERT_FALSE(GetPasswdName("SYSTEM", &unused));
  }
}


/**
 * Check the GetGroup functions work.
 */
void CredentialsTest::testGetGroup() {
  if (SupportsUIDs()) {
    gid_t gid;
    OLA_ASSERT_TRUE(GetGID(&gid));

    GroupEntry group_entry;
    // not all systems will be configured with a group entry so this isn't a
    // failure.
    bool ok = GetGroupGID(gid, &group_entry);
    if (ok) {
      // at the very least we shoud have a name
      OLA_ASSERT_FALSE(group_entry.gr_name.empty());
      OLA_ASSERT_EQ(gid, group_entry.gr_gid);

      // now fetch by name and check it's the same
      // this could fail. if the accounts were really messed up
      GroupEntry group_entry2;
      OLA_ASSERT_TRUE(GetGroupName(group_entry.gr_name, &group_entry2));
      OLA_ASSERT_EQ(gid, group_entry2.gr_gid);
    }
  } else {
  // Make sure GetPasswd* actually return false.
    GroupEntry unused;
    OLA_ASSERT_FALSE(GetGroupGID(0, &unused));

    // Check with a group name that might exist.
    OLA_ASSERT_FALSE(GetGroupName("SYSTEM", &unused));
  }
}
