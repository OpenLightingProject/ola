#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([OLA], m4_esyscmd([./version.sh | tr -d '\n']),
        [open-lighting@googlegroups.com])
AC_CONFIG_SRCDIR([libola.pc.in])
AC_CONFIG_AUX_DIR([config])
AC_SUBST([ac_aux_dir])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE()

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
AC_LIBTOOL_WIN32_DLL

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_RESOLV
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h malloc.h netinet/in.h stdint.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/socket.h sys/time.h sys/timeb.h syslog.h termios.h unistd.h])
AC_CHECK_HEADERS([assert.h endian.h errno.h execinfo.h \
                  linux/if_packet.h math.h net/ethernet.h \
                  net/if_arp.h net/route.h sys/sysctl.h sys/types.h sys/uio.h \
                  sysexits.h])
AC_CHECK_HEADERS([random])

# These both need sa_family_t from bits/sockaddr.h
AC_CHECK_HEADERS([bits/sockaddr.h linux/netlink.h linux/rtnetlink.h], [], [],
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
                inet_ntoa inet_aton select socket strerror getifaddrs \
                getloadavg getpwnam_r getpwuid_r getgrnam_r getgrgid_r])
AX_HAVE_EPOLL([AC_DEFINE([HAVE_EPOLL], [1], [Defined if epoll exists])]) # epoll

LT_INIT
AC_PROG_LIBTOOL

# Doxygen
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN($PACKAGE_NAME, Doxyfile)

# Decide if we're building on Windows early on.
AM_CONDITIONAL([USING_WIN32], [test "x$host_os" = xmingw32])

# check if the compiler supports -rdyanmic
AC_MSG_CHECKING(for -rdynamic support)
old_cppflags=$CPPFLAGS
CPPFLAGS="${CPPFLAGS} -rdynamic -Wall -Werror"
AC_CACHE_VAL(ac_cv_rdynamic,
  AC_TRY_COMPILE([], [], ac_cv_rdynamic=yes, ac_cv_rdynamic=no))
CPPFLAGS=$old_cppflags
AC_MSG_RESULT($ac_cv_rdynamic)

AM_CONDITIONAL([SUPPORTS_RDYNAMIC], [test "x$ac_cv_rdynamic" = xyes])

# check for ipv6 support - taken from unp
AC_MSG_CHECKING(for IPv6 support)
AC_CACHE_VAL(ac_cv_ipv6,
        AC_TRY_RUN([
#       include <stdlib.h>
#       include <sys/types.h>
#       include <sys/socket.h>
#       include <netinet/in.h>
        /* Make sure the definitions for AF_INET6 and struct sockaddr_in6
         * are defined, and that we can actually create an IPv6 TCP socket.
         */
        main()
        {
                int fd;
                struct sockaddr_in6 foo;
                fd = socket(AF_INET6, SOCK_STREAM, 0);
                exit(fd >= 0 ? 0 : 1);
        }],
        ac_cv_ipv6=yes,
        ac_cv_ipv6=no,
        ac_cv_ipv6=no))
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

# dmx4linux
have_dmx4linux="no"
AC_CHECK_LIB(dmx4linux, DMXdev, [have_dmx4linux="yes"])
AC_CHECK_HEADER([dmx/dmxioctl.h], [], [have_dmx4linux="no"])

AS_IF([test "x$have_dmx4linux" = xyes],
      [AC_DEFINE([HAVE_DMX4LINUX], [1], [define if dmx4linux is installed])])

# libexecinfo
# FreeBSD required -lexecinfo to call backtrace - checking for presence of
# header execinfo.h isn't enough
AC_CHECK_LIB([execinfo], [backtrace], [use_lexecinfo="yes"])
case "${host_os}" in
  *freebsd*)
  LIBS="$LIBS -lexecinfo"
  ;;
esac

# libftd2xx (for now this is intentionally disabled, TODO)
have_libftd2xx="no"
AM_CONDITIONAL([HAVE_LIBFTD2XX], [test "x$have_libftd2xx" = xyes])

# ncurses
AC_CHECK_LIB([ncurses], [initscr], [have_ncurses="yes"])
AM_CONDITIONAL([HAVE_NCURSES], [test "x$have_ncurses" = xyes])

