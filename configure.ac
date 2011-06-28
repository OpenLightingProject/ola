#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(configure.ac)
AC_CONFIG_AUX_DIR(config)
AC_SUBST(ac_aux_dir)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(ola, 0.8.11)

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
                inet_ntoa inet_aton select socket strerror getifaddrs])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdint.h stdlib.h string.h \
                  sys/ioctl.h sys/socket.h sys/time.h syslog.h termios.h \
                  endian.h execinfo.h unistd.h linux/if_packet.h])

AC_PROG_LIBTOOL

# Check for ncurses
AC_CHECK_LIB([ncurses], [initscr], [have_ncurses="yes"])
AM_CONDITIONAL(HAVE_NCURSES, test "${have_ncurses}" = "yes")

# windows platform support
AM_CONDITIONAL(USING_WIN32, test "$host_os" = 'mingw32')

# check for hash_map
AC_CXX_STL_HASH

# check for pthread support
AC_CHECK_HEADER([pthread.h],
                [],
                [AC_MSG_ERROR([Missing pthread, please install it])])
LIBS="-lpthread $LIBS"

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


if test -z "${USING_WIN32_FALSE}" && test "${have_msg_no_pipe}" = "no" && \
   test "${have_so_no_pipe}" = "no"; then
 AC_MSG_ERROR([Your system needs either MSG_NOSIGNAL or SO_NOSIGPIPE])
fi

# Check for pkg-config
PKG_PROG_PKG_CONFIG

if test -z "$PKG_CONFIG"; then
  AC_MSG_ERROR([Missing pkg-config, please install it])
fi

# check for protobuf support
PROTOBUF_SUPPORT([2.3.0])

# check for uuid
# uuid handling is quite complex as we can use either the OSSP uuid or the one
# with e2fsprogs. On top of that the header file is different on different
#platforms
use_ossp_uuid="no"
PKG_CHECK_MODULES(
  UUID,
  [uuid],
  [AC_CHECK_HEADERS(
    [ossp/uuid.h],
    [use_ossp_uuid="yes"],
    [AC_CHECK_HEADERS(
      [uuid/uuid.h]
      [],
      [AC_CHECK_LIB([uuid],
                    [uuid_generate],
                    [],
                    [use_ossp_uuid="yes"])])],
    []
  )],
  # the uuid pkg wasn't found, let's try ossp-uuid instead
  [PKG_CHECK_MODULES(
    OSSP_UUID,
    [ossp-uuid],
    [use_ossp_uuid="yes"],
    # if this fails, then our last chance is to check for uuid/uuid.h (present
    # on OS X by default).
    [AC_CHECK_HEADERS(
      [uuid/uuid.h],
      [],
      [AC_MSG_ERROR([Missing the uuid library])]
    )]
  )]
)

if test "${use_ossp_uuid}" = "yes"; then
  AC_DEFINE(USE_OSSP_UUID, 1, [defined if we should use the ossp uuid lib])
fi

# check for plugin dependencies. We also build a list of plugins that we're
# going to compile here so the binary knows what to link against
PLUGINS="artnet dummy espnet e131 opendmx pathport sandnet shownet stageprofi usbpro"

# check for dmx4linux
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

