SUBDIRS = authentication debug mDNS
INCLUDES = -I$(srcdir) -I$(srcdir)/debug
AM_CFLAGS = -W -Wall -D_GNU_SOURCE

lib_LTLIBRARIES = libopendaap.la
libopendaap_la_SOURCES = client.c \
                         daap.c daap.h \
                         daap_contentcodes.h daap_readtypes.h \
                         discover.c discover.h \
                         dmap_generics.c dmap_generics.h \
                         global.c \
                         http_client.c http_client.h \
                         portability.h private.h \
                         threadpool.c threadpool.h \
                         thread.h \
                         ioloop.c ioloop.h
libopendaap_la_LDFLAGS = -version-number 0:1:0 -lpthread
libopendaap_la_LIBADD = authentication/libauthentication.la \
                        debug/libdebug.la \
                        mDNS/libmDNS.la
man_MANS = libopendaap.3

apiheadersdir = $(includedir)/daap
nobase_apiheaders_HEADERS = client.h portability.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opendaap.pc

EXTRA_DIST = $(windows_build) \
             $(man_MANS)
windows_build = libopendaap.dsp libopendaap.dsw \
                mDNS/win32/DNSServiceDiscovery.c \
                mDNS/win32/DNSServiceDiscovery.h \
                mDNS/win32/DNSServices.c \
                mDNS/win32/DNSServices.h \
                mDNS/win32/mDNSWin32.c \
                mDNS/win32/mDNSWin32.h

