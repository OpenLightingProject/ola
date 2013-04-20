include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolaio.la
libolaio_la_SOURCES = Descriptor.cpp \
                      IOQueue.cpp \
                      IOStack.cpp \
                      SelectServer.cpp \
                      StdinHandler.cpp

if BUILD_TESTS
TESTS = DescriptorTester IOQueueTester \
        IOStackTester SelectServerTester StreamTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ../libolacommon.la \
                    ../testing/libtestmain.la

IOQueueTester_SOURCES = IOQueueTest.cpp
IOQueueTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
IOQueueTester_LDADD = $(COMMON_TEST_LDADD)

IOStackTester_SOURCES = IOStackTest.cpp
IOStackTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
IOStackTester_LDADD = $(COMMON_TEST_LDADD)

DescriptorTester_SOURCES = DescriptorTest.cpp
DescriptorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DescriptorTester_LDADD = $(COMMON_TEST_LDADD)

SelectServerTester_SOURCES = SelectServerTest.cpp \
                             SelectServerThreadTest.cpp
SelectServerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SelectServerTester_LDADD = $(COMMON_TEST_LDADD)

StreamTester_SOURCES = InputStreamTest.cpp \
                       OutputStreamTest.cpp
StreamTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
StreamTester_LDADD = $(COMMON_TEST_LDADD)
