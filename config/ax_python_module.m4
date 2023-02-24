# ===========================================================================
#     http://www.gnu.org/software/autoconf-archive/ax_python_module.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_PYTHON_MODULE(modname[, fatal])
#
# DESCRIPTION
#
#   Checks for Python module.
#
#   If fatal is non-empty then absence of a module will trigger an error.
#
# LICENSE
#
#   Copyright (c) 2008 Andrew Collier <colliera@ukzn.ac.za>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 5

AU_ALIAS([AC_PYTHON_MODULE], [AX_PYTHON_MODULE])
AC_DEFUN([AX_PYTHON_MODULE],[
	$PYTHON -c "import $1" 2>/dev/null
	if test $? -eq 0;
	then
		eval AS_TR_CPP([HAVE_PYMOD_$1])=yes
	else
		AC_MSG_RESULT([no])
		eval AS_TR_CPP([HAVE_PYMOD_$1])=no
		#
		if test -n "$2"
		then
			AC_MSG_ERROR([failed to find required module $1])
		fi
	fi
])
