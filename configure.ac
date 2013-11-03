#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(ola, [0.8.33])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_AUX_DIR(config)
AC_SUBST(ac_aux_dir)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_LANG(C++)
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_HEADER_TIME
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Doxygen
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN($PACKAGE_NAME, Doxyfile)

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero gettimeofday memmove memset mkdir strdup strrchr \
                inet_ntoa inet_aton select socket strerror getifaddrs \
                getloadavg getpwnam_r getpwuid_r getgrnam_r getgrgid_r])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdint.h stdlib.h string.h \
                  sys/ioctl.h sys/socket.h sys/time.h syslog.h termios.h \
                  endian.h execinfo.h unistd.h linux/if_packet.h sysexits.h])
AC_CHECK_HEADERS([random])

AC_PROG_LIBTOOL

# windows platform support
AM_CONDITIONAL(USING_WIN32, test "$host_os" = 'mingw32')

# Disable -Werror
AC_ARG_ENABLE(
  fatal-warnings,
  AC_HELP_STRING([--disable-fatal-warnings], [Make compiler warnings non-fatal]),)

AM_CONDITIONAL(FATAL_WARNINGS, test "${enable_fatal_warnings}" != "no")

# check for hash_map
AC_CXX_STL_HASH

# check if the compiler supports -rdyanmic
AC_MSG_CHECKING(for -rdynamic support)
old_cppflags=$CPPFLAGS
CPPFLAGS="${CPPFLAGS} -rdynamic -Wall -Werror"
AC_CACHE_VAL(ac_cv_rdynamic,
  AC_TRY_COMPILE(
    [],
    [],
    ac_cv_rdynamic=yes,
    ac_cv_rdynamic=no))
CPPFLAGS=$old_cppflags
AC_MSG_RESULT($ac_cv_rdynamic)

AM_CONDITIONAL(SUPPORTS_RDYNAMIC, test "${ac_cv_rdynamic}" = "yes")

# check for lex
AC_PROG_LEX
if test "${LEX}" = ":"; then
 AC_MSG_ERROR([lex not found, please install flex or lex])
fi

# check for bison
BISON=""
AC_CHECK_PROG(BISON, [bison -V], bison)
if test "${BISON}" != "bison"; then
 AC_MSG_ERROR([bison not found, please install it])
fi
AC_SUBST(BISON)

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
if test $ac_cv_ipv6 = yes ; then
  AC_DEFINE(IPV6, 1, Define to 1 if the system supports IPv6)
fi

