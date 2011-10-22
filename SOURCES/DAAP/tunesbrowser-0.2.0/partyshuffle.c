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

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#ifdef HAVE_GNOME
#include <libgnomevfs/gnome-vfs.h>
#endif /* HAVE_GNOME */

#include "lists.h"
#include "tunesbrowser.h"

static void on_song_row_activated(GtkTreeView *treeview,
                                  GtkTreePath *arg1,
                                  GtkTreeViewColumn *arg2,
                                  gpointer user_data);
static void on_party_button_toggled(GtkToggleButton *button, gpointer data);
static void on_button_drag_data_received(GtkWidget *widget,
                                         GdkDragContext *drag_context,
                                         gint x, gint y,
                                         GtkSelectionData *data,
                                         guint info, guint time,
                                         gpointer user_data);

static GdkPixbuf *party_shuffle_image;
extern GdkPixbuf *now_playing_image; /* storage: songlist.c */

static GtkWidget *standard_songlayout;
static GtkWidget *party_layout;
static GtkToggleButton *partybutton;

static GtkListStore *partylist;
static GtkTreeView *partytreeview;

static daap_host *playing_song_host = NULL;
static int playing_song_id = 0;

enum
{
    COL_HOST,
    COL_HOSTNAME,
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

#define XSTR(s) STR(s)
#define STR(s) #s
static void load_images()
{
    party_shuffle_image = gdk_pixbuf_new_from_file(XSTR(UIDIR) "/sound2.png", NULL);
}
#undef XSTR
#undef STR

static void setup_dnd()
{
    gtk_drag_dest_set(GTK_WIDGET(partybutton), GTK_DEST_DEFAULT_ALL,
                      partytarget_dst_targets, partytarget_dst_n_targets,
                      GDK_ACTION_LINK | GDK_ACTION_COPY);

    gtk_drag_dest_set(party_layout, GTK_DEST_DEFAULT_ALL,
                      partytarget_dst_targets, partytarget_dst_n_targets,
                      GDK_ACTION_LINK | GDK_ACTION_COPY);

    g_signal_connect(G_OBJECT(partybutton), "drag-data-received",
                     G_CALLBACK(on_button_drag_data_received), NULL);

    g_signal_connect(G_OBJECT(party_layout), "drag-data-received",
                     G_CALLBACK(on_button_drag_data_received), NULL);
}

int partyshuffle_init(GladeXML *xml)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkImage *partyimage;

#ifdef HAVE_GNOME
    gnome_vfs_init();
#endif

    load_images();

