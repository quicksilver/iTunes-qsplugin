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



#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "lists.h"
#include "tunesbrowser.h"
#include <string.h>
#include <stdlib.h>

static GtkListStore *songlist;
static GtkTreeView *songtreeview;

GdkPixbuf *now_playing_image = NULL;

static daap_host *playing_song_host = NULL;
static int playing_song_id = 0;

enum
{
    COL_ID,
    COL_ITEMNAME,
    COL_SONGARTIST,
    COL_SONGALBUM,
    COL_SONGTIME,
    COL_SONGTRACK,
    COL_SONGDISC,
    COL_SONGYEAR,
    COL_IMAGE,
    N_COLUMNS
};

static void on_song_row_activated(GtkTreeView *treeview,
                                  GtkTreePath *arg1,
                                  GtkTreeViewColumn *arg2,
                                  gpointer user_data);
static void on_song_drag_begin(GtkWidget *widget,
                               GdkDragContext *drag_context,
                               gpointer user_data);
static void on_song_drag_data_get(GtkWidget *widget,
                                  GdkDragContext *drag_context,
                                  GtkSelectionData *data,
                                  guint info,
                                  guint time,
                                  gpointer user_data);
static void on_song_drag_end(GtkWidget *widget,
                             GdkDragContext *drag_context,
                             gpointer user_data);

static void songlist_set_icon_for_id(int id, int playing);

    /* don't use this function cause it is slow..
     * sorting in daap.c using qsort, instead.
     * (same algorithm.. I suppose gtk is just slow
     */
/* #define SLOW_SORT */
/* sorts */
gint songlist_default_sort(GtkTreeModel *model,
                           GtkTreeIter *a,
                           GtkTreeIter *b,
                           gpointer user_data);

#define XSTR(s) STR(s)
#define STR(s) #s
static void load_images()
{
    now_playing_image = gdk_pixbuf_new_from_file(XSTR(UIDIR) "/sound1.png", NULL);
}
#undef XSTR
#undef STR

