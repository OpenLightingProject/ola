include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libthread.la
libthread_la_SOURCES = ConsumerThread.cpp ThreadPool.cpp

TESTS = ThreadTester
check_PROGRAMS = $(TESTS)
ThreadTester_SOURCES = ThreadTester.cpp ThreadPoolTest.cpp
ThreadTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ThreadTester_LDADD = $(CPPUNIT_LIBS) \
                     ../logging/liblogging.la \
                     ../utils/libolautils.la \
                     ./libthread.la
