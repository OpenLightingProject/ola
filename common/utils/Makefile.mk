# LIBRARIES
################################################
common_libolacommon_la_SOURCES += \
    common/utils/ActionQueue.cpp \
    common/utils/Clock.cpp \
    common/utils/DmxBuffer.cpp \
    common/utils/StringUtils.cpp \
    common/utils/TokenBucket.cpp \
    common/utils/Watchdog.cpp

# TESTS
################################################
test_programs += common/utils/UtilsTester

common_utils_UtilsTester_SOURCES = \
    common/utils/ActionQueueTest.cpp \
    common/utils/BackoffTest.cpp \
    common/utils/CallbackTest.cpp \
    common/utils/ClockTest.cpp \
    common/utils/DmxBufferTest.cpp \
    common/utils/MultiCallbackTest.cpp \
    common/utils/StringUtilsTest.cpp \
    common/utils/TokenBucketTest.cpp \
    common/utils/UtilsTest.cpp \
    common/utils/WatchdogTest.cpp
common_utils_UtilsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_utils_UtilsTester_LDADD = $(COMMON_TESTING_LIBS)
