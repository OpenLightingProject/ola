#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(configure.ac)
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(ola-examples, 0.7.0)

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Check for ncurses
AC_CHECK_LIB([ncurses], [initscr], [have_ncurses="yes"])
AM_CONDITIONAL(HAVE_NCURSES, test "${have_ncurses}" = "yes")

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([endian.h ppc/endian.h arpa/inet.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_HEADER_STDBOOL

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gettimeofday ftime inet_ntoa memset select socket strerror])

PKG_CHECK_MODULES(libola, [libola >= 0.3.0])
PKG_CHECK_MODULES(libolausbproconf, [libolausbproconf >= 0.3.0])
PKG_CHECK_MODULES(libolaartnetconf, [libolaartnetconf >= 0.3.0])

AC_SUBST(libola_CFLAGS)
AC_SUBST(libola_LIBS)

AC_OUTPUT( Makefile \
           src/Makefile \
         )