int songlist_init(GladeXML *xml)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *select;

    load_images();

    songlist = gtk_list_store_new (N_COLUMNS,
                                    G_TYPE_INT, /* id */
                                    G_TYPE_STRING, /* itemname */
                                    G_TYPE_STRING, /* artist */
                                    G_TYPE_STRING, /* album */
                                    G_TYPE_STRING, /* time */
                                    G_TYPE_STRING, /* track */
                                    G_TYPE_STRING, /* disc */
                                    G_TYPE_STRING, /* year */
                                    GDK_TYPE_PIXBUF /* image */
                                    );
    songtreeview = GTK_TREE_VIEW(glade_xml_get_widget(xml, "songs"));

    gtk_tree_view_set_model(songtreeview, GTK_TREE_MODEL(songlist));

    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("",
                                                      renderer,
                                                      "pixbuf", COL_IMAGE,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_append_column (songtreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Song Name",
                                                      renderer,
                                                      "text", COL_ITEMNAME,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (songtreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Time",
                                                      renderer,
                                                      "text", COL_SONGTIME,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (songtreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Artist",
                                                      renderer,
                                                      "text", COL_SONGARTIST,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (songtreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Track #",
                                                      renderer,
                                                      "text", COL_SONGTRACK,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (songtreeview, column);

#if 0
    /* no one really wants to see disc # */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Disc #",
                                                      renderer,
                                                      "text", COL_SONGDISC,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (songtreeview, column);
#endif

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Album",
                                                      renderer,
                                                      "text", COL_SONGALBUM,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (songtreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Year",
                                                      renderer,
                                                      "text", COL_SONGYEAR,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (songtreeview, column);

#ifdef SLOW_SORT
    gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(songlist),
                                            songlist_default_sort,
                                            NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(songlist),
                                         GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                         GTK_SORT_ASCENDING);
#endif

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(songtreeview));
    /*gtk_tree_selection_set_mode(select, GTK_SELECTION_MULTIPLE);*/
    /* geh, I tried to enable this to get multiple dnd going, but
     * that doesn't seem to be supported by this shitty model anyhow ;\
     */
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

    /* set it up as a drag source */
    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(songtreeview),
                                           GDK_BUTTON1_MASK,
                                           songlist_src_targets,
                                           songlist_src_n_targets,
                                           GDK_ACTION_LINK);
    g_signal_connect(G_OBJECT(songtreeview), "drag-begin",
                     G_CALLBACK(on_song_drag_begin),
                     NULL);
    g_signal_connect(G_OBJECT(songtreeview), "drag-data-get",
                     G_CALLBACK(on_song_drag_data_get),
                     NULL);
    g_signal_connect(G_OBJECT(songtreeview), "drag-end",
                     G_CALLBACK(on_song_drag_end),
                     NULL);

    g_signal_connect(G_OBJECT(songtreeview), "row-activated",
                     G_CALLBACK(on_song_row_activated),
                     NULL);

    return 1;
}

static void songlist_append(int id, char *name, char *artist,
                            char *album, char *time, char *track,
                            char *disc, char *year)
{
    GtkTreeIter iter;

    gtk_list_store_append(songlist, &iter);

    gtk_list_store_set(songlist, &iter,
                       COL_ID, id,
                       COL_ITEMNAME, name,
                       COL_SONGARTIST, artist,
                       COL_SONGALBUM, album,
                       COL_SONGTIME, time,
                       COL_SONGTRACK, track,
                       COL_SONGDISC, disc,
                       COL_SONGYEAR, year,
                       COL_IMAGE, NULL,
                       -1);

}

void songlist_redraw()
{
    daap_host *host = daap_host_get_visible();
    int enum_id = -1;
    DAAP_ClientHost_DatabaseItem song;

    gtk_list_store_clear(songlist);

    if (!host) return;

    while ((enum_id = daap_host_enum_visible_songs(host, &song, enum_id))
            != -1)
    {
        char time[11] = {0};
        char track[11] = {0};
        char year[11] = {0};
        char disc[11] = {0};
        int minutes, seconds;

        seconds = song.songtime / 1000;
        minutes = seconds / 60;
        seconds -= minutes * 60;

        snprintf(time, 10, "%i:%02i", minutes, seconds);

        if (song.songtracknumber)
            snprintf(track, 10, "%i", song.songtracknumber);
        if (song.songdiscnumber)
            snprintf(disc, 10, "%i", song.songdiscnumber);
        if (song.songyear)
            snprintf(year, 10, "%i", song.songyear);

        songlist_append(song.id, song.itemname, song.songartist,
                        song.songalbum, time, track, disc, year);
    }

    /* update playing icon as necessary */
    if (playing_song_host == host)
        songlist_set_icon_for_id(playing_song_id, 1);
}

int songlist_get_selected_id()
{
    GtkTreeSelection *selection =
        gtk_tree_view_get_selection(songtreeview);
    GtkTreeIter iter;
    GtkTreeModel *model;

#if 0
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gint id;
        gtk_tree_model_get(model, &iter,
                           COL_ID, &id,
                           -1);

        return id;
    }
#endif
    if (gtk_tree_selection_count_selected_rows(selection) == 1) /* fail on <> 1 */
    {
        GList *selected_rows;
        selected_rows = gtk_tree_selection_get_selected_rows(selection, &model);
        if (!selected_rows) return -1;

        if (gtk_tree_model_get_iter(model, &iter, selected_rows->data))
        {
            gint id;
            gtk_tree_model_get(model, &iter,
                               COL_ID, &id,
                               -1);
            return id;
        }
    }
    return -1;
}

/* doesn't clear */
static void songlist_set_icon_for_id(int id, int playing)
{
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(songlist),
                                      &iter))
    {
        do
        {
            int iter_id = 0;
            gtk_tree_model_get(GTK_TREE_MODEL(songlist), &iter,
                               COL_ID, &iter_id,
                               -1);
            if (id == iter_id)
            {
                gtk_list_store_set(songlist, &iter,
                                   COL_IMAGE, playing ?
                                              now_playing_image : 0,
                                   -1);
            }
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(songlist),
                                          &iter));
    }
}

void songlist_set_playing(daap_host *host, int id)
{
    int prev_id = playing_song_id;
    daap_host *prev_host = playing_song_host;
    daap_host *visible_host = daap_host_get_visible();

    playing_song_id = id;
    playing_song_host = host;

    if (!visible_host) return; /* no host visible, no list.. */

    /* if prev playing song is visible, remove icon */
    if (prev_id && prev_host == visible_host)
        songlist_set_icon_for_id(prev_id, 0);

    /* if the new song is visible, add the icon */
    if (id && host == visible_host)
        songlist_set_icon_for_id(id, 1);
}

/* callbacks */
static void on_song_row_activated(__UNUSED__ GtkTreeView *treeview,
                                  __UNUSED__ GtkTreePath *arg1,
                                  __UNUSED__ GtkTreeViewColumn *arg2,
                                  __UNUSED__ gpointer user_data)
{
    daap_host *host = daap_host_get_visible();
    int id = songlist_get_selected_id();

    if (!host) return;
    if (id == -1) return;

    daap_host_play_song(PLAYSOURCE_HOST, host, id);
}

/* addref on the host */
static void on_song_drag_begin(__UNUSED__ GtkWidget *widget,
                               __UNUSED__ GdkDragContext *drag_context,
                               __UNUSED__ gpointer user_data)
{
    daap_host *host = daap_host_get_visible();
    daap_host_addref(host);
}

