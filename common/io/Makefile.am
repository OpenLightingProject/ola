include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolaio.la
libolaio_la_SOURCES = Descriptor.cpp \
                      IOQueue.cpp \
                      SelectServer.cpp \
                      StdinHandler.cpp

if BUILD_TESTS
TESTS = BufferedDescriptorTester DescriptorTester IOQueueTester \
        SelectServerTester StreamTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(CPPUNIT_LIBS) \
                    ../libolacommon.la \
                    ../testing/libolatesting.la

IOQueueTester_SOURCES = IOQueueTest.cpp \
                        GenericTester.cpp
IOQueueTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
IOQueueTester_LDADD = $(COMMON_TEST_LDADD)

DescriptorTester_SOURCES = DescriptorTest.cpp \
                           GenericTester.cpp
DescriptorTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DescriptorTester_LDADD = $(COMMON_TEST_LDADD)


BufferedDescriptorTester_SOURCES = BufferedDescriptorTest.cpp \
                                   GenericTester.cpp
BufferedDescriptorTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
BufferedDescriptorTester_LDADD = $(COMMON_TEST_LDADD)

SelectServerTester_SOURCES = GenericTester.cpp \
                             SelectServerTest.cpp \
                             SelectServerThreadTest.cpp
SelectServerTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SelectServerTester_LDADD = $(COMMON_TEST_LDADD)

StreamTester_SOURCES = GenericTester.cpp \
                       StreamTest.cpp
StreamTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
StreamTester_LDADD = $(COMMON_TEST_LDADD)