    /* button */
    partybutton = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "partyshufflebutton"));

    partyimage = GTK_IMAGE(glade_xml_get_widget(xml, "partyimage"));
    gtk_image_set_from_pixbuf(partyimage, party_shuffle_image);

    /* layouts for hide / show */
    standard_songlayout = GTK_WIDGET(glade_xml_get_widget(xml, "standard_songlayout"));
    party_layout = GTK_WIDGET(glade_xml_get_widget(xml, "partyshuffle_layout"));

    /* party song list */
    partylist = gtk_list_store_new (N_COLUMNS,
                                    G_TYPE_POINTER, /* host */
                                    G_TYPE_STRING,
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
    partytreeview = GTK_TREE_VIEW(glade_xml_get_widget(xml, "partysongs"));

    gtk_tree_view_set_model(partytreeview, GTK_TREE_MODEL(partylist));

    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("",
                                                      renderer,
                                                      "pixbuf", COL_IMAGE,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_append_column (partytreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Source",
                                                      renderer,
                                                      "text", COL_HOSTNAME,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (partytreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Song Name",
                                                      renderer,
                                                      "text", COL_ITEMNAME,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (partytreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Time",
                                                      renderer,
                                                      "text", COL_SONGTIME,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (partytreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Artist",
                                                      renderer,
                                                      "text", COL_SONGARTIST,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (partytreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Track #",
                                                      renderer,
                                                      "text", COL_SONGTRACK,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (partytreeview, column);

#if 0
    /* no one really wants to see disc # */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Disc #",
                                                      renderer,
                                                      "text", COL_SONGDISC,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (partytreeview, column);
#endif

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Album",
                                                      renderer,
                                                      "text", COL_SONGALBUM,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (partytreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Year",
                                                      renderer,
                                                      "text", COL_SONGYEAR,
                                                      NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column (partytreeview, column);

    /* dnd */
    setup_dnd();

    /* signals */
    g_signal_connect(G_OBJECT(partybutton), "toggled",
                     G_CALLBACK(on_party_button_toggled), NULL);

    g_signal_connect(G_OBJECT(partytreeview), "row-activated",
                     G_CALLBACK(on_song_row_activated),
                     NULL);

    return 1;
}

void partyshuffle_removehost(daap_host *host)
{
    GtkTreeIter iter;
    gboolean got_next = 0;

    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(partylist),
                                       &iter)) return;
    do
    {
        daap_host *iter_host = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(partylist), &iter,
                           COL_HOST, &iter_host,
                           -1);
        if (iter_host == host)
        {
            GtkTreePath *path = NULL;

            path = gtk_tree_model_get_path(GTK_TREE_MODEL(partylist),
                                           &iter);

            daap_host_release(host);
            gtk_list_store_remove(partylist, &iter);

            /* path now points to the 'next' one */
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(partylist),
                                        &iter, path))
            {
                got_next = 1;
                continue;
            }
            else return;
        }
    } while (got_next ||
             gtk_tree_model_iter_next(GTK_TREE_MODEL(partylist),
                                      &iter));
}

/* shows the party area, hides the standard song area */
static void set_partyarea_visible()
{
    gtk_widget_hide_all(standard_songlayout);
    gtk_widget_show_all(party_layout);
    sourcelist_set_sensitive(0);
};

/* shows the standard song area, hides the party area */
static void set_partyarea_invisible()
{
    gtk_widget_hide_all(party_layout);
    gtk_widget_show_all(standard_songlayout);
    sourcelist_set_sensitive(1);
}

static void party_songlist_append(void *host, char *hostname,
                                  int id, char *name, char *artist,
                                  char *album, char *time, char *track,
                                  char *disc, char *year)
{
    GtkTreeIter iter;

    gtk_list_store_append(partylist, &iter);

    gtk_list_store_set(partylist, &iter,
                       COL_HOST, host,
                       COL_HOSTNAME, hostname,
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

static gboolean find_song_by_host_id(daap_host *host, int id, GtkTreeIter *iter_out)
{
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(partylist),
                                      &iter))
    {
        do
        {
            int iter_id = 0;
            daap_host *iter_host = NULL;
            gtk_tree_model_get(GTK_TREE_MODEL(partylist), &iter,
                               COL_ID, &iter_id,
                               COL_HOST, &iter_host,
                               -1);
            if (id == iter_id && host == iter_host)
            {
                *iter_out = iter;
                return TRUE;
            }
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(partylist),
                                          &iter));
    }
    return FALSE;
}

/* doesn't clear */
static void partyshuffle_set_icon_for_id(daap_host *host, int id, int playing)
{
    GtkTreeIter iter;
    if (find_song_by_host_id(host, id, &iter))
    {
        gtk_list_store_set(partylist, &iter,
                           COL_IMAGE, playing ?
                                      now_playing_image : 0,
                           -1);
    }
}

void partyshuffle_set_playing(daap_host *host, int id)
{
    int prev_id = playing_song_id;
    daap_host *prev_host = playing_song_host;

    playing_song_id = id;
    playing_song_host = host;

    if (prev_id)
        partyshuffle_set_icon_for_id(prev_host, prev_id, 0);

    if (id)
        partyshuffle_set_icon_for_id(host, id, 1);
}

static void partyshuffle_play_iter(GtkTreeIter *iter)
{
    int id;
    daap_host *host;

    gtk_tree_model_get(GTK_TREE_MODEL(partylist), iter,
                       COL_ID, &id,
                       COL_HOST, &host,
                       -1);
    daap_host_play_song(PLAYSOURCE_PARTY, host, id);
}

void partyshuffle_play_selected()
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(partytreeview);
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    partyshuffle_play_iter(&iter);
}

