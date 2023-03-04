#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
m4_include([config/ola_version.m4])
AC_INIT([OLA], [ola_version], [open-lighting@googlegroups.com])
AC_CONFIG_SRCDIR([libola.pc.in])
AC_CONFIG_AUX_DIR([config])
AC_SUBST([ac_aux_dir])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([config])
# Make us require Automake 1.11.1
AM_INIT_AUTOMAKE([1.11.1 subdir-objects])

# OLA version variables
AC_SUBST([ola_major_version])
OLA_MAJOR_VERSION=ola_major_version
AC_SUBST(OLA_MAJOR_VERSION)
AC_SUBST([ola_minor_version])
OLA_MINOR_VERSION=ola_minor_version
AC_SUBST(OLA_MINOR_VERSION)
AC_SUBST([ola_revision_version])
OLA_REVISION_VERSION=ola_revision_version
AC_SUBST(OLA_REVISION_VERSION)

# Checks for programs.
AC_LANG([C++])
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

# check if the compiler supports -std=gnu++98
AC_MSG_CHECKING(for -std=gnu++98 support)
old_cxxflags=$CXXFLAGS
CXXFLAGS="${CXXFLAGS} -std=gnu++98 -Wall -Werror"
AC_CACHE_VAL(ac_cv_gnu_plus_plus_98,
  AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([], [])],
     [ac_cv_gnu_plus_plus_98=yes],
     [ac_cv_gnu_plus_plus_98=no])
)
CXXFLAGS=$old_cxxflags
AC_MSG_RESULT($ac_cv_gnu_plus_plus_98)

AM_CONDITIONAL([SUPPORTS_GNU_PLUS_PLUS_98], [test "x$ac_cv_gnu_plus_plus_98" = xyes])

# check if the compiler supports -std=gnu++11
AC_MSG_CHECKING(for -std=gnu++11 support)
old_cxxflags=$CXXFLAGS
CXXFLAGS="${CXXFLAGS} -std=gnu++11 -Wall -Werror"
AC_CACHE_VAL(ac_cv_gnu_plus_plus_11,
  AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([], [])],
     [ac_cv_gnu_plus_plus_11=yes],
     [ac_cv_gnu_plus_plus_11=no])
)
CXXFLAGS=$old_cxxflags
AC_MSG_RESULT($ac_cv_gnu_plus_plus_11)

AM_CONDITIONAL([SUPPORTS_GNU_PLUS_PLUS_11], [test "x$ac_cv_gnu_plus_plus_11" = xyes])

# This checks whether random needs gnu++11 or not, it doesn't set HAVE_RANDOM
# (as it's using AC_CHECK_HEADER not HEADERS), we do that later as normal
# We don't pass the compiler flag to the preprocessor (CPPFLAGS) because that
# variable is used by the C preprocessor too, and some of the pthread checks
# test for C code, which complains it doesn't understand gnu++. We may
# therefore generate some warnings in configure, but it should all work!
ac_cv_header_random_98="no"
ac_cv_header_random_11="no"

AC_MSG_NOTICE([checking for random gnu++98 compatibility])
old_cxxflags=$CXXFLAGS
CXXFLAGS="${CXXFLAGS} -std=gnu++98 -Wall -Werror"
# Manually purge the cache
AS_UNSET([ac_cv_header_random])
AC_CHECK_HEADER(
   [random],
   [ac_cv_header_random_98="yes"],
   [ac_cv_header_random_98="no"])
# Manually purge the cache
AS_UNSET([ac_cv_header_random])
CXXFLAGS=$old_cxxflags

AS_IF([test "x$ac_cv_gnu_plus_plus_11" = xyes],
      [AC_MSG_NOTICE([checking for random gnu++11 compatibility])
       old_cxxflags=$CXXFLAGS
       CXXFLAGS="${CXXFLAGS} -std=gnu++11 -Wall -Werror"
       # Manually purge the cache
       AS_UNSET([ac_cv_header_random])
       AC_CHECK_HEADER(
          [random],
          [ac_cv_header_random_11=yes],
          [ac_cv_header_random_11=no])
       # Manually purge the cache
       AS_UNSET([ac_cv_header_random])
       CXXFLAGS=$old_cxxflags])

# force us into gnu++98 mode if necessary
# If gnu++11 and gnu++98 then
#   If random works with gnu++98
#     If protobuf < 3.6
#       If no unit tests, force to gnu++98
#       Else we have unit tests
#         If cppunit < 1.14.0, force to gnu++98
#         Else turn off deprecation messages for std::auto_ptr and run gnu++11
#     Else assume we have protobuf >= 3.6 (later checks will confirm that for certain), turn off deprecation messages for std::auto_ptr and run gnu++11
#   Else turn off deprecation messages for std::auto_ptr and run gnu++11
require_gnu_plus_plus_11="no"
AS_IF([test "x$ac_cv_gnu_plus_plus_11" = xyes],
      [AS_IF([test "x$ac_cv_gnu_plus_plus_98" = xyes],
             [AS_IF([test "x$ac_cv_header_random_98" = xyes],
                    [PKG_CHECK_MODULES([PROTOBUF1], [protobuf < 3.6],
                                       [AS_IF([test "x$enable_unittests" = xno],
                                              [CXXFLAGS="$CXXFLAGS -std=gnu++98"],
                                              [PKG_CHECK_MODULES([CPPUNIT1], [cppunit < 1.14.0],
                                                                 [CXXFLAGS="$CXXFLAGS -std=gnu++98"],
                                                                 [PKG_CHECK_MODULES([CPPUNIT2], [cppunit >= 1.14.0],
                                                                                    [require_gnu_plus_plus_11="yes"],
                                                                                    [AC_MSG_WARN([OLA requires std::auto_ptr support.])])
                                                                 ])
                                              ])
                                       ],
                                       [require_gnu_plus_plus_11="yes"])
                    ],
                    [require_gnu_plus_plus_11="yes"])
             ])
      ])
