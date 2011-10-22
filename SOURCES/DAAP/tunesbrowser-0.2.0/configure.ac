#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([tunesbrowser], 0.2.0, [crazney@craz.net])

AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE
AM_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_LIBTOOL

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

AC_C_CONST
AC_TYPE_PID_T

AC_CHECK_FUNCS([memset strdup])

PKG_CHECK_MODULES(OPENDAAP, [opendaap >= 0.3.0])
PKG_CHECK_MODULES(GLADE, libglade-2.0)

PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.8, [gstreamer_found=0.8], [gstreamer_found=no])
if test "$gstreamer_found" = "no"; then
    PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.6, [gstreamer_found=0.6], [gstreamer_found=no])
fi
if test "$gstreamer_found" = "no"; then
    AC_ERROR([couldn't find gstreamer])
fi

TB_CFLAGS="$OPENDAAP_CFLAGS $GLADE_CFLAGS $GSTREAMER_CFLAGS"
TB_LIBS="$OPENDAAP_LIBS $GLADE_LIBS $GSTREAMER_LIBS"

dnl Gnome support
dnl enable_gnome = "yes"
AC_ARG_ENABLE(gnome,
              AC_HELP_STRING([--enable-gnome],
                             [Enable Gnome support in TunesBrowser]),,
                             enable_gnome=default)
#AM_CONDITIONAL(USE_GNOME, test x"$enable_gnome" = xyes)
if test x"$enable_gnome" = xyes || test x"$enable_gnome" = xdefault; then
    PKG_CHECK_MODULES(GNOME, gnome-vfs-2.0 gnome-vfs-module-2.0,
                      [gnome_found=yes], [gnome_found=no])
    if test x"$gnome_found" = xno && test x"$enable_gnome" = xyes; then
        AC_ERROR([Gnome support requested, but Gnome not found])
    fi
    if test x"$gnome_found" = xyes; then
       AC_DEFINE(HAVE_GNOME, 1, [Define if Gnome support is enabled])
       TB_CFLAGS="$TB_CFLAGS $GNOME_CFLAGS"
       TB_LIBS="$TB_LIBS $GNOME_LIBS"
    fi
fi

AC_SUBST(TB_CFLAGS)
AC_SUBST(TB_LIBS)

AC_OUTPUT([Makefile])