void partyshuffle_play_next()
{
    GtkTreeIter iter;

    /* FIXME file hack */
    if (!playing_song_host) return; /* not playing a song */

    /* find currently playing */
    if (find_song_by_host_id(playing_song_host, playing_song_id, &iter))
    {
        if (gtk_tree_model_iter_next(GTK_TREE_MODEL(partylist), &iter))
        {
            partyshuffle_play_iter(&iter);
        }
    }
}

gboolean partyshuffle_is_active()
{
    return gtk_toggle_button_get_active(partybutton);
}

/* callbacks */
static void on_party_button_toggled(GtkToggleButton *button,
                                    __UNUSED__ gpointer data)
{
    gboolean toggled = gtk_toggle_button_get_active(button);

    if (toggled)
    {
        set_partyarea_visible();
    }
    else
    {
        set_partyarea_invisible();
    }
}

static void on_button_drag_data_received(__UNUSED__ GtkWidget *widget,
                                         __UNUSED__ GdkDragContext *drag_context,
                                         __UNUSED__ gint x, __UNUSED__ gint y,
                                         GtkSelectionData *data,
                                         __UNUSED__ guint info, __UNUSED__ guint time,
                                         __UNUSED__ gpointer user_data)
{
    if (data->target == daap_song_atom && data->data)
    {
        selection_song_reference *songref;
        int num_songrefs, i;

        if (data->length % sizeof(selection_song_reference)) return;
        num_songrefs = data->length / sizeof(selection_song_reference);

        songref = (selection_song_reference*)data->data;

        for (i = 0; i < num_songrefs; i++)
        {
            char *hostname;
            DAAP_ClientHost_DatabaseItem song;

            char time[11] = {0};
            char track[11] = {0};
            char year[11] = {0};
            char disc[11] = {0};
            int minutes, seconds;

            hostname = daap_host_get_sharename(songref[i].host);

            daap_host_addref(songref[i].host);

            daap_host_get_song_item(songref[i].host, songref[i].id, &song);

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


            party_songlist_append(songref[i].host, hostname, song.id,
                                  song.itemname, song.songartist,
                                  song.songalbum, time, track, disc, year);
        }

    }
#ifdef HAVE_GNOME
    else if (data->target == text_uri_list_atom && data->data)
    {
        GList *list, *cur;
        fprintf(stderr, "got text/uri-list data: '%s'", data->data);
        list = gnome_vfs_uri_list_parse(data->data);
        cur = g_list_first(list);
        while (cur)
        {
            const GnomeVFSURI *cur_uri = (const GnomeVFSURI *)cur->data;
            gchar *filepath = gnome_vfs_uri_to_string(cur_uri, 0);
            gchar *filepath_unescaped = g_strcompress(filepath);
            party_songlist_append(NULL, /* special file source */
                                  "Gnome VFS File", -1,
                                  filepath_unescaped,
                                  NULL, NULL, NULL, NULL, NULL, NULL);
            g_free(filepath);
            g_free(filepath_unescaped);
            cur = g_list_next(cur);
        }
        gnome_vfs_uri_list_free(list);
    }
#endif
}

static void on_song_row_activated(__UNUSED__ GtkTreeView *treeview,
                                  GtkTreePath *path,
                                  __UNUSED__ GtkTreeViewColumn *arg2,
                                  __UNUSED__ gpointer user_data)
{
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(partylist), &iter, path))
    {
        gint id;
        daap_host *host;
        gtk_tree_model_get(GTK_TREE_MODEL(partylist), &iter,
                           COL_ID, &id,
                           COL_HOST, &host,
                           -1);

        if (host)
        {
            daap_host_play_song(PLAYSOURCE_PARTY, host, id);
        }
#ifdef HAVE_GNOME
        else
        {
            char *path;

            gtk_tree_model_get(GTK_TREE_MODEL(partylist), &iter,
                               COL_ITEMNAME, &path,
                               -1);
            filehack_play_song(path);
        }
#endif /* HAVE_GNOME */
    }
}



