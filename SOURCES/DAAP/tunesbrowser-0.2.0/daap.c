
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

#include "config.h"

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <errno.h>

#ifdef HAVE_GNOME
#include <libgnomevfs/gnome-vfs.h>
#endif

#include <daap/client.h> /* libopendaap */

#include "lists.h"
#include "tunesbrowser.h"

/************* private structure definitions ************/

/* for the browser, and primitive sorting */
struct albumTAG
{
    char *album;
    album *next;
};

struct artistTAG
{
    char *artist;
    album *albumhead;
    artist *next;
};

struct daap_hostTAG
{
    GStaticRecMutex mutex;
    /* ref implies how many references are being kept to the
     * *connected* host. being in the list doesn't add a reference */
    int ref;
    DAAP_SClientHost *libopendaap_host;
    char *sharename;

    DAAP_ClientHost_Database *databases;
    int nDatabases;

    DAAP_ClientHost_DatabaseItem *songs;
    int nSongs;

    artist *artists;
    artist *selected_artist;
    album *selected_album;

    daap_host *prev, *next;

    /* used to check if this host still exists when we get a hosts
     * changed message */
    int marked;

    /* used to mark if the host is dead, but references
     * still exist */
    int dead;
};

enum stop_type
{
    STOP_NONE = 0,
    STOP_NEWSONG,
    STOP_HOSTDEAD,
    STOP_ERROR
};

typedef struct
{
    enum playsource playsource;
    daap_host *host;
    int song_id;
} song_ref;

/******************** globals ***************************/
static DAAP_SClient *clientInstance = NULL;
static daap_host *clientHosts = NULL;
static daap_host *visibleHost = NULL;
static DAAP_Status prev_libopendaap_status = DAAP_STATUS_idle;

/* music playing globals */
static song_ref playing_song = { PLAYSOURCE_NONE, NULL, -1 };
static enum stop_type stop_type = STOP_NONE;
/*   used to schedule the next song */
  static song_ref next_song = { PLAYSOURCE_NONE, NULL, -1 };
/* pipe used for connection between libopendaap and gstreamer */
static int songpipe[2];

/************* forward declerations *********************/
static void cb_hosts_updated();
static int compare_songitems(const void *pva, const void *pvb);
static void free_artists(daap_host *host);

/************* static functions used internally *********/

static int get_songindex_by_id(daap_host *host, int song_id)
{
    int i;

    for (i = 0; i < host->nSongs; i++)
    {
        if (host->songs[i].id == song_id)
            return i;
    }
    return -1;
}

/* adds a given artist / album if it doesn't exist
 * to the artist and album list of the current host
 */
static void artistalbumview_add(daap_host *host,
                                char *artist_s, char *album_s)
{
    artist *cur_artist = host->artists;
    album *cur_album = NULL;
    if (!artist_s || !album_s) return;
    if (!artist_s[0] || !album_s[0]) return;
    while (cur_artist)
    {
        if (strcasecmp(cur_artist->artist, artist_s) == 0)
            break;
        cur_artist = cur_artist->next;
    }
    if (!cur_artist)
    {
        artist *newartist = malloc(sizeof(artist));

        newartist->artist = malloc(strlen(artist_s) +1);
        strcpy(newartist->artist, artist_s);

        newartist->albumhead = NULL;

        newartist->next = host->artists;
        host->artists = newartist;

        cur_artist = newartist;
    }
    cur_album = cur_artist->albumhead;
    while (cur_album)
    {
        if (strcasecmp(cur_album->album, album_s) == 0)
            break;
        cur_album = cur_album->next;
    }
    if (!cur_album)
    {
        album *newalbum = malloc(sizeof(album));

        newalbum->album = malloc(strlen(album_s) + 1);
        strcpy(newalbum->album, album_s);

        newalbum->next = cur_artist->albumhead;
        cur_artist->albumhead = newalbum;

        cur_album = newalbum;
    }
}

/* gets the current database
 * FIXME: what if it updates in between getting the size and
 *        getting the database? */
