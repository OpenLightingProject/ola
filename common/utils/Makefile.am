include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolautils.la
libolautils_la_SOURCES = ActionQueue.cpp \
                         DmxBuffer.cpp \
                         OlaThread.cpp \
                         RunLengthEncoder.cpp \
                         StringUtils.cpp \
                         TokenBucket.cpp

TESTS = UtilsTester
check_PROGRAMS = $(TESTS)
UtilsTester_SOURCES = ActionQueueTest.cpp ClockTest.cpp CallbackTest.cpp \
                      DmxBufferTest.cpp OlaThreadTest.cpp \
                      RunLengthEncoderTest.cpp  StringUtilsTest.cpp \
                      UtilsTester.cpp
UtilsTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
UtilsTester_LDADD = $(CPPUNIT_LIBS) \
                    libolautils.la \
                    ../logging/liblogging.la