AS_IF([test "x$require_gnu_plus_plus_11" = xyes],
      [CXXFLAGS="$CXXFLAGS -std=gnu++11"])
AM_CONDITIONAL([GNU_PLUS_PLUS_11_DEPRECATIONS], [test "x$require_gnu_plus_plus_11" = xyes])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_RESOLV
AC_HEADER_STDC
# Required headers (we cannot work without these)
AC_CHECK_HEADERS([errno.h],[],[AC_MSG_ERROR([Missing a required header])])
# Other headers (we can work without these, but may need to modify things slightly)
AC_CHECK_HEADERS([arpa/inet.h bits/sockaddr.h fcntl.h float.h limits.h \
                  malloc.h netinet/in.h stdint.h stdlib.h string.h strings.h \
                  sys/file.h sys/ioctl.h sys/socket.h sys/time.h sys/timeb.h \
                  syslog.h termios.h unistd.h])
AC_CHECK_HEADERS([asm/termbits.h asm/termios.h assert.h dlfcn.h endian.h \
                  execinfo.h linux/if_packet.h math.h net/ethernet.h \
                  stropts.h sys/ioctl.h sys/param.h sys/types.h sys/uio.h \
                  sysexits.h])
AC_CHECK_HEADERS([winsock2.h winerror.h])
AC_CHECK_HEADERS([random])

# This needs stuff in sys/param.h on OpenBSD
AC_CHECK_HEADERS([sys/sysctl.h], [], [],
                 [[#ifdef HAVE_SYS_PARAM_H
                     #include <sys/param.h>
                   #endif
                  ]])

# These all need stuff in sys/types.h and sys/socket.h on FreeBSD/OpenBSD
# OpenBSD is fussy about the ordering
AC_CHECK_HEADERS([net/if.h net/if_arp.h net/route.h], [], [],
                 [[#ifdef HAVE_SYS_TYPES_H
                     #include <sys/types.h>
                   #endif
                   #ifdef HAVE_SYS_SOCKET_H
                     #include <sys/socket.h>
                   #endif
                  ]])

# This needs stuff in net/if.h on NetBSD
AC_CHECK_HEADERS([net/if_ether.h], [], [],
                 [[#ifdef HAVE_NET_IF_H
                     #include <net/if.h>
                   #endif
                  ]])

# This needs stuff in lots of headers on OpenBSD
AC_CHECK_HEADERS([netinet/if_ether.h], [], [],
                 [[#ifdef HAVE_SYS_TYPES_H
                     #include <sys/types.h>
                   #endif
                   #ifdef HAVE_SYS_SOCKET_H
                     #include <sys/socket.h>
                   #endif
                   #ifdef HAVE_NET_IF_H
                     #include <net/if.h>
                   #endif
                   #ifdef HAVE_NET_IF_ARP_H
                     #include <net/if_arp.h>
                   #endif
                   #ifdef HAVE_ARPA_INET_H
                     #include <arpa/inet.h>
                   #endif
                  ]])

# These both need sa_family_t from bits/sockaddr.h
AC_CHECK_HEADERS([linux/netlink.h linux/rtnetlink.h], [], [],
                 [[#ifdef HAVE_BITS_SOCKADDR_H
                     #include <bits/sockaddr.h>
                   #endif
                  ]])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CXX_STL_HASH  # hash_map

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero gettimeofday memmove memset mkdir strdup strrchr \
                if_nametoindex inet_ntoa inet_ntop inet_aton inet_pton select \
                socket strerror getifaddrs getloadavg getpwnam_r getpwuid_r \
                getgrnam_r getgrgid_r secure_getenv clock_gettime])

LT_INIT([win32-dll])

# Decide if we're building on Windows early on.
AM_CONDITIONAL([USING_WIN32], [test "x$host_os" = xmingw32])
have_not_win32="yes"
if test "x$host_os" = xmingw32; then
  have_not_win32="no"
fi

# Epoll
AX_HAVE_EPOLL(
  [AC_DEFINE(HAVE_EPOLL, 1, [Defined if epoll exists])], [])
AM_CONDITIONAL(HAVE_EPOLL, test "${ax_cv_have_epoll}" = "yes")

# kqueue
AC_CHECK_FUNCS([kqueue])
AM_CONDITIONAL(HAVE_KQUEUE, test "${ac_cv_func_kqueue}" = "yes")

# check if the compiler supports -rdynamic
AC_MSG_CHECKING(for -rdynamic support)
old_cppflags=$CPPFLAGS
CPPFLAGS="${CPPFLAGS} -rdynamic -Wall -Werror"
AC_CACHE_VAL(ac_cv_rdynamic,
  AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([], [])],
     [ac_cv_rdynamic=yes],
     [ac_cv_rdynamic=no])
)
CPPFLAGS=$old_cppflags
AC_MSG_RESULT($ac_cv_rdynamic)

AM_CONDITIONAL([SUPPORTS_RDYNAMIC], [test "x$ac_cv_rdynamic" = xyes])

# check for ipv6 support - taken from unp
AC_MSG_CHECKING(for IPv6 support)
AC_CACHE_VAL(ac_cv_ipv6,
        AC_RUN_IFELSE(
         [AC_LANG_PROGRAM([[#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>]],[[
        /* Make sure the definitions for AF_INET6 and struct sockaddr_in6
         * are defined, and that we can actually create an IPv6 TCP socket.
         */
        main()
        {
                int fd;
                struct sockaddr_in6 foo;
                fd = socket(AF_INET6, SOCK_STREAM, 0);
                exit(fd >= 0 ? 0 : 1);
        }
        ]])],
        [ac_cv_ipv6=yes],
        [ac_cv_ipv6=no],
        [ac_cv_ipv6=no])
)
AC_MSG_RESULT($ac_cv_ipv6)
AS_IF([test "x$ac_cv_ipv6" = xyes],
      [AC_DEFINE([IPV6], [1], [Define to 1 if the system supports IPv6])])