# check for libusb
# look for microhttpd if the user asked us to
AC_ARG_ENABLE(
  [libusb],
  AS_HELP_STRING([--disable-libusb], [Don't use libusb even if it exists]))

have_libusb="no"

if test "${enable_libusb}" != "no"; then
  PKG_CHECK_MODULES(libusb, [libusb-1.0 >= 1.0.2], [have_libusb="yes"], [true])
fi
AM_CONDITIONAL(HAVE_LIBUSB, test "${have_libusb}" = "yes")

if test "${have_libusb}" = "yes"; then
 PLUGINS="${PLUGINS} usbdmx"
 AC_DEFINE(HAVE_LIBUSB, 1, [define if libusb is installed])
fi

# create a list of plugin libs
PLUGIN_LIBS=''
OLA_SERVER_LIBS=''
for p in $PLUGINS; do
  LIB_NAME="libola$p"
  PLUGIN_LIBS="$PLUGIN_LIBS \$(top_builddir)/plugins/$p/$LIB_NAME.la"
  OLA_SERVER_LIBS="$OLA_SERVER_LIBS -l$LIB_NAME"
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

OLAD_DEFINES="-DHTTP_DATA_DIR=\\\"$www_datadir\\\" -DPLUGIN_DIR=\\\"$plugindir\\\" "
AC_SUBST(OLAD_DEFINES)

# check for cppunit
AM_PATH_CPPUNIT(
  1.9.6,
  [],
  [AC_MSG_ERROR([Missing cppunit, please install >= 1.9.6])])

# gcov
AC_ARG_ENABLE(gcov,
              AC_HELP_STRING([--enable-gcov],
                             [turn on code coverage analysis tools]),
              [enable_gov=$enablevar],
              [enable_gov="no"],
             )

if test "${enable_gov}" != "no"; then
  CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
  CXXFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
  LDFLAGS="$LDFLAGS -fprofile-arcs"
fi

# slp - used for E1.33 support
AC_ARG_ENABLE(
  slp,
  AC_HELP_STRING([--disable-slp], [Disable the SLP tools]),)

SLP_LIBS=""
have_slp=no
if test "${enable_slp}" != "no"; then
  AC_CHECK_LIB([slp], [SLPOpen], [have_slp="yes"])
fi

AM_CONDITIONAL(HAVE_SLP, test "${have_slp}" = "yes")

if test "${have_slp}" = "yes"; then
  AC_DEFINE(HAVE_LIBSLP, 1, [define if libslp is installed])
  SLP_LIBS="-lslp"
fi
AC_SUBST(SLP_LIBS)

# look for microhttpd if the user asked us to
AC_ARG_ENABLE(
  [http],
  AS_HELP_STRING([--disable-http], [Disable the built in HTTP server]))

MICROHTTPD_LIBS=""
have_microhttpd="no"

if test "${enable_http}" != "no"; then
  AC_CHECK_LIB(microhttpd, MHD_start_daemon, [have_microhttpd="yes"])
fi

AM_CONDITIONAL(HAVE_LIBMICROHTTPD, test "${have_microhttpd}" = "yes")

if test "${have_microhttpd}" = "yes"; then
  AC_DEFINE(HAVE_LIBMICROHTTPD, 1, [define if libmicrohttpd is installed])
  MICROHTTPD_LIBS="-lmicrohttpd"
fi
AC_SUBST(MICROHTTPD_LIBS)

# Decide if we should build the example programs
AC_ARG_ENABLE(
  examples,
  AC_HELP_STRING([--disable-examples],
                 [Disable the OLA example client programs]),)

AM_CONDITIONAL(BUILD_EXAMPLES, test "${enable_examples}" != "no")

# python wrappers
AC_ARG_ENABLE(python-libs,
              AC_HELP_STRING([--enable-python-libs],
                             [Build the python interface]),
              [build_python_libs="yes"]
             )
AM_CONDITIONAL(BUILD_PYTHON_LIBS, test "${build_python_libs}" = "yes")

if test "${build_python_libs}" = "yes"; then
 AM_PATH_PYTHON(2.4)
fi

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
AC_OUTPUT( Makefile\
    common/Makefile\
    common/export_map/Makefile\
    common/logging/Makefile\
    common/protocol/Makefile\
    common/rdm/Makefile\
    common/rpc/Makefile\
    common/network/Makefile\
    common/utils/Makefile\
    common/web/Makefile\
    data/Makefile\
    data/rdm/Makefile\
    examples/Makefile \
    debian/Makefile\
    include/Makefile\
    include/ola/Makefile\
    include/ola/network/Makefile\
    include/ola/rdm/Makefile\
    include/ola/web/Makefile\
    include/olad/Makefile\
    ola/Makefile \
    python/Makefile \
    python/rpc/Makefile \
    python/examples/Makefile \
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
    plugins/e131/messages/Makefile \
    plugins/e131/messages/libolae131conf.pc \
    plugins/espnet/Makefile \
    plugins/opendmx/Makefile \
    plugins/pathport/Makefile \
    plugins/sandnet/Makefile \
    plugins/shownet/Makefile \
    plugins/stageprofi/Makefile \
    plugins/usbdmx/Makefile \
    plugins/usbpro/Makefile \
    plugins/usbpro/messages/Makefile \
    plugins/usbpro/messages/libolausbproconf.pc \
    tools/Makefile \
    tools/e133/Makefile \
    tools/rdm/Makefile \
    tools/usbpro/Makefile \
    libola.pc \
    libolaserver.pc \
    )
