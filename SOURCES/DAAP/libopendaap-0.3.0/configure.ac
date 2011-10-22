#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([libopendaap], 0.3.0, [crazney@craz.net])

AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE
AM_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_LIBTOOL

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

AC_C_BIGENDIAN
AC_C_CONST
AC_TYPE_PID_T

AC_CHECK_FUNCS([memset strdup])

AC_CHECK_MEMBERS([struct sockaddr.sa_len],
                 [],
                 [ AC_DEFINE(NOT_HAVE_SA_LEN, 1, [Not have sockaddr.sa_len]) ],
                 [
#include <sys/types.h>
#include <sys/socket.h>
])

AC_OUTPUT([Makefile
           debug/Makefile
           authentication/Makefile
           mDNS/Makefile
           opendaap.pc])
