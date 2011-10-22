uidir = $(datadir)
bin_PROGRAMS = tunesbrowser
tunesbrowser_SOURCES = albumlist.c artistlist.c audioplayer.c \
                       daap.c main.c misc_ui.c songlist.c sourcelist.c \
                       partyshuffle.c utilities.c \
                       lists.h tunesbrowser.h
tunesbrowser_LDADD = $(TB_LIBS)
man_MANS = tunesbrowser.1

INCLUDES=-I$(srcdir)
AM_CFLAGS=$(TB_CFLAGS) -W -Wall -DUIDIR=$(datadir)

ui_DATA = tunesbrowser.glade \
          comp.blue.png comp.red.png \
          sound1.png sound2.png

EXTRA_DIST = \
    $(ui_DATA) \
    $(man_MANS)