# pthread
ACX_PTHREAD([
             LIBS="$PTHREAD_LIBS $LIBS"
             CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
             CXXFLAGS="$CXXFLAGS $PTHREAD_CXXFLAGS"
             CC="$PTHREAD_CC"
             CXX="$PTHREAD_CXX"],
            [AC_MSG_ERROR([Missing pthread, please install it])])

# resolv
SAVED_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -lresolv"
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <resolv.h>],
    [res_init()])],
  [TEST_LIBS="$TEST_LIBS -lresolv"] [],
  [AC_MSG_ERROR([Missing resolv, please install it])])
LDFLAGS=$SAVED_LDFLAGS

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

# dns_sd
AC_CHECK_HEADER(
  [dns_sd.h],
  [AC_SEARCH_LIBS(DNSServiceRegister, [dns_sd], [have_dnssd="yes"])])

AS_IF([test "x$have_dnssd" = xyes],
      [AC_DEFINE([HAVE_DNSSD], [1], [Defined to use Bonjour DNS_SD])])
AM_CONDITIONAL([HAVE_DNSSD], [test "x$have_dnssd" = xyes])

# avahi
PKG_CHECK_MODULES(
  avahi,
  [avahi-client],
  [have_avahi="yes"],
  [true])

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
      [AM_PATH_CPPUNIT([1.9.6], [],
          [AC_MSG_ERROR([Missing cppunit, please install >= 1.9.6])])
      ])
AM_CONDITIONAL([BUILD_TESTS], [test "x$enable_unittests" != xno])

