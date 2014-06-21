include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
noinst_LTLIBRARIES = libolaio.la
libolaio_la_SOURCES = Descriptor.cpp \
                      ExtendedSerial.cpp \
                      IOQueue.cpp \
                      IOStack.cpp \
                      IOUtils.cpp \
                      PollerInterface.cpp \
                      PollerInterface.h \
                      SelectServer.cpp \
                      Serial.cpp \
                      StdinHandler.cpp \
                      TimeoutManager.cpp \
                      TimeoutManager.h
if USING_WIN32
libolaio_la_SOURCES += WindowsPoller.cpp \
                       WindowsPoller.h
else
libolaio_la_SOURCES += SelectPoller.cpp \
                       SelectPoller.h
endif

# TESTS
##################################################
if BUILD_TESTS
TESTS = DescriptorTester IOQueueTester \
        IOStackTester MemoryBlockTester \
        SelectServerTester StreamTester \
        TimeoutManagerTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ../libolacommon.la
if USING_WIN32
COMMON_TEST_LDADD += -lWs2_32 -lIphlpapi
endif

IOQueueTester_SOURCES = IOQueueTest.cpp
IOQueueTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
IOQueueTester_LDADD = $(COMMON_TEST_LDADD)

IOStackTester_SOURCES = IOStackTest.cpp
IOStackTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
IOStackTester_LDADD = $(COMMON_TEST_LDADD)

DescriptorTester_SOURCES = DescriptorTest.cpp
DescriptorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DescriptorTester_LDADD = $(COMMON_TEST_LDADD)

MemoryBlockTester_SOURCES = MemoryBlockTest.cpp
MemoryBlockTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
MemoryBlockTester_LDADD = $(COMMON_TEST_LDADD)

SelectServerTester_SOURCES = SelectServerTest.cpp \
                             SelectServerThreadTest.cpp
SelectServerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SelectServerTester_LDADD = $(COMMON_TEST_LDADD)

TimeoutManagerTester_SOURCES = TimeoutManagerTest.cpp
TimeoutManagerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
TimeoutManagerTester_LDADD = $(COMMON_TEST_LDADD)

StreamTester_SOURCES = InputStreamTest.cpp \
                       OutputStreamTest.cpp
StreamTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
StreamTester_LDADD = $(COMMON_TEST_LDADD)