static void update_databases(daap_host *host)
{
    int size;
    size = DAAP_ClientHost_GetDatabases(host->libopendaap_host,
                                        NULL, NULL, 0);

    if (host->databases) free(host->databases);
    host->databases = malloc(size);
    DAAP_ClientHost_GetDatabases(host->libopendaap_host,
                                 host->databases,
                                 &(host->nDatabases),
                                 size);
}

/* gets the songs from the first database (FIXME use multi databases?)
 * and sorts them.
 * update_databases must be called first.
 */
static void update_songs(daap_host *host)
{
    int size;
    int db_id;
    int i;

    if (!host->nDatabases) return;
    db_id = host->databases[0].id;

    size = DAAP_ClientHost_GetDatabaseItems(host->libopendaap_host, db_id,
                                            NULL, NULL, 0);

    if (host->songs) free(host->songs);
    host->songs = malloc(size);
    DAAP_ClientHost_GetDatabaseItems(host->libopendaap_host, db_id,
                                     host->songs,
                                     &(host->nSongs),
                                     size);

    /* sort it */
    qsort(host->songs, host->nSongs,
          sizeof(DAAP_ClientHost_DatabaseItem),
          compare_songitems);

    if (host->artists) free_artists(host);
    for (i = 0; i < host->nSongs; i++)
    {
        artistalbumview_add(host, host->songs[i].songartist,
                            host->songs[i].songalbum);
    }
}

/* connects to the new daap host,
 * downloads database, etc
 */
static void initial_connect(daap_host *host)
{
    int ret;

    DAAP_ClientHost_AddRef(host->libopendaap_host);
    ret = DAAP_ClientHost_Connect(host->libopendaap_host);
    if (ret == -1)
    {
        fprintf(stderr, "Failed to connect to host '%s', perhaps it's password protected."
                " (We don't support that yet).\n", host->sharename);
        DAAP_ClientHost_Release(host->libopendaap_host);
        daap_host_release(host);
        return;
    }

    sourcelist_set_source_connect_state(host, 1);

    update_databases(host);

    if (host->nDatabases > 1)
    {
        int i;
        printf("not really sure what to do with multiple databases "
               "(never seen it myself). The following databases will "
               "be invisible\n");
        for (i = 1; i < host->nDatabases; i++)
        {
            printf("invisible database: '%s'\n",
                   host->databases[i].name);
        }
    }

    update_songs(host);

    /* now make sure it tells us when updates have happened */
    DAAP_ClientHost_AsyncWaitUpdate(host->libopendaap_host);
}

static void free_albums(album *alb)
{
    album *cur = alb;
    while (cur)
    {
        album *next = cur->next;
        if (cur->album) free(cur->album);
        free(cur);
        cur = next;
    }
}

static void free_artists(daap_host *host)
{
    artist *cur = host->artists;

    host->selected_artist = NULL;
    host->selected_album = NULL;

    while (cur)
    {
        artist *next = cur->next;
        if (cur->artist) free(cur->artist);
        if (cur->albumhead) free_albums(cur->albumhead);
        free(cur);
        cur = next;
    }

    host->artists = NULL;
}

static void disconnect_host(daap_host *host)
{
    g_static_rec_mutex_lock(&host->mutex);
    if (host->databases) free(host->databases);
    host->databases = NULL;
    host->nDatabases = 0;

    if (host->songs) free(host->songs);
    host->songs = NULL;
    host->nSongs = 0;

    free_artists(host);

    DAAP_ClientHost_AsyncStopUpdate(host->libopendaap_host);
    DAAP_ClientHost_Disconnect(host->libopendaap_host);
    DAAP_ClientHost_Release(host->libopendaap_host);
    sourcelist_set_source_connect_state(host, 0);
    g_static_rec_mutex_unlock(&host->mutex);
}

