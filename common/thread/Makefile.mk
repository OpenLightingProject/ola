# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/thread/ConsumerThread.cpp \
    common/thread/ExecutorThread.cpp \
    common/thread/Mutex.cpp \
    common/thread/PeriodicThread.cpp \
    common/thread/SignalThread.cpp \
    common/thread/Thread.cpp \
    common/thread/ThreadPool.cpp \
    common/thread/Utils.cpp

# TESTS
##################################################
test_programs += common/thread/ExecutorThreadTester \
                 common/thread/ThreadTester \
                 common/thread/FutureTester

common_thread_ThreadTester_SOURCES = \
    common/thread/ThreadPoolTest.cpp \
    common/thread/ThreadTest.cpp
common_thread_ThreadTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_thread_ThreadTester_LDADD = $(COMMON_TESTING_LIBS)

common_thread_FutureTester_SOURCES = common/thread/FutureTest.cpp
common_thread_FutureTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_thread_FutureTester_LDADD = $(COMMON_TESTING_LIBS)

common_thread_ExecutorThreadTester_SOURCES = \
    common/thread/ExecutorThreadTest.cpp
common_thread_ExecutorThreadTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_thread_ExecutorThreadTester_LDADD = $(COMMON_TESTING_LIBS)
