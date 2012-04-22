# here we define common flags for C++ targets
WARNING_CXXFLAGS = -I$(top_builddir)/include \
                   -Wall -Wformat -W \
                   $(libprotobuf_CFLAGS)

COMMON_CXXFLAGS = $(WARNING_CXXFLAGS)

# the genererated protobuf files don't compile with -Werror on win32
if USING_WIN32_FALSE
  COMMON_CXXFLAGS += -Werror
endif

# AM_CXXFLAGS is used when target_CXXFLAGS isn't defined
AM_CXXFLAGS = $(COMMON_CXXFLAGS)

COMMON_TESTING_LIBS = $(CPPUNIT_LIBS) \
                      $(top_builddir)/common/testing/libolatesting.la
