#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([breakexfat], [0.1.0], [starbow.duster@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
               [case "${enableval}" in
                       yes) debug=true ;;
                       no)  debug=false ;;
                       *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
               esac],
               [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
