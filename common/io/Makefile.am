include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolaio.la
libolaio_la_SOURCES = Descriptor.cpp \
                      IOQueue.cpp \
                      SelectServer.cpp

TESTS = BufferedDescriptorTester DescriptorTester IOQueueTester \
        SelectServerTester
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(CPPUNIT_LIBS) \
                    ../export_map/libolaexportmap.la \
                    ../logging/liblogging.la \
                    ../network/libolanetwork.la \
                    ../testing/libolatesting.la \
                    ../thread/libthread.la \
                    ../utils/libolautils.la \
                    ./libolaio.la

IOQueueTester_SOURCES = IOQueueTest.cpp \
                        IOQueueTester.cpp
IOQueueTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
IOQueueTester_LDADD = $(COMMON_TEST_LDADD)

DescriptorTester_SOURCES = DescriptorTest.cpp \
                           DescriptorTester.cpp
DescriptorTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DescriptorTester_LDADD = $(COMMON_TEST_LDADD)


BufferedDescriptorTester_SOURCES = BufferedDescriptorTest.cpp \
                                   BufferedDescriptorTester.cpp
BufferedDescriptorTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
BufferedDescriptorTester_LDADD = $(COMMON_TEST_LDADD)

SelectServerTester_SOURCES = SelectServerTest.cpp \
                             SelectServerTester.cpp \
                             SelectServerThreadTest.cpp
SelectServerTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SelectServerTester_LDADD = $(COMMON_TEST_LDADD)