/* actually remove a host from the list, that is it no longer exists */
static void daap_host_remove(daap_host *host)
{
    g_static_rec_mutex_lock(&host->mutex);
    /* only remove it from the list once */
    if (!host->dead)
    {
        if (host->prev) host->prev->next = host->next;
        else clientHosts = host->next;
        if (host->next) host->next->prev = host->prev;
    }

    /* if it's visible, remove visibility */
    if (visibleHost == host) daap_host_set_visible(NULL);

    /* if it's playing, stop it */
    if (playing_song.host == host)
    {
        stop_type = STOP_HOSTDEAD;
        DAAP_ClientHost_AsyncStop(host->libopendaap_host);
    }

    /* perhaps there are some songs in party shuffle? */
    if (daap_host_is_connected(host)) partyshuffle_removehost(host);

    if (daap_host_is_connected(host))
    {
        /* delaying final deletion until all references are destroyed */
        fprintf(stderr, "delaying free of %s\n", host->sharename);
        host->dead = 1;
        g_static_rec_mutex_unlock(&host->mutex);
        return;
    }
    fprintf(stderr, "freeing %s\n", host->sharename);
    if (host->sharename) free(host->sharename);
    DAAP_ClientHost_Release(host->libopendaap_host);
    g_static_rec_mutex_unlock(&host->mutex);
    g_static_rec_mutex_free(&host->mutex);
    free(host);
}

/*************** Status Callback from libopendaap *******/
static void DAAP_StatusCB(__UNUSED__ DAAP_SClient *client, DAAP_Status status,
                          __UNUSED__ int pos, __UNUSED__ void* context)
{
    switch (status)
    {
        case DAAP_STATUS_hostschanged:
        {
            cb_hosts_updated();
            break;
        }
        case DAAP_STATUS_downloading:
        {
            if (prev_libopendaap_status == DAAP_STATUS_negotiating)
            {
                /* start playing the song */
                audioplayer_playpipe(songpipe[0]);
            }
            break;
        }
        case DAAP_STATUS_error:
        {
            if (prev_libopendaap_status == DAAP_STATUS_negotiating ||
                prev_libopendaap_status == DAAP_STATUS_downloading)
            {
                stop_type = STOP_ERROR;
                audioplayer_stop();
            }
            break;
        }
        case DAAP_STATUS_idle:
        {
            if (prev_libopendaap_status == DAAP_STATUS_downloading)
            {
                /* downloading has finished, close the pipe,
                 * this will cause an EOF to be sent.
                 * FIXME: need to check that all data has been flushed?
                 */
                close(songpipe[1]);
                songpipe[1] = -1;
            }
        }
        default:
            break;
    }
    prev_libopendaap_status = status;
}

/******************* specific callback handlers *********/
static int cb_enum_hosts(__UNUSED__ DAAP_SClient *client,
                         DAAP_SClientHost *host,
                         __UNUSED__ void *ctx)
{
    daap_host *cur, *prev = NULL;
    daap_host *newhost;

    /* check if the host is already on the list */
    for (cur = clientHosts; cur != NULL; cur = cur->next)
    {
        if (cur->libopendaap_host == host)
        {
            cur->marked = 1;
            return 1;
        }
        prev = cur;
    }

    /* if not add it to the end of the list */
    newhost = malloc(sizeof(daap_host));
    memset(newhost, 0, sizeof(daap_host));
    g_static_rec_mutex_init(&newhost->mutex);
    DAAP_ClientHost_AddRef(host);

    newhost->ref = 0;
    newhost->prev = prev;
    newhost->next = NULL;
    newhost->libopendaap_host = host;

    /* ensure we don't delete it */
    newhost->marked = 1;

    if (prev) prev->next = newhost;
    else clientHosts = newhost;

    return 1;
}

static void cb_hosts_updated()
{
    daap_host *cur;

    for (cur = clientHosts; cur != NULL; cur = cur->next)
        cur->marked = 0;

    DAAP_Client_EnumerateHosts(clientInstance, cb_enum_hosts, NULL);

    /* now find hosts that need to be removed */
    cur = clientHosts;
    while (cur)
    {
        daap_host *next = cur->next;
        if (cur->marked == 0)
        {
            daap_host_remove(cur);
        }
        cur = next;
    }

    for (cur = clientHosts; cur != NULL; cur = cur->next)
    {
        char *buf;
        int size;

        if (cur->sharename) continue;

        size = DAAP_ClientHost_GetSharename(cur->libopendaap_host, NULL, 0);

        buf = malloc(size);
        DAAP_ClientHost_GetSharename(cur->libopendaap_host, buf, size);
        cur->sharename = buf;
    }

    schedule_lists_draw(1, 0, 0, 0);
}

