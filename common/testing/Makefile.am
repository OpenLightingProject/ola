include $(top_srcdir)/common.mk

if BUILD_TESTS
noinst_LTLIBRARIES = libolatesting.la libtestmain.la
libolatesting_la_SOURCES = MockUDPSocket.cpp TestUtils.cpp

libtestmain_la_SOURCES = GenericTester.cpp
endif
