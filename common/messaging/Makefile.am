include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolamessaging.la
libolamessaging_la_SOURCES = Descriptor.cpp \
                             Message.cpp \
                             MessagePrinter.cpp \
                             SchemaPrinter.cpp

check_PROGRAMS = DescriptorTester
DescriptorTester_SOURCES = DescriptorTester.cpp \
                           DescriptorTest.cpp \
                           SchemaPrinterTest.cpp \
                           MessagePrinterTest.cpp
DescriptorTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DescriptorTester_LDADD = $(CPPUNIT_LIBS) \
                         libolamessaging.la \
                         ../logging/liblogging.la