/**** daap utility functions used by the rest of tb *****/
void daap_host_addref(daap_host *host)
{
    g_static_rec_mutex_lock(&host->mutex);
    if (!(host->ref++)) initial_connect(host);
    g_static_rec_mutex_unlock(&host->mutex);
}

void daap_host_release(daap_host *host)
{
    g_static_rec_mutex_lock(&host->mutex);
    if (--(host->ref))
    {
        g_static_rec_mutex_unlock(&host->mutex);
        return;
    }
    disconnect_host(host);
    if (host->dead) daap_host_remove(host);
    g_static_rec_mutex_unlock(&host->mutex);
}

daap_host *daap_host_enumerate(daap_host *prev)
{
    if (!prev) return clientHosts;
    return prev->next;
}

char *daap_host_get_sharename(daap_host *host)
{
    return host->sharename;
}

int daap_host_is_connected(daap_host *host)
{
    return host->ref ? 1 : 0;
}

void daap_host_set_visible(daap_host *host)
{
    if (visibleHost) daap_host_release(visibleHost);

    if (host) daap_host_addref(host);

    visibleHost = host;

    schedule_lists_draw(0, 1, 1, 1);
}

daap_host *daap_host_get_visible()
{
    return visibleHost;
}

artist *daap_host_enum_artists(daap_host *host, artist *prev)
{
    if (!prev) return host->artists;
    return prev->next;
}

album *daap_host_enum_album(__UNUSED__ daap_host *host,
                            artist *artist, album *prev)
{
    if (!prev) return artist->albumhead;
    return prev->next;
}

artist *daap_host_get_selected_artist(daap_host *host)
{
    return host->selected_artist;
}

album *daap_host_get_selected_album(daap_host *host)
{
    return host->selected_album;
}

void daap_host_set_selected_artist(daap_host *host, artist *artist)
{
    host->selected_artist = artist;

    /* unselect album if all was selected */
    if (host->selected_album && !artist)
        host->selected_album = NULL;

    /* unslected album if it's not in this artist */
    if (host->selected_album)
    {
        album *cur = artist->albumhead;
        while (cur && cur != host->selected_album)
        {
            cur = cur->next;
        }
        if (!cur) host->selected_album = NULL;
    }

    schedule_lists_draw(0, 0, 1, 1);
}

void daap_host_set_selected_album(daap_host *host, album *album)
{
    host->selected_album = album;
    schedule_lists_draw(0, 0, 0, 1);
}

char *daap_host_get_artistname(__UNUSED__ daap_host *host, artist *artist)
{
    return artist->artist;
}

char *daap_host_get_albumname(__UNUSED__ daap_host *host, album *album)
{
    return album->album;
}

int daap_host_enum_artist_album_songs(daap_host *host,
                                      DAAP_ClientHost_DatabaseItem *song,
                                      int prev_id,
                                      artist *artist, album *album)
{
    int i;

    for (i = prev_id+1; i < host->nSongs; i++)
    {
        DAAP_ClientHost_DatabaseItem *thissong = &(host->songs[i]);
        if (artist && (!thissong->songartist ||
                    strcasecmp(daap_host_get_artistname(host, artist),
                               thissong->songartist) != 0))
            continue;
        if (album && (!thissong->songalbum ||
                    strcasecmp(daap_host_get_albumname(host, album),
                               thissong->songalbum) != 0))
            continue;

        if (song)
            memcpy(song, thissong, sizeof(DAAP_ClientHost_DatabaseItem));
        return i;
    }
    return -1;
}

int daap_host_enum_visible_songs(daap_host *host,
                                 DAAP_ClientHost_DatabaseItem *song,
                                 int prev_id)
{
    return daap_host_enum_artist_album_songs(host, song, prev_id,
                                             host->selected_artist,
                                             host->selected_album);
}

