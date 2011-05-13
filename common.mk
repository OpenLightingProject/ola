AM_CPPFLAGS = -I$(top_builddir)/include -Wall -Wformat -W

# the genererate protobuf files don't compile with -Werror on win32
if USING_WIN32_FALSE
  AM_CPPFLAGS += -Werror
endif
