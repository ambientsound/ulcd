AC_PREREQ([2.69])
AC_INIT([caso], [1.0], [kimtjen@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([ulcd43], [ulcd_new])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h ulcd43.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