void daap_host_get_song_item(daap_host *host, int song_id,
                             DAAP_ClientHost_DatabaseItem *song_out)
{
    int songindex = get_songindex_by_id(host, song_id);

    if (songindex == -1) return;

    memcpy(song_out, &host->songs[songindex],
           sizeof(DAAP_ClientHost_DatabaseItem));
}

enum playsource daap_get_playsource()
{
    return playing_song.playsource;
}

/******************* daap music playing *****************/

void daap_host_play_song(enum playsource playsource, daap_host *host, int song_id)
{
    int songindex;

    if (playing_song.host)
    {
        /* if a song is being played from any host, stop it */

        /* schedule the next song */
        daap_host_addref(host);
        next_song.playsource = playsource;
        next_song.host = host;
        next_song.song_id = song_id;

        stop_type = STOP_NEWSONG;
        /* this should then call cb_eos in gstreamer, which will
         * do the rest */
        DAAP_ClientHost_AsyncStop(playing_song.host->libopendaap_host);
        return;
    }

    stop_type = STOP_NONE;

    songindex = get_songindex_by_id(host, song_id);
    if (songindex == -1) return;

    if (strcmp(host->songs[songindex].songformat, "m4p") == 0)
    {
        fprintf(stderr, "The song you are trying to play is a protected AAC file.\n"
                        "This file has likely been purchased from the iTunes Music Store\n"
                        "and is not able to be shared through traditional sharing mechanisms.\n"
                        "This song will not be played.\n");
    }

    daap_host_addref(host);
    playing_song.playsource = playsource;
    playing_song.host = host;
    playing_song.song_id = song_id;

    if (pipe(songpipe) == -1) return;

    if (DAAP_ClientHost_AsyncGetAudioFile(host->libopendaap_host,
                                          host->databases[0].id, song_id,
                                          host->songs[songindex].songformat,
                                          songpipe[1]))
        goto fail;

    /* we will call into the audioplayer and start playing on the 
     * daap status callback, once download has started
     */

    set_songstatus(GUISTATUS_PLAYING, host, song_id);

    return;

fail:
    close(songpipe[0]);
    close(songpipe[1]);
}

void daap_host_get_playing_info(daap_host **host_out, int *song_id_out)
{
    *host_out = playing_song.host;
    *song_id_out = playing_song.song_id;
}

void daap_audiocb_finished()
{
    daap_host *prevhost = playing_song.host;
    int previd = playing_song.song_id;
    enum playsource prevsource = playing_song.playsource;
    playing_song.host = NULL;
    playing_song.song_id = -1;

    set_songstatus(GUISTATUS_STOPPED, NULL, -1);
#ifdef HAVE_GNOME
  if (prevhost)
  {
#endif
    daap_host_release(prevhost);

    close(songpipe[0]);
    songpipe[0] = -1;
#ifdef HAVE_GNOME
  }
#endif

    if (stop_type == STOP_NEWSONG)
    {
        daap_host_play_song(next_song.playsource, next_song.host, next_song.song_id);
        daap_host_release(next_song.host); /* play_song will addref */
        next_song.host = NULL;
        next_song.song_id = -1;
    }
    else if (stop_type == STOP_NONE) /* ended normally, try and play the next song */
    {
        switch (prevsource)
        {
        case PLAYSOURCE_HOST:
            {
                DAAP_ClientHost_DatabaseItem *song;
                int songindex = get_songindex_by_id(prevhost, previd);
                song = &(prevhost->songs[songindex]);
                /* if some other artist / album is visible, break */
                if (prevhost->selected_artist &&
                        strcasecmp(prevhost->selected_artist->artist,
                                   song->songartist) != 0)
                    break;
                if (prevhost->selected_album &&
                        strcasecmp(prevhost->selected_album->album,
                                   song->songalbum) != 0)
                    break;

                /* now get the next song */
                /* not if it's the alst song */
                if (songindex+1 >= prevhost->nSongs) break;

                song = &(prevhost->songs[songindex+1]);

                /* if it's not in the visible list, break */
                if (prevhost->selected_artist &&
                        strcasecmp(prevhost->selected_artist->artist,
                                   song->songartist) != 0)
                    break;
                if (prevhost->selected_album &&
                        strcasecmp(prevhost->selected_album->album,
                                   song->songalbum) != 0)
                    break;

                /* ok, lets play it */
                daap_host_play_song(PLAYSOURCE_HOST, prevhost, song->id);
            }
            break;
        case PLAYSOURCE_PARTY:
            partyshuffle_play_next();
            break;
        case PLAYSOURCE_NONE:
            break;
        }
    }
}

