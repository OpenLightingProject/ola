include $(top_srcdir)/common.mk

SUBDIRS = testdata
EXTRA_DIST = DescriptorConsistencyChecker.h DiscoveryAgentTestHelper.h \
             GroupSizeCalculator.h Pids.proto \
             PidStoreLoader.h VariableFieldSizeCalculator.h

BUILT_SOURCES = Pids.pb.cc Pids.pb.h

noinst_LTLIBRARIES = libolardm.la
libolardm_la_SOURCES = RDMAPI.cpp RDMCommand.cpp UID.cpp 

nodist_libolardm_la_SOURCES = Pids.pb.cc
libolardm_la_LIBADD = $(libprotobuf_LIBS)

if BUILD_TESTS
TESTS = RDMTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ../libolacommon.la

RDMTester_SOURCES = RDMAPITest.cpp
RDMTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RDMTester_LDADD = $(COMMON_TEST_LDADD)

Pids.pb.cc Pids.pb.h: Pids.proto
	$(PROTOC) --cpp_out ./ Pids.proto

TestService.pb.cc TestService.pb.h: TestService.proto
	$(PROTOC) --cpp_out ./ TestService.proto

clean-local:
	rm -f *.pb.{h,cc}