# Check if sockaddr{} has sa_len member.
AC_CHECK_MEMBER(
  [struct sockaddr.sa_len],
  AC_DEFINE([HAVE_SOCKADDR_SA_LEN], [1],
            [Define if socket address structures have length fields]),
  ,
  [#include <sys/types.h>
   #include <sys/socket.h>])

AC_CHECK_MEMBER(
  struct sockaddr_dl.sdl_family,
  AC_DEFINE([HAVE_SOCKADDR_DL_STRUCT], [1], [define if we have sockaddr_dl]),
  ,
  [#include <sys/types.h>
   #include <net/if_dl.h>])

# check for time_t and suseconds_t
AC_CHECK_TYPE(time_t,
              AC_DEFINE([HAVE_TIME_T], [1], [define if we have time_t]),
              ,
              [#include <sys/time.h>])
AC_CHECK_TYPE(suseconds_t,
              AC_DEFINE([HAVE_SUSECONDS_T], [1],
                        [define if we have suseconds_t]),
              ,
              [#include <sys/time.h>])

# check for SO_NOSIGPIPE or MSG_NOSIGNAL
AC_CHECK_DECLS(MSG_NOSIGNAL, , have_msg_no_signal=no,
               [#include <sys/types.h>
                #include <sys/socket.h>])

AC_CHECK_DECLS(SO_NOSIGPIPE, , have_so_no_pipe=no,
               [#include <sys/types.h>
                #include <sys/socket.h>])

AC_CHECK_DECLS(PF_ROUTE, , ,
               [#include <sys/types.h>
                #include <sys/socket.h>])

AC_CHECK_DECLS(NET_RT_DUMP, , ,
               [#include <sys/types.h>
                #include <sys/socket.h>])

AC_CHECK_DECLS(RLIMIT_RTPRIO, , ,
               [#include <sys/resource.h>])

AC_CHECK_DECLS(RLIMIT_RTTIME, , ,
               [#include <sys/resource.h>])

AC_CHECK_DECLS(SO_REUSEADDR, , ,
               [#include <sys/types.h>
                #include <sys/socket.h>])

AC_CHECK_DECLS(SO_REUSEPORT, , ,
               [#include <sys/types.h>
                #include <sys/socket.h>])

if test -z "${USING_WIN32_FALSE}" && test "${have_msg_no_signal}" = "no" && \
   test "${have_so_no_pipe}" = "no"; then
 AC_MSG_ERROR([Your system needs either MSG_NOSIGNAL or SO_NOSIGPIPE])
fi

AC_CHECK_TYPE(termios2,
              AC_DEFINE([HAVE_TERMIOS2], [1], [define if we have termios2]),
              ,
              [#include <asm/termios.h>])

# Headers.
#####################################################
AC_CHECK_HEADER([linux/spi/spidev.h], [have_spi="yes"], [have_spi="no"])

# Programs.
#####################################################

# bison
BISON=""
AC_CHECK_PROG(BISON, [bison -V], bison)
AS_IF([test "x$BISON" != xbison],
      [AC_MSG_ERROR([bison not found, please install it])])
AC_SUBST(BISON)

# lex
AC_PROG_LEX
AS_IF([test "x$LEX" = "x:"],
      [AC_MSG_ERROR([lex not found, please install flex or lex])])

# pkg-config
PKG_PROG_PKG_CONFIG
AS_IF([test -z "$PKG_CONFIG"],
      [AC_MSG_ERROR([Missing pkg-config, please install it])])

# Libraries.
#####################################################

# dlopen
AC_SEARCH_LIBS([dlopen], [dl], [have_dlopen="yes"])
AM_CONDITIONAL([HAVE_DLOPEN], [test "x$have_dlopen" = xyes])

# dmx4linux
have_dmx4linux="no"
AC_CHECK_LIB(dmx4linux, DMXdev, [have_dmx4linux="yes"])
AC_CHECK_HEADER([dmx/dmxioctl.h], [], [have_dmx4linux="no"])

AS_IF([test "x$have_dmx4linux" = xyes],
      [AC_DEFINE([HAVE_DMX4LINUX], [1], [define if dmx4linux is installed])])

# librt - may be separate or part of libc
AC_SEARCH_LIBS([clock_gettime], [rt])

# libexecinfo
# FreeBSD required -lexecinfo to call backtrace - checking for presence of
# header execinfo.h isn't enough
AC_CHECK_LIB([execinfo], [backtrace], [use_lexecinfo="yes"])
# TODO(Peter): This should really be AC_COMPILE_IFELSE and AC_LANG_PROGRAM
case "${host_os}" in
  *freebsd* | *netbsd* | *dragonfly*)
  LIBS="$LIBS -lexecinfo"
  ;;
esac

# libftd2xx (for now this is intentionally disabled, TODO)
have_libftd2xx="no"
AM_CONDITIONAL([HAVE_LIBFTD2XX], [test "x$have_libftd2xx" = xyes])

# ncurses
# Location is ncurses/curses.h on DragonFly
AC_CHECK_HEADERS([curses.h ncurses/curses.h])
AC_CHECK_LIB([ncurses], [initscr], [have_ncurses="yes"])
AM_CONDITIONAL([HAVE_NCURSES], [test "x$have_ncurses" = xyes])
# Defines libncurses_LIBS that we can use to link against ncurses
# Needed since >=ncurses-6 where -ltinfo is needed in addition to -lncurses
PKG_CHECK_MODULES(libncurses,
                  [ncurses >= 5],
                  [have_ncurses_pkgconfig="yes"],
                  [have_ncurses_pkgconfig="no"])
AM_CONDITIONAL([HAVE_NCURSES_PKGCONFIG],
               [test "x$have_ncurses_pkgconfig" = xyes])

# pthread
ACX_PTHREAD([
             LIBS="$PTHREAD_LIBS $LIBS"
             CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
             CXXFLAGS="$CXXFLAGS $PTHREAD_CXXFLAGS"
             CC="$PTHREAD_CC"
             CXX="$PTHREAD_CXX"],
            [AC_MSG_ERROR([Missing pthread, please install it])])

# pthread_setname_np can take either 1 or 2 arguments.
PTHREAD_SET_NAME()

# resolv
AS_IF([test -z "${USING_WIN32_FALSE}"],
  [ACX_RESOLV()],
  [AC_CHECK_LIB([Iphlpapi], [main], [have_iphlpapi="yes"])]) # Use equivalent library on windows

# uuid
# We've had to revert to os matching here because ossp uuid conflicts with the
# built in uuid on OS X.
# On other platforms, we can use either the OSSP uuid or the one
# with e2fsprogs. On top of that the header file is different on different
# platforms :(.

# default values for everything
uuid_CFLAGS=
uuid_LIBS=
use_ossp_uuid="no"

case "${host_os}" in
  *darwin*)
    # Running on mac, just use the built in UUID
    # If we try to use ossp/uuid then we can get conflicts with <unistd.h>, if
    # <unistd.h> is #include'd before ossp/uuid.h. The errors take the form:
    #   /opt/local/include/ossp/uuid.h:94: error: conflicting declaration
    #    'typedef struct uuid_st uuid_t'
    #   /usr/include/unistd.h:133: error: 'uuid_t' has a previous declaration as
    #     'typedef unsigned char uuid_t [16]'
    AC_CHECK_HEADERS(
      [uuid/uuid.h],
      [],
      [AC_MSG_ERROR([Missing the uuid library])])
    ;;
  *)
    # non-mac, first look for uuid using pkg-config
    pkg_config_found_uuid="no"
    PKG_CHECK_MODULES(
      base_uuid,
      [uuid],
      [pkg_config_found_uuid="yes"],
      [true])

    if test ${pkg_config_found_uuid} = "yes"; then
      # uuid was found, now we need to determine which uuid library it is.
      # First modify the CPPFLAGS to use the correct include location.
      old_cppflags=$CPPFLAGS
      old_libs=$LIBS
      CPPFLAGS="${CPPFLAGS} ${base_uuid_CFLAGS}"
      LIBS="${LIBS} ${base_uuid_LIBS}"
      # see if we need to include uuid/uuid.h or just uuid.h
      AC_CHECK_HEADERS(
        [uuid/uuid.h],
        [],
        [true])
      # check if this is actually ossp uuid (this is true in win32)
      AC_CHECK_LIB([uuid],
                   [uuid_generate],
                   [],
                   [use_ossp_uuid="yes"])
      # restore CPPFLAGS
      CPPFLAGS=$old_cppflags
      LIBS=$old_libs
      uuid_CFLAGS="${base_uuid_CFLAGS}"
      uuid_LIBS="${base_uuid_LIBS}"
    else
      # the uuid pkg wasn't found, let's try ossp-uuid instead
      PKG_CHECK_MODULES(
        ossp_uuid,
        [ossp-uuid],
        [use_ossp_uuid="yes"],
        [AC_MSG_ERROR([Missing the uuid library])])
      # see if we need to include ossp/uuid.h, otherwise fall back to uuid.h
      AC_CHECK_HEADERS(
        [ossp/uuid.h],
        [],
        [true])
      uuid_CFLAGS="${ossp_uuid_CFLAGS}"
      uuid_LIBS="${ossp_uuid_LIBS}"
    fi
    ;;
esac

# now create the variables we actually use.
AC_SUBST(uuid_CFLAGS)
AC_SUBST(uuid_LIBS)

AS_IF([test "x$use_ossp_uuid" = xyes],
      [AC_DEFINE([USE_OSSP_UUID], [1],
                 [Defined if we should use the ossp uuid lib])])

# DNS-SD support
# We use either avahi or the Apple DNS-SD library.
AC_ARG_WITH(dns-sd,
[AS_HELP_STRING([--without-dns-sd], [disable DNS-SD support])],,)
if test "x$with_dns_sd" != "xno"; then
	# dns_sd
	AC_CHECK_HEADER(
	  [dns_sd.h],
	  [AC_SEARCH_LIBS(DNSServiceRegister, [dns_sd], [have_dnssd="yes"])])

	# avahi
	PKG_CHECK_MODULES(
	  avahi,
	  [avahi-client],
	  [have_avahi="yes"],
	  [true])
fi

AS_IF([test "x$have_dnssd" = xyes],
      [AC_DEFINE([HAVE_DNSSD], [1], [Defined to use Bonjour DNS_SD])])
AM_CONDITIONAL([HAVE_DNSSD], [test "x$have_dnssd" = xyes])

AS_IF([test "x$have_avahi" = xyes],
      [AC_DEFINE([HAVE_AVAHI], [1], [Defined to use Avahi])])
AM_CONDITIONAL([HAVE_AVAHI], [test "x$have_avahi" = xyes])

# Look for -lSaleaeDevice, if we have it we build the logic sniffer.
SALEAE_DEVICE


# Features
#####################################################

# Cppunit, which is used by the tests.
AC_ARG_ENABLE(
  [unittests],
  [AS_HELP_STRING([--disable-unittests], [Disable all unittests])])
AS_IF([test "x$enable_unittests" != xno],
      [PKG_CHECK_MODULES([CPPUNIT], [cppunit >= 1.9.6], [],
          [AC_MSG_ERROR([Missing cppunit, please install >= 1.9.6])])
      ])
AM_CONDITIONAL([BUILD_TESTS], [test "x$enable_unittests" != xno])

# Optionally install the E1.33 library.
AC_ARG_ENABLE(
  [e133],
  [AS_HELP_STRING([--enable-e133], [Install the E1.33 library])])
# For now we install the ACN lib if the E1.33 lib is requested
AM_CONDITIONAL([INSTALL_ACN], [test "x$enable_e133" = xyes])
AM_CONDITIONAL([INSTALL_E133], [test "x$enable_e133" = xyes])

# Decide if we should build the example programs.
AC_ARG_ENABLE(
  [examples],
  [AS_HELP_STRING([--disable-examples],
                  [Disable the OLA example client programs])])
AM_CONDITIONAL([BUILD_EXAMPLES], [test "x$enable_examples" != xno])

# Make -Werror non-fatal. By default errors cause the build to abort. We do
# this to catch problems early so lets be nice and give the user a work around.
AC_ARG_ENABLE(
  [fatal-warnings],
  [AS_HELP_STRING([--disable-fatal-warnings],
                  [Make compiler warnings non-fatal])])
AM_CONDITIONAL([FATAL_WARNINGS], [test "x$enable_fatal_warnings" != xno])

# Enable gcov to produce coverage data.
AC_ARG_ENABLE(
  [gcov],
  [AS_HELP_STRING([--enable-gcov], [Turn on code coverage analysis tools])])
AS_IF([test "x$enable_gcov" = xyes],
      [CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
       CXXFLAGS="$CXXFLAGS -fprofile-arcs -ftest-coverage"
       LIBS="$LIBS -lgcov"])

# Enable HTTP support. This requires libmicrohttpd.
AC_ARG_ENABLE(
  [http],
  [AS_HELP_STRING([--disable-http], [Disable the built in HTTP server])])

have_microhttpd="no"
AS_IF([test "x$enable_http" != xno],
      [PKG_CHECK_MODULES([libmicrohttpd], [libmicrohttpd],
                         [have_microhttpd="yes"], [true])])

AS_IF([test "x$enable_http" = xyes && test "x$have_microhttpd" != xyes],
      AC_MSG_ERROR([--enable-http was given but libmicrohttpd was not found.]))

AM_CONDITIONAL([HAVE_LIBMICROHTTPD], [test "x$have_microhttpd" = xyes])
AS_IF([test "x$have_microhttpd" = xyes],
      [AC_DEFINE([HAVE_LIBMICROHTTPD], [1],
                 [define if libmicrohttpd is installed])])

if test "x$have_microhttpd" = xyes; then
  # Check if we have MHD_create_response_from_buffer
  old_cflags=$CFLAGS
  old_libs=$LIBS
  CFLAGS="${CPPFLAGS} ${libmicrohttpd_CFLAGS}"
  LIBS="${LIBS} ${libmicrohttpd_LIBS}"
  AC_CHECK_FUNCS([MHD_create_response_from_buffer])
  # restore CFLAGS
  CFLAGS=$old_cflags
  LIBS=$old_libs
fi

# Java API, this requires Maven
AC_ARG_ENABLE(
  [java-libs],
  [AS_HELP_STRING([--enable-java-libs], [Build the Java interface])],
  ,
  enable_java_libs="no")
AS_IF([test "x$enable_java_libs" = xyes],
      [AC_PROG_JAVA_CC()
       MAVEN_SUPPORT([2.2.1])])
AM_CONDITIONAL([BUILD_JAVA_LIBS], [test "x$enable_java_libs" = xyes])

# Optionally build the Ja Rule tool
AC_ARG_ENABLE(
  [ja-rule],
  [AS_HELP_STRING([--enable-ja-rule], [Build the Ja Rule test tool])])

# libftdi0: this requires libusb-0.1.4+ don't know if I should also
# check for it, since the libusb check above is not for 0.1.4 but for libusb-1
AC_ARG_ENABLE(
  [libftdi],
  [AS_HELP_STRING([--disable-libftdi],
                  [Avoid using libftdi0 or libftdi1 even if either exists])])

have_libftdi0="no"
AS_IF([test "x$enable_libftdi" != xno],
      [PKG_CHECK_MODULES([libftdi0], [libftdi >= 0.18], [have_libftdi0="yes"],
                         [true])])
AM_CONDITIONAL([HAVE_LIBFTDI0], [test "x$have_libftdi0" = xyes])
AS_IF([test "x$have_libftdi0" = xyes],
      [AC_DEFINE([HAVE_LIBFTDI0], [1], [define if libftdi0 is installed])])
have_libftdi1="no"
AS_IF([test "x$enable_libftdi" != xno],
      [PKG_CHECK_MODULES([libftdi1], [libftdi1 >= 1.0], [have_libftdi1="yes"],
                         [true])])
AM_CONDITIONAL([HAVE_LIBFTDI1], [test "x$have_libftdi1" = xyes])
AS_IF([test "x$have_libftdi1" = xyes],
      [AC_DEFINE([HAVE_LIBFTDI1], [1], [define if libftdi1 is installed])])
have_libftdi="no"
if test "x$have_libftdi0" = xyes || test "x$have_libftdi1" = xyes; then
  have_libftdi="yes"
fi
AS_IF([test "x$have_libftdi" = xyes],
      [AC_DEFINE([HAVE_LIBFTDI], [1], [define if libftdi0 or libftdi1 is installed])])

# libusb
AC_ARG_ENABLE(
  [libusb],
  [AS_HELP_STRING([--disable-libusb], [Avoid using libusb even if it exists])])

have_libusb="no"
AS_IF([test "x$enable_libusb" != xno],
      [PKG_CHECK_MODULES([libusb], [libusb-1.0 >= 1.0.2], [have_libusb="yes"],
                         [true])])
AS_IF([test "x$have_libusb" = xyes],
      [AC_DEFINE([HAVE_LIBUSB], [1], [define if libusb is installed])])

have_libusb_error_name="no"
AS_IF([test "x$enable_libusb" != xno],
      [PKG_CHECK_MODULES([libusb_error_name], [libusb-1.0 >= 1.0.9],
                         [have_libusb_error_name="yes"], [true])])
AS_IF([test "x$have_libusb_error_name" = xyes],
      [AC_DEFINE([HAVE_LIBUSB_ERROR_NAME], [1],
                 [define if libusb is installed and has libusb_error_name])])

have_libusb_hotplug_api="no"
AS_IF([test "x$enable_libusb" != xno],
      [PKG_CHECK_MODULES([libusb_hotplug_api], [libusb-1.0 >= 1.0.16],
                         [have_libusb_hotplug_api="yes"], [true])])
AS_IF([test "x$have_libusb_hotplug_api" = xyes],
      [AC_DEFINE([HAVE_LIBUSB_HOTPLUG_API], [1],
                 [define if libusb is installed and the libusb hotplug API is available])])
AM_CONDITIONAL([HAVE_LIBUSB_HOTPLUG_API], [test "x$have_libusb_hotplug_api" = xyes])

have_libusb_set_option="no"
AS_IF([test "x$enable_libusb" != xno],
      [PKG_CHECK_MODULES([libusb_set_option], [libusb-1.0 >= 1.0.22],
                         [have_libusb_set_option="yes"], [true])])
AS_IF([test "x$have_libusb_set_option" = xyes],
      [AC_DEFINE([HAVE_LIBUSB_SET_OPTION], [1],
                 [define if libusb is installed and libusb_set_option is available])])
AM_CONDITIONAL([HAVE_LIBUSB_SET_OPTION], [test "x$have_libusb_set_option" = xyes])

# UART direct plugin
AC_ARG_ENABLE(
  [uart],
  [AS_HELP_STRING([--disable-uart],
                  [Avoid using native uart support even if it exists])])

have_uart="no"
AS_IF([test "x$enable_uart" != xno],
      [AC_CHECK_HEADERS([asm/termios.h], [have_uart="yes"], [true])])
AS_IF([test "x$have_uart" = xyes],
      [AC_DEFINE([HAVE_UART], [1], [define if native UART support exists])])


# OSC Support.
# look for liblo if the user asked us to
AC_ARG_ENABLE(
  [osc],
  [AS_HELP_STRING([--disable-osc],
                  [Disable the OSC plugin even if liblo exists])])

have_liblo="no"
AS_IF([test "x$enable_osc" != xno],
      [PKG_CHECK_MODULES(liblo, [liblo >= 0.26], [have_liblo="yes"], [true])])
AS_IF([test "x$have_liblo" = xyes],
      [AC_DEFINE([HAVE_LIBLO], [1], [define if liblo is installed])])

# Python API
AC_ARG_ENABLE(
  [python-libs],
  [AS_HELP_STRING([--enable-python-libs], [Build the Python API Module])],
  ,
  enable_python_libs="no")

# RDM tests, if requested this enables the Python API as well.
AC_ARG_ENABLE(
  [rdm-tests],
  [AS_HELP_STRING(
      [--enable-rdm-tests],
      [Install the RDM responder tests, adds --enable-python-libs])],
  ,
  enable_rdm_tests="no")
AS_IF([test "x$enable_rdm_tests" = xyes], [enable_python_libs="yes"])
AM_CONDITIONAL([INSTALL_RDM_TESTS], [test "x$enable_rdm_tests" = xyes])

# By default olad refuses to run as root. However some people want to use root
# for embedded platforms so we give them an option.
AC_ARG_ENABLE(
  [root-check],
  [AS_HELP_STRING([--disable-root-check],
                  [Disable the check that prevents olad from running as root])])
AS_IF([test "x$enable_root_check" = xno],
      [AC_DEFINE([OLAD_SKIP_ROOT_CHECK], [1],
                 [Defined if olad is allowed to run as root])])

# Use tcmalloc. This is used by the buildbot leak checks.
AC_ARG_ENABLE([tcmalloc], AS_HELP_STRING([--enable-tcmalloc], [Use tcmalloc]))
AS_IF([test "x$enable_tcmalloc" = xyes],
      [AC_CHECK_LIB([tcmalloc], [malloc], [AM_LDFLAGS="$AM_LDFLAGS -ltcmalloc"], [AS_ERROR([tcmalloc not found, but enabled on command line])])])

# Optionally set the Doxygen version to "Latest Git" for website latest
# version.
AC_ARG_ENABLE(
  [doxygen-version],
  [AS_HELP_STRING([--disable-doxygen-version],
  [Substitute the Doxygen version with latest, for the website])])

# UUCP Lock directory
AC_ARG_WITH([uucp-lock],
            [AS_HELP_STRING([--with-uucp-lock],
                            [The directory to use for UUCP lock files])],
            [], [])

# Try to locate the UUCP lock path.
UUCPLOCK=""
AH_TEMPLATE([UUCP_LOCK_DIR], [Define to the path of the UUCP lock directory])
AC_MSG_CHECKING(for serial port lock directory)

# This list is from the minicom configs, The Filesystem Hierarchy Standard says
# /var/lock is the one true source.
for ac_uucplock in /var/lock /etc/locks /usr/spool/locks /var/spool/locks /var/spool/lock /usr/spool/uucp /var/spool/uucp /var/run; do
  AS_IF([test -d $ac_uucplock], [UUCPLOCK=$ac_uucplock; break])
done

# Mac stores the lock files in /tmp
case "${host_os}" in
  *darwin*)
    UUCPLOCK=/tmp
    ;;
esac

AS_IF([test "x$with_uucp_lock" != x], [UUCPLOCK=$with_uucp_lock])
AS_IF([test -d $UUCPLOCK],
      [AC_DEFINE_UNQUOTED(UUCP_LOCK_DIR, "$UUCPLOCK")
       AC_MSG_RESULT($UUCPLOCK)],
      AC_MSG_ERROR(no suitable lock directory))

# Python wrappers & RDM Responder tests.
#####################################################
AM_CONDITIONAL([BUILD_PYTHON_LIBS], [test "x$enable_python_libs" = xyes])

AS_IF([test "${enable_python_libs}" = "yes"],
      [AM_PATH_PYTHON(2.6)
       AS_IF([test -z $PYTHON], [PYTHON="python"])
       PYTHON_NAME=`basename $PYTHON`
       AC_CACHE_CHECK([for $PYTHON_NAME module: google.protobuf],
          [ac_cv_have_pymod_google_protobuf],
          [AX_PYTHON_MODULE([google.protobuf], [fatal])
           eval ac_cv_have_pymod_google_protobuf=\$AS_TR_CPP([HAVE_PYMOD_google.protobuf])])
     ])

AS_IF([test "${enable_rdm_tests}" = "yes"],
      [AC_CACHE_CHECK([for $PYTHON_NAME module: numpy],
          [ac_cv_have_pymod_numpy],
          [AX_PYTHON_MODULE([numpy], [fatal])
           eval ac_cv_have_pymod_numpy=\$AS_TR_CPP([HAVE_PYMOD_numpy])])
     ])

# Check if the flake8 python linter is available
AC_CHECK_PROG([flake8],[flake8],[yes],[no])
AM_CONDITIONAL([FOUND_FLAKE8], [test "x$flake8" = xyes])

# Libraries, that depend on the feature args above.
#####################################################

# LIBRARY: protobuf
# We require 2.4.0 for the java libs
AS_IF([test "x$build_java_libs" = xyes],
      [PROTOBUF_SUPPORT([2.4.0])],
      [PROTOBUF_SUPPORT([2.3.0])])
# Version 3.7 and above of protoc require some additional includes
AC_CHECK_HEADERS([google/protobuf/io/strtod.h google/protobuf/stubs/logging.h \
                  google/protobuf/stubs/stl_util.h])


# Doxygen
#####################################################
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)

DX_ARG_ABLE(verbose, [Verbose doxygen output],
            [], [], [],
            [DX_ENV_APPEND(QUIET, NO)],
            [DX_ENV_APPEND(QUIET, YES)])
doxygen_project_number=$PACKAGE_VERSION
AS_IF([test "x$enable_doxygen_version" = xno],
      [doxygen_project_number="Latest Git"])
DX_ENV_APPEND(PROJECT_NUMBER, $doxygen_project_number)

DX_INIT_DOXYGEN($PACKAGE_NAME, Doxyfile)

# Plugins
#####################################################

AC_ARG_ENABLE(
  [all-plugins],
  AS_HELP_STRING(
      [--disable-all-plugins],
      [Disable all plugins, then enable the specific ones you want]))

# We build a list of plugins that we're going to compile here so the olad
# knows what to link against.
PLUGINS=""

# Force enable_usbdmx to yes when Ja Rule is explicitly requested.
AS_IF([test "x$enable_ja_rule" = xyes],
      [AS_ECHO(["Ja Rule is enabled, enabling the usbdmx plugin."])]
      [enable_usbdmx="yes"])

PLUGIN_SUPPORT(artnet, USE_ARTNET)
PLUGIN_SUPPORT(dmx4linux, USE_DMX4LINUX, [$have_dmx4linux])
PLUGIN_SUPPORT(dummy, USE_DUMMY)
PLUGIN_SUPPORT(e131, USE_E131)
PLUGIN_SUPPORT(espnet, USE_ESPNET)
PLUGIN_SUPPORT(ftdidmx, USE_FTDI, [$have_libftdi])
PLUGIN_SUPPORT(gpio, USE_GPIO)
PLUGIN_SUPPORT(karate, USE_KARATE)
PLUGIN_SUPPORT(kinet, USE_KINET)
PLUGIN_SUPPORT(milinst, USE_MILINST)
PLUGIN_SUPPORT(nanoleaf, USE_NANOLEAF)
PLUGIN_SUPPORT(opendmx, USE_OPENDMX, [$have_not_win32])
PLUGIN_SUPPORT(openpixelcontrol, USE_OPENPIXELCONTROL)
PLUGIN_SUPPORT(osc, USE_OSC, [$have_liblo])
PLUGIN_SUPPORT(pathport, USE_PATHPORT)
PLUGIN_SUPPORT(renard, USE_RENARD)
PLUGIN_SUPPORT(sandnet, USE_SANDNET)
PLUGIN_SUPPORT(shownet, USE_SHOWNET)
PLUGIN_SUPPORT(spi, USE_SPI, [$have_spi])
PLUGIN_SUPPORT(spidmx, USE_SPIDMX, [$have_spi])
PLUGIN_SUPPORT(stageprofi, USE_STAGEPROFI)
PLUGIN_SUPPORT(uartdmx, USE_UART, [$have_uart])
PLUGIN_SUPPORT(usbdmx, USE_LIBUSB, [$have_libusb])
PLUGIN_SUPPORT(usbpro, USE_USBPRO)

# Finally build a list of the required plugin libs.
PLUGIN_LIBS=''
OLA_SERVER_LIBS=''
for p in $PLUGINS; do
  PLUGIN_LIBS="$PLUGIN_LIBS plugins/${p}/libola${p}.la"
  OLA_SERVER_LIBS="$OLA_SERVER_LIBS -lola${p}"
done

if test -z "${USING_WIN32_TRUE}"; then
  OLA_SERVER_LIBS="$OLA_SERVER_LIBS -lWs2_32 -lIphlpapi -lpthread"
fi

AC_SUBST(PLUGIN_LIBS)
AC_SUBST(OLA_SERVER_LIBS)

# HTML & data directories
ola_datadir=$datadir/olad
www_datadir=$ola_datadir/www
piddatadir=$datadir/ola/pids
AC_SUBST(www_datadir)
AC_SUBST(piddatadir)

# Additional libraries needed by Windows clients
OLA_CLIENT_LIBS=''
if test -z "${USING_WIN32_TRUE}"; then
  OLA_CLIENT_LIBS="-lWs2_32 -lIphlpapi -lpthread"
fi
AC_SUBST(OLA_CLIENT_LIBS)

# Extra tools
#####################################################
AS_IF([test "x$enable_ja_rule" = xyes && test "x$enable_usbdmx" = xno],
      [AC_MSG_ERROR([Ja Rule requires the usbdmx plugin, but it could not be enabled.])])
AS_IF([test "x$enable_ja_rule" = xyes && test "x$have_libusb" = xno],
      [AC_MSG_ERROR([Ja Rule requires libusb, but it was not found on your system.])])

AM_CONDITIONAL([BUILD_JA_RULE], [test "x$enable_ja_rule" = xyes])

# Status just for configure
BUILDING_JA_RULE='no'
if test "x$enable_ja_rule" = xyes; then
  BUILDING_JA_RULE='yes'
fi

# Linters
AC_CHECK_PROG([flake8],[flake8],[yes],[no])
AM_CONDITIONAL([FOUND_FLAKE8], [test "x$flake8" = xyes])
AC_CHECK_PROG([cpplint],[cpplint],[yes],[no])
AM_CONDITIONAL([FOUND_CPPLINT], [test "x$cpplint" = xyes])

# Output
#####################################################
# Hack alert!
# Python modules can't be split over two directories. This is a problem when
# doing VPATH builds since the generated files like
# ${builddir}/python/ola/Ola_pb.py will be in a separate path ( $builddir ) from
# the non-generated files $srcdir/python/ols/PidStore.py). To get the tests
# to pass we symlink the files we need for the tests from the builddir to the
# srcdir and set PYTHONPATH=${top_builddir}/python in data/rdm/Makefile.am
AC_CONFIG_LINKS([python/ola/__init__.py:python/ola/__init__.py
                 python/ola/ClientWrapper.py:python/ola/ClientWrapper.py
                 python/ola/DMXConstants.py:python/ola/DMXConstants.py
                 python/ola/DUBDecoder.py:python/ola/DUBDecoder.py
                 python/ola/MACAddress.py:python/ola/MACAddress.py
                 python/ola/OlaClient.py:python/ola/OlaClient.py
                 python/ola/PidStore.py:python/ola/PidStore.py
                 python/ola/RDMAPI.py:python/ola/RDMAPI.py
                 python/ola/RDMConstants.py:python/ola/RDMConstants.py
                 python/ola/StringUtils.py:python/ola/StringUtils.py
                 python/ola/TestUtils.py:python/ola/TestUtils.py
                 python/ola/UID.py:python/ola/UID.py
                 python/ola/rpc/__init__.py:python/ola/rpc/__init__.py
                 python/ola/rpc/SimpleRpcController.py:python/ola/rpc/SimpleRpcController.py
                 python/ola/rpc/StreamRpcChannel.py:python/ola/rpc/StreamRpcChannel.py
                 tools/rdm/__init__.py:tools/rdm/__init__.py
                 tools/rdm/ExpectedResults.py:tools/rdm/ExpectedResults.py
                 tools/rdm/ResponderTest.py:tools/rdm/ResponderTest.py
                 tools/rdm/TestCategory.py:tools/rdm/TestCategory.py
                 tools/rdm/TestDefinitions.py:tools/rdm/TestDefinitions.py
                 tools/rdm/TestHelpers.py:tools/rdm/TestHelpers.py
                 tools/rdm/TestMixins.py:tools/rdm/TestMixins.py
                 tools/rdm/TestRunner.py:tools/rdm/TestRunner.py
                 tools/rdm/TestState.py:tools/rdm/TestState.py
                 tools/rdm/TimingStats.py:tools/rdm/TimingStats.py])

# Non-makefile generated files.
AC_CONFIG_FILES([include/ola/base/Version.h
                 libola.pc
                 libolaserver.pc
                 libs/acn/libolaacn.pc
                 ola.spec
                 plugins/artnet/messages/libolaartnetconf.pc
                 plugins/e131/messages/libolae131conf.pc
                 plugins/usbpro/messages/libolausbproconf.pc
                 tools/e133/libolae133common.pc
                 tools/e133/libolae133controller.pc
                 ])
# Makefiles
AC_CONFIG_FILES([Makefile java/Makefile])
AC_SUBST(AM_LDFLAGS)
AC_OUTPUT

echo \
"-------------------------------------------------------
${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Prefix: '${prefix}'
Compiler: '${CXX} ${CXXFLAGS} ${CPPFLAGS}'
Linker: '${LD} ${AM_LDFLAGS} ${LIBS}'
Python: ${PYTHON}

Python API: ${enable_python_libs}
Java API: ${enable_java_libs}
Enable HTTP Server: ${have_microhttpd}
RDM Responder Tests: ${enable_rdm_tests}
Ja Rule: ${BUILDING_JA_RULE}
Enabled Plugins:${PLUGINS}
UUCP Lock Directory: $UUCPLOCK

Now type 'make @<:@<target>@:>@'
  where the optional <target> is:
    all          - build everything
    check        - run the tests
    doxygen-doc  - generate the HTML documentation
    lint         - run the linters
-------------------------------------------------------"