/* release the host */
static void on_song_drag_end(__UNUSED__ GtkWidget *widget,
                             __UNUSED__ GdkDragContext *drag_context,
                             __UNUSED__ gpointer user_data)
{
    daap_host *host = daap_host_get_visible();
    daap_host_release(host);
}

static void on_song_drag_data_get(__UNUSED__ GtkWidget *widget,
                                  GdkDragContext *drag_context,
                                  GtkSelectionData *data,
                                  __UNUSED__ guint info,
                                  __UNUSED__ guint time,
                                  __UNUSED__ gpointer user_data)
{
    if (data->target == daap_song_atom)
    {
        GtkTreeRowReference *reference;
        GtkTreePath *path;
        GtkTreeIter iter;
        selection_song_reference song;

        reference = g_object_get_data(G_OBJECT(drag_context),
                                      "gtk-tree-view-source-row");
        if (!reference) return;
        path = gtk_tree_row_reference_get_path(reference);
        if (!path) return;

        gtk_tree_model_get_iter(GTK_TREE_MODEL(songlist), &iter, path);

        gtk_tree_model_get(GTK_TREE_MODEL(songlist), &iter,
                           COL_ID, &(song.id),
                           -1);

        song.host = daap_host_get_visible();
        gtk_selection_data_set(data, data->target, 8, (void*)&song, sizeof(song));

        gtk_tree_path_free(path);
    }
}


/* sort funcs */
gint songlist_default_sort(__UNUSED__ GtkTreeModel *model,
                           __UNUSED__ GtkTreeIter *a,
                           __UNUSED__ GtkTreeIter *b,
                           __UNUSED__ gpointer user_data)
{
#ifndef SLOW_SORT
    return 0;
#else
    gchar *artist[2];
    gchar *artist_s[2];
    gchar *album[2];
    gchar *album_s[2];
    gchar *track[2];
    gchar *disc[2];
    gchar *name[2];
    gchar *name_s[2];

    int track_i[2];
    int disc_i[2];

    int ret = 0;

    gtk_tree_model_get(model, a,
                       COL_SONGARTIST, &artist[0],
                       COL_SONGALBUM, &album[0],
                       COL_SONGTRACK, &track[0],
                       COL_SONGDISC, &disc[0],
                       COL_ITEMNAME, &name[0],
                       -1);

    gtk_tree_model_get(model, b,
                       COL_SONGARTIST, &artist[1],
                       COL_SONGALBUM, &album[1],
                       COL_SONGTRACK, &track[1],
                       COL_SONGDISC, &disc[1],
                       COL_ITEMNAME, &name[1],
                       -1);

    artist_s[0] = artist[0];
    artist_s[1] = artist[1];
    if (!artist_s[0][0]) { ret = 1; goto end; }
    if (!artist_s[1][0]) { ret = -1; goto end; }
    if (strncasecmp(artist_s[0], "the ", 4) == 0)
        artist_s[0] += 4;
    if (strncasecmp(artist_s[1], "the ", 4) == 0)
        artist_s[1] += 4;
    ret = strcasecmp(artist_s[0], artist_s[1]);
    if (ret != 0) goto end;

    album_s[0] = album[0];
    album_s[1] = album[1];
    if (!album_s[0][0]) { ret = 1; goto end; }
    if (!album_s[1][0]) { ret = -1; goto end; }
    if (strncasecmp(album_s[0], "the ", 4) == 0)
        album_s[0] += 4;
    if (strncasecmp(album_s[1], "the ", 4) == 0)
        album_s[1] += 4;
    ret = strcasecmp(album_s[0], album_s[1]);
    if (ret != 0) goto end;

    disc_i[0] = atoi(disc[0]);
    disc_i[1] = atoi(disc[1]);
    ret = (disc_i[0] < disc_i[1] ? -1 :
            (disc_i[0] > disc_i[1] ? 1 : 0));
    if (ret != 0) goto end;

    track_i[0] = atoi(track[0]);
    track_i[1] = atoi(track[1]);
    ret = (track_i[0] < track_i[1] ? -1 :
            (track_i[0] > track_i[1] ? 1 : 0));
    if (ret != 0) goto end;

    name_s[0] = name[0];
    name_s[1] = name[1];
    if (!name_s[0][0]) { ret = 1; goto end; }
    if (!name_s[1][0]) { ret = -1; goto end; }
    if (strncasecmp(name_s[0], "the ", 4) == 0)
        name_s[0] += 4;
    if (strncasecmp(name_s[1], "the ", 4) == 0)
        name_s[1] += 4;
    ret = strcasecmp(name_s[0], name_s[1]);
    if (ret != 0) goto end;

end:
    g_free(artist[0]);
    g_free(album[0]);
    g_free(track[0]);
    g_free(disc[0]);
    g_free(name[0]);
    g_free(artist[1]);
    g_free(album[1]);
    g_free(track[1]);
    g_free(disc[1]);
    g_free(name[1]);

    return ret;
#endif
}

