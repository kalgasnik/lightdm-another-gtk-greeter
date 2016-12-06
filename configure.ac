#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])

define(_PACKAGE_VERSION, 1.0.6.3-4)

AC_INIT(lightdm-another-gtk-greeter, _PACKAGE_VERSION, pan.pav.7c5@gmail.com)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
LT_INIT

AC_DEFINE_UNQUOTED(PACKAGE_VERSION, "_PACKAGE_VERSION")

GOBJECT_INTROSPECTION_CHECK(0.9.5)
GNOME_COMPILE_WARNINGS(minimum)

# #############################################################################
# Checks
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL

# #############################################################################
# Options

AC_ARG_ENABLE(ido-calendar,
[  --enable-ido-calendar    Show calendar on click on time label (disabled by default). Using calendar from Ido libary.],
    [if test x$enableval = xyes; then
        AC_DEFINE(CLOCK_USE_IDO_CALENDAR, [], [Show calendar on click on time label. Using Ido library.])
    fi]
)

# #############################################################################
# Dependencies

PKG_CHECK_MODULES(GREETER, [
    gtk+-3.0
    gthread-2.0
    x11
    gmodule-export-2.0
    libxklavier
])

PKG_CHECK_MODULES([LIGHTDMGOBJECT], [liblightdm-gobject-1 >= 1.19.2],
  [AC_DEFINE([HAVE_LIBLIGHTDMGOBJECT_1_19_2], [1], [Building with liblightdmgobject 1.19.2])],
  [PKG_CHECK_MODULES([LIGHTDMGOBJECT], [liblightdm-gobject-1 >= 1.3.5])]
)

AS_IF([test "x$enable_ido_calendar" = "xyes"], [
    PKG_CHECK_MODULES(IDO, [libido3-0.1])
])

# #############################################################################
# Checks for header files
AC_PATH_X
AC_CHECK_HEADERS([locale.h stdlib.h])

# #############################################################################
# Checks for typedefs, structures, and compiler characteristics

# #############################################################################
# Checks for library functions
AC_CHECK_FUNCS([setlocale])

# #############################################################################
# Internationalization

IT_PROG_INTLTOOL(0.35.0)
AC_SUBST(GETTEXT_PACKAGE, lightdm-another-gtk-greeter)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", Gettext package)

# #############################################################################

AC_CONFIG_FILES([
    Makefile
    data/Makefile
    data/themes/Makefile
    data/themes/default/Makefile
    data/themes/default/gtk-themes-fixes/Makefile
    data/themes/sample.css.dark/Makefile
    data/themes/gtk-greeter-150/Makefile
    src/Makefile
    po/Makefile.in])
AC_OUTPUT
