## ola.m4 -- macros for OLA
## Copyright (C) 2009 Simon Newton
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# PROTOBUF_SUPPORT(version)
# Check that the protobuf headers are installed and that protoc is the correct
# version
# -----------------------------------------------------------------------------
AC_DEFUN([PROTOBUF_SUPPORT],
[
AC_REQUIRE_CPP()
PKG_CHECK_MODULES(libprotobuf, [protobuf >= $1])

AC_SUBST([libprotobuf_CFLAGS])

AC_ARG_WITH([protoc],
  [AS_HELP_STRING([--with-protoc=COMMAND],
    [use the given protoc command instead of searching $PATH (useful for cross-compiling)])],
  [],[with_protoc=no])

if test "$with_protoc" != "no"; then
  PROTOC=$with_protoc;
  echo "set protoc to $with_protoc"
  AC_SUBST([PROTOC])
else
  AC_PATH_PROG([PROTOC],[protoc])
fi


if test -z "$PROTOC" ; then
  AC_MSG_ERROR([cannot find 'protoc' program]);
elif test -n "$1" ; then
  AC_MSG_CHECKING([protoc version])
  [protoc_version=`$PROTOC --version 2>&1 | grep 'libprotoc' | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`]
  [required=$1]
  [required_major=`echo $required | sed 's/[^0-9].*//'`]
  [required_minor=`echo $required | sed 's/[0-9][0-9]*\.\([0-9][0-9]*\)\.[0-9][0-9]*/\1/'`]
  [required_patch=`echo $required | sed 's/^.*[^0-9]//'`]
  [actual_major=`echo $protoc_version | sed 's/[^0-9].*//'`]
  [actual_minor=`echo $protoc_version | sed 's/[0-9][0-9]*\.\([0-9][0-9]*\)\.[0-9][0-9]*/\1/'`]
  [actual_patch=`echo $protoc_version | sed 's/^.*[^0-9]//'`]

  protoc_version_proper=`expr \
    $actual_major \> $required_major \| \
    $actual_major \= $required_major \& \
    $actual_minor \> $required_minor \| \
    $actual_major \= $required_major \& \
    $actual_minor \= $required_minor \& \
    $actual_patch \>= $required_patch `

  if test "$protoc_version_proper" = "1" ; then
    AC_MSG_RESULT([$protoc_version])
  else
    AC_MSG_ERROR([protoc version too old $protoc_version < $required]);
  fi
fi

AC_ARG_WITH([ola-protoc-plugin],
  [AS_HELP_STRING([--with-ola-protoc-plugin=COMMAND],
    [use the given ola_protoc_plugin instead of building one (useful for cross-compiling)])],
  [],[with_ola_protoc_plugin=no])


if test "$with_ola_protoc_plugin" != "no"; then
  OLA_PROTOC="$PROTOC --plugin=protoc-gen-cppservice=${with_ola_protoc_plugin}";
  echo "set ola_protoc to $OLA_PROTOC"
else
  OLA_PROTOC="$PROTOC --plugin=protoc-gen-cppservice=\$(top_builddir)/protoc/ola_protoc_plugin${EXEEXT}";
  AC_CHECK_HEADER(
      [google/protobuf/compiler/command_line_interface.h],
      [],
      AC_MSG_ERROR([Cannot find the protoc header files]))
  SAVED_LIBS=$LIBS
  LIBS="$LIBS -lprotoc"
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <google/protobuf/compiler/command_line_interface.h>],
      [google::protobuf::compiler::CommandLineInterface cli])],
    [TEST_LIBS="$TEST_LIBS -lprotoc"] [],
    [AC_MSG_ERROR([cannot find libprotoc])])
  LIBS=$SAVED_LIBS
fi
AC_SUBST([OLA_PROTOC])
AM_CONDITIONAL(BUILD_OLA_PROTOC_PLUGIN, test "${with_ola_protoc_plugin}" = "no")
])


# PLUGIN_SUPPORT(plugin_key, conditional, prerequisites_found)
#
# Build the specified plugin if requested and its dependencies are met.
#
# Each plugin specified with PLUGIN_SUPPORT gets its own configure switches
# --enable-X/--disable-X, which translate to the enable_X variable. The three
# acceptable states for this variable are:
#
#  - auto: Build the plugin if its dependencies are met, otherwise
#    don't. This is the default state used when the user does not specify
#    anything regarding this plugin.
#  - yes: Build the plugin and error out if its dependencies are not met.
#  - no: Never build the plugin.
#
# If the input value of enable_X is "auto" and EITHER of the following is true:
#
#  - --disable-all-plugins is specified
#  - the plugin's dependencies are not met
#
# then PLUGIN_SUPPORT will overwrite the value of enable_X to "no". In all
# other cases, the original value is preserved.
#
# This means that after PLUGIN_SUPPORT has "run" for a particular plugin, a value
# of "auto" or "yes" means that the plugin will get built, while a value of "no"
# means that it won't.
AC_DEFUN([PLUGIN_SUPPORT],
[
  plugin_key=$1
  enable_arg="enable_${plugin_key}"

  AC_ARG_ENABLE(
    [$1],
    AS_HELP_STRING([--disable-$1], [Disable the $1 plugin]),
    [],
    [eval $enable_arg=auto])

  eval enable_plugin=\$$enable_arg;

  # Input validation.
  if test "${enable_plugin}" != "yes" -a \
          "${enable_plugin}" != "no" -a \
          "${enable_plugin}" != "auto"; then
    AC_MSG_ERROR([Invalid value for --enable-${plugin_key}/\
--disable-${plugin_key}. Valid values are yes, no and auto.])
  fi

  # If dependencies are not met...
  if test "$3" = "no"; then
    # ...and the user has explicitly requested this plugin to be enabled,
    # error out.
    if test "${enable_plugin}" = "yes"; then
      AC_MSG_ERROR([Dependencies for plugin ${plugin_key} are not met.])
    fi

    # Otherwise, silently force disable the plugin.
    enable_plugin="no";
  fi

  # If we've disabled all plugins, disable this one unless it has been
  # explicitly enabled.
  if test "${enable_all_plugins}" = "no" -a "${enable_plugin}" = "auto"; then
    enable_plugin="no";
  fi

  if test "${enable_plugin}" != "no"; then
    PLUGINS="${PLUGINS} ${plugin_key}";
    AC_DEFINE_UNQUOTED($2, [1], [define if $1 is to be used])
  fi
  AM_CONDITIONAL($2, test "${enable_plugin}" != "no")
])
