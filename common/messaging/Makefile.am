include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolamessaging.la
libolamessaging_la_SOURCES = Descriptor.cpp \
                             Message.cpp \
                             MessagePrinter.cpp \
                             SchemaPrinter.cpp

if BUILD_TESTS
TESTS = DescriptorTester
endif
check_PROGRAMS = $(TESTS)

DescriptorTester_SOURCES = DescriptorTest.cpp \
                           SchemaPrinterTest.cpp \
                           MessagePrinterTest.cpp
DescriptorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DescriptorTester_LDADD = $(COMMON_TESTING_LIBS) \
                         ../libolacommon.la \
                         ../testing/libtestmain.la

