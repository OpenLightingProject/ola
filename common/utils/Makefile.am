include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolautils.la
libolautils_la_SOURCES = ActionQueue.cpp \
                         DmxBuffer.cpp \
                         StringUtils.cpp \
                         TokenBucket.cpp

if BUILD_TESTS
TESTS = UtilsTester
endif
check_PROGRAMS = $(TESTS)
UtilsTester_SOURCES = ActionQueueTest.cpp BackoffTest.cpp ClockTest.cpp \
                      CallbackTest.cpp DmxBufferTest.cpp \
                      MultiCallbackTest.cpp StringUtilsTest.cpp \
                      TokenBucketTest.cpp
UtilsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
UtilsTester_LDADD = $(COMMON_TESTING_LIBS) \
                    libolautils.la \
                    ../base/libolabase.la
