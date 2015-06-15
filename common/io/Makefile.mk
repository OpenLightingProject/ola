# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/io/Descriptor.cpp \
    common/io/ExtendedSerial.cpp \
    common/io/EPoller.h \
    common/io/IOQueue.cpp \
    common/io/IOStack.cpp \
    common/io/IOUtils.cpp \
    common/io/NonBlockingSender.cpp \
    common/io/PollerInterface.cpp \
    common/io/PollerInterface.h \
    common/io/SelectServer.cpp \
    common/io/Serial.cpp \
    common/io/StdinHandler.cpp \
    common/io/TimeoutManager.cpp \
    common/io/TimeoutManager.h

if USING_WIN32
common_libolacommon_la_SOURCES += \
    common/io/WindowsPoller.cpp \
    common/io/WindowsPoller.h
else
common_libolacommon_la_SOURCES += \
    common/io/SelectPoller.cpp \
    common/io/SelectPoller.h
endif

if HAVE_EPOLL
common_libolacommon_la_SOURCES += \
    common/io/EPoller.h \
    common/io/EPoller.cpp
endif

if HAVE_KQUEUE
common_libolacommon_la_SOURCES += \
    common/io/KQueuePoller.h \
    common/io/KQueuePoller.cpp
endif

# TESTS
##################################################
test_programs += \
    common/io/DescriptorTester \
    common/io/IOQueueTester \
    common/io/IOStackTester \
    common/io/MemoryBlockTester \
    common/io/SelectServerTester \
    common/io/StreamTester \
    common/io/TimeoutManagerTester

common_io_IOQueueTester_SOURCES = common/io/IOQueueTest.cpp
common_io_IOQueueTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_io_IOQueueTester_LDADD = $(COMMON_TESTING_LIBS)

common_io_IOStackTester_SOURCES = common/io/IOStackTest.cpp
common_io_IOStackTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_io_IOStackTester_LDADD = $(COMMON_TESTING_LIBS)

common_io_DescriptorTester_SOURCES = common/io/DescriptorTest.cpp
common_io_DescriptorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_io_DescriptorTester_LDADD = $(COMMON_TESTING_LIBS)

common_io_MemoryBlockTester_SOURCES = common/io/MemoryBlockTest.cpp
common_io_MemoryBlockTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_io_MemoryBlockTester_LDADD = $(COMMON_TESTING_LIBS)

common_io_SelectServerTester_SOURCES = common/io/SelectServerTest.cpp \
                                       common/io/SelectServerThreadTest.cpp
common_io_SelectServerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_io_SelectServerTester_LDADD = $(COMMON_TESTING_LIBS)

common_io_TimeoutManagerTester_SOURCES = common/io/TimeoutManagerTest.cpp
common_io_TimeoutManagerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_io_TimeoutManagerTester_LDADD = $(COMMON_TESTING_LIBS)

common_io_StreamTester_SOURCES = common/io/InputStreamTest.cpp \
                                 common/io/OutputStreamTest.cpp
common_io_StreamTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_io_StreamTester_LDADD = $(COMMON_TESTING_LIBS)