# Optionally install the E1.33 library.
AC_ARG_ENABLE(
  [e133],
  [AS_HELP_STRING([--install-e133], [Install the E1.33 library])])
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
AS_IF([test "x$enable_gov" = xyes],
      [CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
       CXXFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"])

# Enable HTTP support. This requires libmicrohttpd.
AC_ARG_ENABLE(
  [http],
  [AS_HELP_STRING([--disable-http], [Disable the built in HTTP server])])

have_microhttpd="no"
AS_IF([test "x$enable_http" != xno],
      [PKG_CHECK_MODULES([libmicrohttpd], [libmicrohttpd],
                         [have_microhttpd="yes"], [true])])
AM_CONDITIONAL([HAVE_LIBMICROHTTPD], [test "x$have_microhttpd" = xyes])
AS_IF([test "x$have_microhttpd" = xyes],
      [AC_DEFINE([HAVE_LIBMICROHTTPD], [1],
                 [define if libmicrohttpd is installed])])

# Java API, this requires Maven
AC_ARG_ENABLE(
  [java-libs],
  [AS_HELP_STRING([--enable-java-libs], [Build the Java interface])])
AS_IF([test "x$enable_java_libs" = xyes],
      [AC_PROG_JAVA_CC()
       MAVEN_SUPPORT([2.2.1])])
AM_CONDITIONAL([BUILD_JAVA_LIBS], [test "x$enable_java_libs" = xyes])

# libftdi: this requires libusb-0.1.4+ don't know if I should also
# check for it, since the libusb check above is not for 0.1.4 but for libusb-1
AC_ARG_ENABLE(
  [libftdi],
  [AS_HELP_STRING([--disable-libftdi],
                  [Avoid using libftdi even if it exists])])

have_libftdi="no"
AS_IF([test "x$enable_libftdi" != xno],
      [PKG_CHECK_MODULES([libftdi], [libftdi >= 0.18], [have_libftdi="yes"],
                         [true])])
AS_IF([test "x$have_libftdi" = xyes],
      [AC_DEFINE([HAVE_LIBFTDI], [1], [define if libftdi is installed])])

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
  [AS_HELP_STRING([--enable-python-libs], [Build the Python API Module])])

# RDM tests, if requested this enables the Python API as well.
AC_ARG_ENABLE(
  [rdm-tests],
  [AS_HELP_STRING(
      [--enable-rdm-tests],
      [Install the RDM responder tests, adds --enable-python-libs])])
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

# OpenSLP is used for E1.33 support
# This should be removed if we decide that E1.33 is going to use DNS-SD.
# Openslp doesn't use pkg-config, so the users may need to pass CPPFLAGS
# & LDFLAGS so we can find it.
AC_ARG_ENABLE(
  [slp],
  AS_HELP_STRING([--disable-slp], [Disable the SLP tools]),)

openslp_LIBS=""
have_slp=no
AS_IF([test "x$enable_slp" != xno],
      [AC_CHECK_LIB([slp], [SLPOpen],
                    [have_slp="yes" openslp_LIBS="-lslp"])])

AM_CONDITIONAL([HAVE_SLP], [test "x$have_slp" = xyes])
AS_IF([test "x$have_slp" = xyes],
      [AC_DEFINE([HAVE_LIBSLP], [1], [define if libslp is installed])])
AC_SUBST(openslp_LIBS)

# Use tcmalloc. This is used by the buildbot leak checks.
AC_ARG_ENABLE([tcmalloc], AS_HELP_STRING([--enable-tcmalloc], [Use tcmalloc]))
AS_IF([test "x$enable_tcmalloc" = xyes],
      [LDFLAGS="$LDFLAGS -ltcmalloc"])


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

# Libraries, that depend on the feature args above.
#####################################################

# LIBRARY: protobuf
# We require 2.4.0 for the java libs
AS_IF([test "x$build_java_libs" = xyes],
      [PROTOBUF_SUPPORT([2.4.0])],
      [PROTOBUF_SUPPORT([2.3.0])])


# Plugins
#####################################################

# We build a list of plugins that we're going to compile here so the olad
# knows what to link against.
PLUGINS=""

PLUGIN_SUPPORT(artnet, USE_ARTNET)
PLUGIN_SUPPORT(dmx4linux, USE_DMX4LINUX, [$have_dmx4linux])
PLUGIN_SUPPORT(dummy, USE_DUMMY)
PLUGIN_SUPPORT(e131, USE_E131)
PLUGIN_SUPPORT(espnet, USE_ESPNET)
PLUGIN_SUPPORT(ftdidmx, USE_FTDI, [$have_libftdi])
PLUGIN_SUPPORT(karate, USE_KARATE)
PLUGIN_SUPPORT(kinet, USE_KINET)
PLUGIN_SUPPORT(milinst, USE_MILINST)
PLUGIN_SUPPORT(opendmx, USE_OPENDMX)
PLUGIN_SUPPORT(osc, USE_OSC, [$have_liblo])
PLUGIN_SUPPORT(pathport, USE_PATHPORT)
PLUGIN_SUPPORT(renard, USE_RENARD)
PLUGIN_SUPPORT(sandnet, USE_SANDNET)
PLUGIN_SUPPORT(shownet, USE_SHOWNET)
PLUGIN_SUPPORT(spi, USE_SPI, [$have_spi])
PLUGIN_SUPPORT(stageprofi, USE_STAGEPROFI)
PLUGIN_SUPPORT(usbpro, USE_USBPRO)
PLUGIN_SUPPORT(usbdmx, USE_LIBUSB, [$have_libusb])

# Finally build a list of the required plugin libs.
PLUGIN_LIBS=''
OLA_SERVER_LIBS=''
for p in $PLUGINS; do
  PLUGIN_LIBS="$PLUGIN_LIBS \$(top_builddir)/plugins/${p}/libola${p}.la"
  OLA_SERVER_LIBS="$OLA_SERVER_LIBS -lola${p}"
done

AC_SUBST(PLUGIN_LIBS)
AC_SUBST(OLA_SERVER_LIBS)

# plugin & html data directories
plugindir=$libdir/olad
ola_datadir=$datadir/olad
www_datadir=$ola_datadir/www
piddatadir=$datadir/ola/pids
AC_SUBST(plugindir)
AC_SUBST(www_datadir)
AC_SUBST(piddatadir)

# Output
#####################################################
# Hack alert!
# Python modules can't be split over two directories. This is a problem when
# doing VPATH builds since the generated files like
# ${builddir}/python/ola/Ola_pb.py will be in a separate path ( $builddir ) from
# the non-generated files $srcdir/python/ols/PidStore.py). To get the tests
# to pass we symlink the files we need for the tests from the builddir to the
# srcdir and set PYTHONPATH=${top_builddir}/python in data/rdm/Makefile.am
AC_CONFIG_LINKS([python/ola/PidStore.py:python/ola/PidStore.py
                 python/ola/MACAddress.py:python/ola/MACAddress.py
                 python/ola/RDMConstants.py:python/ola/RDMConstants.py
                 python/ola/UID.py:python/ola/UID.py
                 python/ola/__init__.py:python/ola/__init__.py])

# Non-makefile generted files.
AC_CONFIG_FILES([Doxyfile
                 libola.pc
                 libolaserver.pc
                 plugins/artnet/messages/libolaartnetconf.pc
                 plugins/e131/e131/libolaacn.pc
                 plugins/e131/messages/libolae131conf.pc
                 plugins/usbpro/messages/libolausbproconf.pc
                 slp/libolaslpclient.pc
                 slp/libolaslpserver.pc
                 tools/e133/libolae133common.pc
                 tools/e133/libolae133controller.pc
                 tools/e133/libolae133slp.pc
                 ])
# Makefiles
AC_CONFIG_FILES([Makefile
                 common/Makefile
                 common/base/Makefile
                 common/dmx/Makefile
                 common/export_map/Makefile
                 common/file/Makefile
                 common/http/Makefile
                 common/io/Makefile
                 common/math/Makefile
                 common/messaging/Makefile
                 common/network/Makefile
                 common/protocol/Makefile
                 common/rdm/Makefile
                 common/rdm/testdata/Makefile
                 common/rdm/testdata/pids/Makefile
                 common/rpc/Makefile
                 common/system/Makefile
                 common/testing/Makefile
                 common/thread/Makefile
                 common/timecode/Makefile
                 common/utils/Makefile
                 common/web/Makefile
                 data/Makefile
                 data/rdm/Makefile
                 debian/Makefile
                 doxygen/Makefile
                 doxygen/examples/Makefile
                 examples/Makefile
                 include/Makefile
                 include/ola/Makefile
                 include/ola/acn/Makefile
                 include/ola/base/Makefile
                 include/ola/client/Makefile
                 include/ola/dmx/Makefile
                 include/ola/e133/Makefile
                 include/ola/file/Makefile
                 include/ola/http/Makefile
                 include/ola/io/Makefile
                 include/ola/math/Makefile
                 include/ola/messaging/Makefile
                 include/ola/network/Makefile
                 include/ola/rdm/Makefile
                 include/ola/slp/Makefile
                 include/ola/stl/Makefile
                 include/ola/system/Makefile
                 include/ola/testing/Makefile
                 include/ola/thread/Makefile
                 include/ola/timecode/Makefile
                 include/ola/util/Makefile
                 include/ola/web/Makefile
                 include/olad/Makefile
                 java/Makefile
                 man/Makefile
                 ola/Makefile
                 olad/Makefile
                 olad/www/Makefile
                 plugins/Makefile
                 plugins/artnet/Makefile
                 plugins/artnet/messages/Makefile
                 plugins/dmx4linux/Makefile
                 plugins/dummy/Makefile
                 plugins/e131/Makefile
                 plugins/e131/e131/Makefile
                 plugins/e131/messages/Makefile
                 plugins/espnet/Makefile
                 plugins/ftdidmx/Makefile
                 plugins/karate/Makefile
                 plugins/kinet/Makefile
                 plugins/milinst/Makefile
                 plugins/opendmx/Makefile
                 plugins/osc/Makefile
                 plugins/pathport/Makefile
                 plugins/renard/Makefile
                 plugins/sandnet/Makefile
                 plugins/shownet/Makefile
                 plugins/spi/Makefile
                 plugins/stageprofi/Makefile
                 plugins/usbdmx/Makefile
                 plugins/usbpro/Makefile
                 plugins/usbpro/messages/Makefile
                 protoc/Makefile
                 python/Makefile
                 python/examples/Makefile
                 python/ola/Makefile
                 python/ola/rpc/Makefile
                 slp/Makefile
                 tools/Makefile
                 tools/e133/Makefile
                 tools/logic/Makefile
                 tools/ola_trigger/Makefile
                 tools/rdm/Makefile
                 tools/rdmpro/Makefile
                 tools/usbpro/Makefile])
AC_OUTPUT

echo \
"-------------------------------------------------------
${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Prefix: '${prefix}'
Compiler: '${CXX} ${CXXFLAGS} ${CPPFLAGS}'
Linker: '${LD} ${LDFLAGS} ${LIBS}'
Enable HTTP Server: ${have_microhttpd}

Now type 'make @<:@<target>@:>@'
  where the optional <target> is:
    all          - build everything
    check        - run the tests
    doxygen-doc  - generate the html documentation
-------------------------------------------------------"
