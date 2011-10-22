/* Copyright (c) 2004 David Hammerton - All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#ifndef __TUNESBROWSER_H
#define __TUNESBROWSER_H
#include "config.h"

#include <daap/client.h> /* libopendaap */

#define __UNUSED__ __attribute__ ((unused))

/* daap */
typedef struct daap_hostTAG daap_host;
typedef struct artistTAG artist;
typedef struct albumTAG album;

enum playsource
{
    PLAYSOURCE_NONE = 0,
    PLAYSOURCE_HOST,
    PLAYSOURCE_PARTY
};

void init_daap(int *argc, char **argv[]);
void       daap_host_addref(daap_host *host);
void       daap_host_release(daap_host *host);
daap_host *daap_host_enumerate(daap_host *prev);
char      *daap_host_get_sharename(daap_host *host);
int        daap_host_is_connected(daap_host *host);
void       daap_host_set_visible(daap_host *host);
daap_host *daap_host_get_visible();

artist    *daap_host_enum_artists(daap_host *host, artist *prev);
album     *daap_host_enum_album(daap_host *host, artist *artist, album *prev);
artist    *daap_host_get_selected_artist(daap_host *host);
album     *daap_host_get_selected_album(daap_host *album);
void       daap_host_set_selected_artist(daap_host *host, artist *artist);
void       daap_host_set_selected_album(daap_host *host, album *album);
char      *daap_host_get_artistname(daap_host *host, artist *artist);
char      *daap_host_get_albumname(daap_host *host, album *album);

/* start enumeration with -1, ends with -1 */
int        daap_host_enum_artist_album_songs(daap_host *host,
                                             DAAP_ClientHost_DatabaseItem *song,
                                             int prev_id,
                                             artist *artist, album *album);
int        daap_host_enum_visible_songs(daap_host *host,
                                        DAAP_ClientHost_DatabaseItem *song,
                                        int prev_id);

void       daap_host_get_song_item(daap_host *host, int song_id,
                                   DAAP_ClientHost_DatabaseItem *song_out);

void       daap_host_play_song(enum playsource playsource, daap_host *host, int song_id);
void       daap_host_get_playing_info(daap_host **host_out, int *song_id_out);

enum playsource
           daap_get_playsource();

void       daap_audiocb_finished();

#ifdef HAVE_GNOME
/* file hack */
void       filehack_play_song(const char *uri);
#endif

/* audio player */
void audioplayer_init();
void audioplayer_playpipe(int fd);
void audioplayer_finalize();
void audioplayer_pause();
void audioplayer_resume();
void audioplayer_stop();

#ifdef HAVE_GNOME
void audioplayer_playfile(const char *uri);
#endif


/* misc_ui */
void misc_ui_init(GladeXML *xml);
void misc_ui_finalize();

enum songstatus
{
    GUISTATUS_STOPPED,
    GUISTATUS_PLAYING,
    GUISTATUS_PAUSED
};
void set_songstatus(enum songstatus status, daap_host *host, int songid);
void set_songpos(int seconds);
void schedule_lists_draw(int source, int artist, int album, int song);

/************** utilities ******************/
/* async cmd queue */
typedef void (*async_cmd_cb)(void *cb_data);
void utilities_init();
void utilities_finalize();
void async_cmd_queue_add(async_cmd_cb callback, void *cb_data);

/* targets and selections - for dnd etc */
extern const GtkTargetEntry partytarget_dst_targets[];
extern const int partytarget_dst_n_targets;

extern const GtkTargetEntry songlist_src_targets[];
extern const int songlist_src_n_targets;

extern const GtkTargetEntry artistlist_src_targets[];
extern const int artistlist_src_n_targets;

extern const GtkTargetEntry albumlist_src_targets[];
extern const int albumlist_src_n_targets;

GdkAtom daap_song_atom;
GdkAtom text_uri_list_atom;

/* an array of this is fine. we know the size from the info
 * we are given on callback
 */
typedef struct
{
    daap_host *host;
    int id;
} selection_song_reference;

#endif

