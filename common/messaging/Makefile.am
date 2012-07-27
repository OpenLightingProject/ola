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
DescriptorTester_SOURCES = DescriptorTester.cpp \
                           DescriptorTest.cpp \
                           SchemaPrinterTest.cpp \
                           MessagePrinterTest.cpp
DescriptorTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DescriptorTester_LDADD = $(CPPUNIT_LIBS) \
                         ../libolacommon.la