# Check if sockaddr{} has sa_len member.
AC_CHECK_MEMBER(
  [struct sockaddr.sa_len],
  AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, define if socket address structures have length fields),
  ,
  [#include <sys/types.h>
   #include <sys/socket.h>])

AC_CHECK_MEMBER(
  struct sockaddr_dl.sdl_family,
  AC_DEFINE(HAVE_SOCKADDR_DL_STRUCT, 1, define if we have sockaddr_dl),
  ,
  [#include <sys/types.h>
   #include <net/if_dl.h>])

# check for time_t and suseconds_t
AC_CHECK_TYPE(time_t,
              AC_DEFINE(HAVE_TIME_T, 1, define if we have time_t),
              ,
              [#include <sys/time.h>])
AC_CHECK_TYPE(suseconds_t,
              AC_DEFINE(HAVE_SUSECONDS_T, 1, define if we have suseconds_t),
              ,
              [#include <sys/time.h>])

# check for SO_NOSIGPIPE or MSG_NOSIGNAL
AC_CHECK_DECLS(MSG_NOSIGNAL,
               ,
               have_msg_no_pipe=no,
               [#include <sys/types.h>
                #include <sys/socket.h>])

AC_CHECK_DECLS(SO_NOSIGPIPE,
               ,
               have_so_no_pipe=no,
               [#include <sys/types.h>
                #include <sys/socket.h>])

AC_CHECK_DECLS(SO_REUSEADDR,
               ,
               have_so_reuseaddr=no,
               [#include <sys/types.h>
                #include <sys/socket.h>])

AC_CHECK_DECLS(SO_REUSEPORT,
               ,
               have_so_reuseport=no,
               [#include <sys/types.h>
                #include <sys/socket.h>])

if test -z "${USING_WIN32_FALSE}" && test "${have_msg_no_pipe}" = "no" && \
   test "${have_so_no_pipe}" = "no"; then
 AC_MSG_ERROR([Your system needs either MSG_NOSIGNAL or SO_NOSIGPIPE])
fi

# Check for pkg-config
PKG_PROG_PKG_CONFIG

if test -z "$PKG_CONFIG"; then
  AC_MSG_ERROR([Missing pkg-config, please install it])
fi

# We build a list of plugins that we're going to compile here so the olad
# knows what to link against.
PLUGINS=""

PLUGIN_SUPPORT(artnet, USE_ARTNET)
PLUGIN_SUPPORT(dummy, USE_DUMMY)
PLUGIN_SUPPORT(e131, USE_E131)
PLUGIN_SUPPORT(espnet, USE_ESPNET)
PLUGIN_SUPPORT(karate, USE_KARATE)
PLUGIN_SUPPORT(kinet, USE_KINET)
PLUGIN_SUPPORT(milinst, USE_MILINST)
PLUGIN_SUPPORT(opendmx, USE_OPENDMX)
PLUGIN_SUPPORT(pathport, USE_PATHPORT)
PLUGIN_SUPPORT(sandnet, USE_SANDNET)
PLUGIN_SUPPORT(shownet, USE_SHOWNET)

# SPI support is only enabled if the header is found
AC_CHECK_HEADER([linux/spi/spidev.h], [have_spi="yes"], [have_spi="no"])
PLUGIN_SUPPORT(spi, USE_SPI, $have_spi)
PLUGIN_SUPPORT(stageprofi, USE_STAGEPROFI)
PLUGIN_SUPPORT(usbpro, USE_USBPRO)

# LIBRARY: ncurses
AC_CHECK_LIB([ncurses], [initscr], [have_ncurses="yes"])
AM_CONDITIONAL(HAVE_NCURSES, test "${have_ncurses}" = "yes")

# LIBRARY: pthread
AC_CHECK_HEADER([pthread.h],
                [],
                [AC_MSG_ERROR([Missing pthread, please install it])])
AC_CHECK_LIB([pthread], [pthread_create], [use_lpthread="yes"])
if test "${use_lpthread}" = "yes"; then
  LIBS="-lpthread $LIBS"
fi

# Java API, this requires Maven
AC_ARG_ENABLE(java-libs,
              AC_HELP_STRING([--enable-java-libs], [Build the Java interface]))
if test "${enable_java_libs}" = "yes"; then
  AC_PROG_JAVA_CC()
  MAVEN_SUPPORT([2.2.1])
fi

AM_CONDITIONAL(BUILD_JAVA_LIBS, test "${enable_java_libs}" = "yes")


# LIBRARY: protobuf
# We require 2.4.0 for the java libs
if test "${build_java_libs}" = "yes"; then
  PROTOBUF_SUPPORT([2.4.0])
else
  PROTOBUF_SUPPORT([2.3.0])
fi

# LIBRARY: uuid
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

if test "${use_ossp_uuid}" = "yes"; then
  AC_DEFINE(USE_OSSP_UUID, 1, [defined if we should use the ossp uuid lib])
fi

# LIBRARY: dmx4linux
have_dmx4linux=""
AC_CHECK_LIB(dmx4linux, DMXdev, [have_dmx4linux="yes"])
AC_CHECK_HEADER(
   [dmx/dmxioctl.h],
   [],
   [have_dmx4linux=""])

AM_CONDITIONAL(HAVE_DMX4LINUX, test "${have_dmx4linux}" = "yes")

if test "${have_dmx4linux}" = "yes"; then
 PLUGINS="${PLUGINS} dmx4linux"
 AC_DEFINE(HAVE_DMX4LINUX, 1, [define if dmx4linux is installed])
fi

# LIBRARY: libusb
# look for libusb if the user asked us to
AC_ARG_ENABLE(
  [libusb],
  AS_HELP_STRING([--disable-libusb], [Avoid using libusb even if it exists]))

have_libusb="no"

if test "${enable_libusb}" != "no"; then
  PKG_CHECK_MODULES(libusb, [libusb-1.0 >= 1.0.2], [have_libusb="yes"], [true])
fi
AM_CONDITIONAL(HAVE_LIBUSB, test "${have_libusb}" = "yes")

if test "${have_libusb}" = "yes"; then
 PLUGINS="${PLUGINS} usbdmx"
 AC_DEFINE(HAVE_LIBUSB, 1, [define if libusb is installed])
fi

# LIBRARY: libftdi
# look for libftdi (this requires libusb-0.1.4+ don't know if I should also check for it, since the libusb check above is not for 0.1.4 but for libusb-1
AC_ARG_ENABLE(
  [libftdi],
  AS_HELP_STRING([--disable-libftdi], [Avoid using libftdi even if it exists]))


have_libftdi="no"

if test "${enable_libftdi}" != "no"; then
  PKG_CHECK_MODULES(libftdi, [libftdi >= 0.18], [have_libftdi="yes"], [true])
fi

AM_CONDITIONAL(HAVE_LIBFTDI, test "${have_libftdi}" = "yes")

if test "${have_libftdi}" = "yes"; then
  PLUGINS="${PLUGINS} ftdidmx"
  AC_DEFINE(HAVE_LIBFTDI, 1, [define if libftdi is installed])
fi

# LIBRARY: libftd2xx (for now this is intentionally disabled, TODO)
have_libftd2xx="no"
AM_CONDITIONAL(HAVE_LIBFTD2XX, test "${have_libftd2xx}" = "yes")

# LIBRARY: openslp
# This is used for E1.33 support
# openslp doesn't come use pkg-config, so the users may need to pass CPPFLAGS
# & LDFLAGS so we can find it.
AC_ARG_ENABLE(
  slp,
  AC_HELP_STRING([--disable-slp], [Disable the SLP tools]),)

openslp_LIBS=""
have_slp=no
if test "${enable_slp}" != "no"; then
  AC_CHECK_LIB([slp], [SLPOpen], [have_slp="yes"])
fi

AM_CONDITIONAL(HAVE_SLP, test "${have_slp}" = "yes")

if test "${have_slp}" = "yes"; then
  AC_DEFINE(HAVE_LIBSLP, 1, [define if libslp is installed])
  openslp_LIBS="-lslp"
fi
AC_SUBST(openslp_LIBS)

# LIBRARY: liblo (OSC)
# look for liblo if the user asked us to
AC_ARG_ENABLE(
  [osc],
  AS_HELP_STRING([--disable-osc], [Disable the OSC plugin even if liblo exists]))

have_liblo="no"

if test "${enable_osc}" != "no"; then
  PKG_CHECK_MODULES(liblo, [liblo >= 0.26], [have_liblo="yes"], [true])
fi
AM_CONDITIONAL(HAVE_LIBLO, test "${have_liblo}" = "yes")

if test "${have_liblo}" = "yes"; then
 PLUGINS="${PLUGINS} osc"
 AC_DEFINE(HAVE_LIBLO, 1, [define if liblo is installed])
fi

# LIBRARY: libmicrohttpd
# look for microhttpd if the user asked us to
AC_ARG_ENABLE(
  [http],
  AS_HELP_STRING([--disable-http], [Disable the built in HTTP server]))

have_microhttpd="no"

if test "${enable_http}" != "no"; then
  PKG_CHECK_MODULES(libmicrohttpd, [libmicrohttpd], [have_microhttpd="yes"], [true])
fi
AM_CONDITIONAL(HAVE_LIBMICROHTTPD, test "${have_microhttpd}" = "yes")

if test "${have_microhttpd}" = "yes"; then
  AC_DEFINE(HAVE_LIBMICROHTTPD, 1, [define if libmicrohttpd is installed])
fi

# Now build a list of plugin libs
PLUGIN_LIBS=''
OLA_SERVER_LIBS=''
for p in $PLUGINS; do
  LIB_NAME="libola${p}"
  PLUGIN_LIBS="$PLUGIN_LIBS \$(top_builddir)/plugins/${p}/$LIB_NAME.la"
  OLA_SERVER_LIBS="$OLA_SERVER_LIBS -lola${p}"
done

AC_SUBST(PLUGIN_LIBS)
AC_SUBST(OLA_SERVER_LIBS)

# plugin & html data directories
plugindir=$libdir/olad
ola_datadir=$datadir/olad
www_datadir=$ola_datadir/www
pid_datadir=$datadir/ola/pids
AC_SUBST(plugindir)
AC_SUBST(www_datadir)
AC_SUBST(pid_datadir)

OLAD_DEFINES="-DHTTP_DATA_DIR=\\\"$www_datadir\\\" -DPLUGIN_DIR=\\\"$plugindir\\\" -DPID_DATA_DIR=\\\"$pid_datadir\\\" "
AC_SUBST(OLAD_DEFINES)

# check for cppunit
AC_ARG_ENABLE(
  unittests,
  AC_HELP_STRING([--disable-unittests], [Disable all unittests]),)

if test "${enable_unittests}" != "no"; then
AM_PATH_CPPUNIT(
  1.9.6,
  [],
  [AC_MSG_ERROR([Missing cppunit, please install >= 1.9.6])])
fi

AM_CONDITIONAL(BUILD_TESTS, test "${enable_unittests}" != "no")

# gcov
AC_ARG_ENABLE(
  gcov,
  AC_HELP_STRING([--enable-gcov], [turn on code coverage analysis tools]))

if test "${enable_gov}" = "yes"; then
  CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
  CXXFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
fi

# tcmalloc
AC_ARG_ENABLE(tcmalloc, AC_HELP_STRING([--enable-tcmalloc], [Use tcmalloc]))

if test "${enable_tcmalloc}" = "yes"; then
  LDFLAGS="$LDFLAGS -ltcmalloc"
fi

# Decide if we should build the example programs
AC_ARG_ENABLE(
  examples,
  AC_HELP_STRING([--disable-examples],
                 [Disable the OLA example client programs]),)

AM_CONDITIONAL(BUILD_EXAMPLES, test "${enable_examples}" != "no")

# Optionally install the E1.33 library.
AC_ARG_ENABLE(
  e133,
  AC_HELP_STRING([--enable-e133], [Install the E1.33 library]),)
# For now we install the ACN lib if the E1.33 lib is requested
AM_CONDITIONAL(INSTALL_ACN, test "${enable_e133}" == "yes")
AM_CONDITIONAL(INSTALL_E133, test "${enable_e133}" == "yes")

# python wrappers & RDM Responder tests.
AC_ARG_ENABLE(
  python-libs,
  AC_HELP_STRING([--enable-python-libs], [Build the python interface]))

AC_ARG_ENABLE(
  rdm-tests,
  AC_HELP_STRING([--enable-rdm-tests], [Install the RDM responder tests]))

if test "${enable_rdm_tests}" = "yes"; then
  enable_python_libs="yes";
fi

AM_CONDITIONAL(BUILD_PYTHON_LIBS, test "${enable_python_libs}" = "yes")
AM_CONDITIONAL(INSTALL_RDM_TESTS, test "${enable_rdm_tests}" = "yes")

if test "${enable_python_libs}" = "yes"; then
 AM_PATH_PYTHON(2.6)
 AX_PYTHON_MODULE("google.protobuf", "fatal")
fi

# Maybe build the logic sniffer tools
SALEAE_DEVICE

# Enable running as root
AC_ARG_ENABLE(
  root-check,
  AC_HELP_STRING([--disable-root-check],
                 [Disable the check that prevents olad from running as root]),
)

if test "${enable_root_check}" = "no"; then
 AC_DEFINE(OLAD_SKIP_ROOT_CHECK,
           1,
           [Defined if olad is allowed to run as root])
fi

# common.h
AC_OUTPUT_COMMANDS([
  if test -n "$CONFIG_FILES" && test -n "$CONFIG_HEADERS"; then
    # If both these vars are non-empty, then config.status wasn't run by
    # automake rules (which always set one or the other to empty).
    CONFIG_OTHER=${CONFIG_OTHER-ola/common.h}
  fi
  echo $CONFIG_OTHER
  case "$CONFIG_OTHER" in
  *ola/common.h*)
    outfile=ola/common.h
    stampfile=ola/stamp-common
    tmpfile=${outfile}T
    dirname="sed s,^.*/,,g"

    echo creating $outfile
    cat > $tmpfile << _EOF_
/*  -*- Mode: C -*-
 * --------------------------------------------------------------------
 * DO NOT EDIT THIS FILE!  It has been automatically generated
 * from:    configure.in and `echo $outfile|$dirname`.in
 * on host: `(hostname || uname -n) 2>/dev/null | sed 1q`
 * --------------------------------------------------------------------
 */

#ifndef OLA_COMMON_H
#define OLA_COMMON_H 1

_EOF_

    # The ugly but portable cpp stuff comes from here
    infile=$srcdir/ola/`echo $outfile | sed 's,.*/,,g;s,\..*$,,g'`-h.in
    sed '/^##.*$/d' $infile >> $tmpfile

    cat >> $tmpfile << '_EOF_'

#endif /* OLA_COMMON_H */
_EOF_

    if cmp -s $tmpfile $outfile; then
      echo $outfile is unchanged
      rm -f $tmpfile
    else
      mv $tmpfile $outfile
      touch $stampfile
    fi
  esac;
],[
  srcdir=$srcdir
  have_threads=$have_threads
])


# output
AC_OUTPUT( Makefile \
    common/Makefile \
    common/base/Makefile \
    common/dmx/Makefile \
    common/export_map/Makefile \
    common/file/Makefile \
    common/http/Makefile \
    common/io/Makefile \
    common/math/Makefile \
    common/messaging/Makefile \
    common/network/Makefile \
    common/protocol/Makefile \
    common/rdm/Makefile \
    common/rdm/testdata/Makefile \
    common/rdm/testdata/pids/Makefile \
    common/rpc/Makefile \
    common/testing/Makefile \
    common/thread/Makefile \
    common/timecode/Makefile \
    common/utils/Makefile \
    common/web/Makefile \
    data/Makefile \
    data/rdm/Makefile \
    doxygen/Makefile \
    doxygen/examples/Makefile \
    examples/Makefile \
    debian/Makefile \
    include/Makefile \
    include/ola/Makefile \
    include/ola/acn/Makefile \
    include/ola/base/Makefile \
    include/ola/client/Makefile \
    include/ola/dmx/Makefile \
    include/ola/file/Makefile \
    include/ola/e133/Makefile \
    include/ola/http/Makefile \
    include/ola/io/Makefile \
    include/ola/math/Makefile \
    include/ola/messaging/Makefile \
    include/ola/network/Makefile \
    include/ola/rdm/Makefile \
    include/ola/slp/Makefile \
    include/ola/stl/Makefile \
    include/ola/testing/Makefile \
    include/ola/thread/Makefile \
    include/ola/timecode/Makefile \
    include/ola/util/Makefile \
    include/ola/web/Makefile \
    include/olad/Makefile \
    java/Makefile \
    man/Makefile \
    ola/Makefile \
    python/Makefile \
    python/examples/Makefile \
    python/ola/Makefile \
    python/ola/rpc/Makefile \
    olad/Makefile \
    olad/OlaVersion.h \
    olad/testdata/Makefile \
    olad/www/Makefile \
    plugins/Makefile \
    plugins/artnet/Makefile \
    plugins/artnet/messages/Makefile \
    plugins/artnet/messages/libolaartnetconf.pc \
    plugins/dmx4linux/Makefile \
    plugins/dummy/Makefile \
    plugins/e131/Makefile \
    plugins/e131/e131/Makefile \
    plugins/e131/e131/libolaacn.pc \
    plugins/e131/messages/Makefile \
    plugins/e131/messages/libolae131conf.pc \
    plugins/espnet/Makefile \
    plugins/ftdidmx/Makefile \
    plugins/karate/Makefile \
    plugins/kinet/Makefile \
    plugins/milinst/Makefile \
    plugins/opendmx/Makefile \
    plugins/osc/Makefile \
    plugins/pathport/Makefile \
    plugins/sandnet/Makefile \
    plugins/shownet/Makefile \
    plugins/spi/Makefile \
    plugins/stageprofi/Makefile \
    plugins/usbdmx/Makefile \
    plugins/usbpro/Makefile \
    plugins/usbpro/messages/Makefile \
    plugins/usbpro/messages/libolausbproconf.pc \
    protoc/Makefile \
    slp/Makefile \
    slp/libolaslpclient.pc \
    slp/libolaslpserver.pc \
    tools/Makefile \
    tools/ola_trigger/Makefile \
    tools/e133/Makefile \
    tools/e133/libolae133controller.pc \
    tools/e133/libolae133common.pc \
    tools/e133/libolae133slp.pc \
    tools/logic/Makefile \
    tools/rdm/Makefile \
    tools/rdmpro/Makefile \
    tools/usbpro/Makefile \
    libola.pc \
    libolaserver.pc \
    )
