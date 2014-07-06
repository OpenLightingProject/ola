# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/messaging/Descriptor.cpp \
    common/messaging/Message.cpp \
    common/messaging/MessagePrinter.cpp \
    common/messaging/SchemaPrinter.cpp

# TESTS
##################################################
test_programs += common/messaging/DescriptorTester

common_messaging_DescriptorTester_SOURCES = \
    common/messaging/DescriptorTest.cpp \
    common/messaging/SchemaPrinterTest.cpp \
    common/messaging/MessagePrinterTest.cpp
common_messaging_DescriptorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_messaging_DescriptorTester_LDADD = $(COMMON_TESTING_LIBS)

