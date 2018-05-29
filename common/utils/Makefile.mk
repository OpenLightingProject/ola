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
# DmxBufferTest.cpp assigns a value to itself which clang complains about, so explicitly allow that here
common_utils_UtilsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS) -Wno-error=self-assign-overloaded
common_utils_UtilsTester_LDADD = $(COMMON_TESTING_LIBS)
