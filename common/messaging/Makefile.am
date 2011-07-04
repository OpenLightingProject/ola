include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolamessaging.la
libolamessaging_la_SOURCES = Descriptor.cpp SchemaPrinter.cpp

TESTS = DescriptorTest
check_PROGRAMS = $(TESTS)
DescriptorTest_SOURCES = DescriptorTester.cpp DescriptorTest.cpp \
                         SchemaPrinterTest.cpp
DescriptorTest_CPPFLAGS = $(CPPUNIT_CFLAGS) $(AM_CPPFLAGS)
DescriptorTest_LDFLAGS = $(CPPUNIT_LIBS)
DescriptorTest_LDADD = libolamessaging.la ../logging/liblogging.la
