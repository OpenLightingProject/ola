include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolautils.la
libolautils_la_SOURCES = ActionQueue.cpp \
                         DmxBuffer.cpp \
                         RunLengthEncoder.cpp \
                         StringUtils.cpp \
                         TokenBucket.cpp

if BUILD_TESTS
TESTS = UtilsTester
endif
check_PROGRAMS = $(TESTS)
UtilsTester_SOURCES = ActionQueueTest.cpp BackoffTest.cpp ClockTest.cpp \
                      CallbackTest.cpp DmxBufferTest.cpp MultiCallbackTest.cpp \
                      RunLengthEncoderTest.cpp  StringUtilsTest.cpp \
                      TokenBucketTest.cpp UtilsTester.cpp
UtilsTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
UtilsTester_LDADD = $(CPPUNIT_LIBS) \
                    libolautils.la \
                    ../base/libolabase.la \
                    ../logging/liblogging.la
