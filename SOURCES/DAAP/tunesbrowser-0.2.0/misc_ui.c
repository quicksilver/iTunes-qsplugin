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


#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "lists.h"
#include "tunesbrowser.h"

static GtkLabel *playstatus;
static GtkLabel *play_str;

static GtkLabel *playpause_label;
static GtkImage *playpause_image;

enum songstatus currentStatus = GUISTATUS_STOPPED;

static void cb_update_songstatus(enum songstatus status, daap_host *host, int songid);
static void cb_update_songpos(int seconds);

typedef struct
{
    enum songstatus s;
    daap_host *host;
    int songid;
} guistatus;

/*********** async command callbacks ***********/

static void async_cmd_cb_sourcelist_draw(__UNUSED__ void *cb_data)
{
    sourcelist_redraw();
}

static void async_cmd_cb_artistlist_draw(__UNUSED__ void *cb_data)
{
    artistlist_redraw();
}

static void async_cmd_cb_albumlist_draw(__UNUSED__ void *cb_data)
{
    albumlist_redraw();
}

static void async_cmd_cb_songlist_draw(__UNUSED__ void *cb_data)
{
    songlist_redraw();
}

static void async_cmd_cb_set_songpos(void *cb_data)
{
    int seconds = (int)cb_data;
    cb_update_songpos(seconds);
}

static void async_cmd_cb_set_status(void *cb_data)
{
    guistatus *status = (guistatus*)cb_data;
    cb_update_songstatus(status->s, status->host,
                         status->songid);

    free(status);
}

/****************** insert message ************/
void set_songstatus(enum songstatus status, daap_host *host, int songid)
{
    guistatus *gstatus = malloc(sizeof(guistatus));
    gstatus->s = status;
    gstatus->host = host;
    gstatus->songid = songid;
    async_cmd_queue_add(async_cmd_cb_set_status, (void*)gstatus);
}

void set_songpos(int seconds)
{
    async_cmd_queue_add(async_cmd_cb_set_songpos, (void*)seconds);
}

void schedule_lists_draw(int source, int artist, int album, int song)
{
    if (source) async_cmd_queue_add(async_cmd_cb_sourcelist_draw, NULL);
    if (artist) async_cmd_queue_add(async_cmd_cb_artistlist_draw, NULL);
    if (album) async_cmd_queue_add(async_cmd_cb_albumlist_draw, NULL);
    if (song) async_cmd_queue_add(async_cmd_cb_songlist_draw, NULL);
}


/****************** async callback handlers *************/

static void cb_update_songstatus(enum songstatus status, daap_host *host, int songid)
{
    currentStatus = status;

    switch(status)
    {
        case GUISTATUS_STOPPED:
            gtk_label_set_text(play_str, "Not Playing");
            gtk_label_set_text(playstatus, "-");

            songlist_set_playing(NULL, 0);
            partyshuffle_set_playing(NULL, 0);
            /* fall through */

        case GUISTATUS_PAUSED:
            gtk_label_set_text(playpause_label, "Play");
            gtk_image_set_from_stock(playpause_image, "gtk-yes", 4);

            if (status == GUISTATUS_STOPPED)
                break;

            /* fall through */
        case GUISTATUS_PLAYING:
            {
                DAAP_ClientHost_DatabaseItem song;
                char *str;

                if (host)
                {
                    daap_host_get_song_item(host, songid, &song);
                    if (song.songartist)
                    {
                        str = malloc(strlen(song.songartist) + strlen(" - ") +
                                     strlen(song.itemname) + 1);
                        strcpy(str, song.songartist);
                        strcat(str, " - ");
                    }
                    else
                    {
                        str = malloc(strlen(song.itemname) + 1);
                        str[0] = 0;
                    }
                    strcat(str, song.itemname);
                }
                else
                {
                    str = malloc(strlen("File Source")+1);
                    strcpy(str, "File Source");
                }

                gtk_label_set_text(play_str, str);

                free(str);

                switch (daap_get_playsource())
                {
                case PLAYSOURCE_HOST:
                    partyshuffle_set_playing(NULL, 0);
                    songlist_set_playing(host, songid);
                    break;
                case PLAYSOURCE_PARTY:
                    songlist_set_playing(NULL, 0);
                    partyshuffle_set_playing(host, songid);
                    break;
                default:
                    break;
                }
            }
            if (status == GUISTATUS_PAUSED)
                break;
            gtk_label_set_text(playpause_label, "Pause");
            gtk_image_set_from_stock(playpause_image, "gtk-no", 4);

            break;
    }
}

static void cb_update_songpos(int seconds)
{
    if (currentStatus == GUISTATUS_STOPPED)
    {
        return;
    }

    if (seconds >= 0)
    {
        int minutes = seconds / 60;
        seconds -= minutes * 60;
        char bufstr[11] = {0};

        snprintf(bufstr, 10, "%i:%02i",
                 minutes, seconds);

        gtk_label_set_text(playstatus, bufstr);
    }
    else
    {
        gtk_label_set_text(playstatus, "-");
    }
}

/********** standard callbacks *******/
void on_playpause_clicked(__UNUSED__ GtkButton *playpause, __UNUSED__ gpointer user_data)
{
    daap_host *host = daap_host_get_visible();
    if (!host) return;

    if (currentStatus == GUISTATUS_PLAYING) /* current play -> pause */
    {
        /* FIXME shouldn't really call audioplayer from here */
        set_songstatus(GUISTATUS_PAUSED, host, songlist_get_selected_id());
        audioplayer_pause();
    }
    else if (currentStatus == GUISTATUS_STOPPED) /* current stopped -> play */
    {

        if (partyshuffle_is_active())
        {
            partyshuffle_play_selected();
        }
        else
        {
            /* FIXME songlist should have a play_selected function */
            int id = songlist_get_selected_id();
            if (id == -1) return;
            daap_host_play_song(PLAYSOURCE_HOST, host, id);
        }
    }
    else if (currentStatus == GUISTATUS_PAUSED) /* current paused -> resume */
    {
        /* FIXME shouldn't call audioplayer from here */
        set_songstatus(GUISTATUS_PLAYING, host, songlist_get_selected_id());
        audioplayer_resume();
    }
}

/************** init **************/

void misc_ui_init(GladeXML *xml)
{
    GtkButton *playpause;

    playstatus = GTK_LABEL(glade_xml_get_widget(xml, "playstatus"));
    play_str = GTK_LABEL(glade_xml_get_widget(xml, "play_str"));

    playpause_label = GTK_LABEL(glade_xml_get_widget(xml, "playpause_label"));
    playpause_image = GTK_IMAGE(glade_xml_get_widget(xml, "playpause_image"));

    playpause = GTK_BUTTON(glade_xml_get_widget(xml, "playpause"));
    g_signal_connect(G_OBJECT(playpause), "clicked",
                     G_CALLBACK(on_playpause_clicked),
                     NULL);
}

void misc_ui_finalize()
{
}

