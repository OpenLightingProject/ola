include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libthread.la
libthread_la_SOURCES = ConsumerThread.cpp Mutex.cpp Thread.cpp ThreadPool.cpp

TESTS = ThreadTester
check_PROGRAMS = $(TESTS)
ThreadTester_SOURCES = ThreadPoolTest.cpp ThreadTest.cpp ThreadTester.cpp
ThreadTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ThreadTester_LDADD = $(CPPUNIT_LIBS) \
                     ../logging/liblogging.la \
                     ../utils/libolautils.la \
                     ./libthread.la