/***************** and the gnome vfs player *************/
#ifdef HAVE_GNOME
void filehack_play_song(const char *uri)
{
    if (playing_song.host)
    {
        fprintf(stderr, "FIXME: don't currently support playing next song from file\n");
        return;
    }

    stop_type = STOP_NONE;

    audioplayer_playfile(uri);
    set_songstatus(GUISTATUS_PLAYING, NULL, 0);
}
#endif /* HAVE_GNOME */

/********************* basic routines *******************/
void init_daap(int *argc, char **argv[])
{
    clientInstance = DAAP_Client_Create(DAAP_StatusCB, NULL);
    if (*argc > 1)
    {
        int i = 1;
        while (i < *argc)
        {
            if (strcmp((*argv)[i], "--debugmsg") == 0)
            {
                if (i+1 < *argc)
                {
                    i++;
                    DAAP_Client_SetDebug(clientInstance, (*argv)[i]);
                }
                else
                {
                    fprintf(stderr, "--debugmsg must also specify what to do\n");
                }
            }
            i++;
        }
    }
}

/******* sorting routine used by qsort on songs *********/
static int compare_songitems(const void *pva, const void *pvb)
{
    DAAP_ClientHost_DatabaseItem *a = (DAAP_ClientHost_DatabaseItem*)pva;
    DAAP_ClientHost_DatabaseItem *b = (DAAP_ClientHost_DatabaseItem*)pvb;

    int ret;

    char *artist[2], *album[2], *name[2];
    int track_i[2], disc_i[2];

    /* compare artist */
    artist[0] = a->songartist;
    artist[1] = b->songartist;
    if (!artist[0]) return 1;
    if (!artist[1]) return -1;
    if (!artist[0][0]) return 1;
    if (!artist[1][0]) return -1;
    if (strncasecmp(artist[0], "the ", 4) == 0)
        artist[0] += 4;
    if (strncasecmp(artist[1], "the ", 4) == 0)
        artist[1] += 4;
    ret = strcasecmp(artist[0], artist[1]);
    if (ret != 0) return ret;

    /* compare album */
    album[0] = a->songalbum;
    album[1] = b->songalbum;
    if (!album[0]) return 1;
    if (!album[1]) return -1;
    if (!album[0][0]) return 1;
    if (!album[1][0]) return -1;
    if (strncasecmp(album[0], "the ", 4) == 0)
        album[0] += 4;
    if (strncasecmp(album[1], "the ", 4) == 0)
        album[1] += 4;
    ret = strcasecmp(album[0], album[1]);
    if (ret != 0) return ret;

    /* compare disc */
    disc_i[0] = a->songdiscnumber;
    disc_i[1] = b->songdiscnumber;
    ret = (disc_i[0] < disc_i[1] ? -1 :
            (disc_i[0] > disc_i[1] ? 1 : 0));
    if (ret != 0) return ret;

    /* compare track */
    track_i[0] = a->songtracknumber;
    track_i[1] = b->songtracknumber;
    ret = (track_i[0] < track_i[1] ? -1 :
            (track_i[0] > track_i[1] ? 1 : 0));
    if (ret != 0) return ret;

    name[0] = a->itemname;
    name[1] = b->itemname;
    if (!name[0][0]) return 1;
    if (!name[1][0]) return -1;
    if (strncasecmp(name[0], "the ", 4) == 0)
        name[0] += 4;
    if (strncasecmp(name[1], "the ", 4) == 0)
        name[1] += 4;
    ret = strcasecmp(name[0], name[1]);
    if (ret != 0) return ret;

    return 0;
}


