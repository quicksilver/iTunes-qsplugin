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
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "lists.h"
#include "tunesbrowser.h"

static void on_album_selection_changed(GtkTreeSelection *selection,
                                       gpointer data);
static void on_album_drag_begin(GtkWidget *widget,
                                GdkDragContext *drag_context,
                                gpointer user_data);
static void on_album_drag_data_get(GtkWidget *widget,
                                   GdkDragContext *drag_context,
                                   GtkSelectionData *data,
                                   guint info,
                                   guint time,
                                   gpointer user_data);
static void on_album_drag_end(GtkWidget *widget,
                              GdkDragContext *drag_context,
                              gpointer user_data);


gint albumlist_default_sort(GtkTreeModel *model,
                            GtkTreeIter *a,
                            GtkTreeIter *b,
                            gpointer user_data);

static GtkListStore *albumlist;
static GtkTreeView *albumtreeview;

enum
{
    ALBUM_PTR,
    ALBUM_COLUMN,
    ALBUM_N_COLUMNS
};

int albumlist_init(GladeXML *xml)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *select;

    albumlist = gtk_list_store_new (ALBUM_N_COLUMNS,
                                    G_TYPE_POINTER, /* album ptr */
                                    G_TYPE_STRING /* album */
                                   );
    albumtreeview = GTK_TREE_VIEW(glade_xml_get_widget(xml, "album"));

    gtk_tree_view_set_model(albumtreeview, GTK_TREE_MODEL(albumlist));

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Album",
                                                      renderer,
                                                      "text", ALBUM_COLUMN,
                                                      NULL);
    gtk_tree_view_append_column (albumtreeview, column);

    gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(albumlist),
                                            albumlist_default_sort,
                                            NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(albumlist),
                                         GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                         GTK_SORT_ASCENDING);

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(albumtreeview));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(select), "changed",
                     G_CALLBACK(on_album_selection_changed),
                     NULL);

    /* set it up as a drag source */
    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(albumtreeview),
                                           GDK_BUTTON1_MASK,
                                           albumlist_src_targets,
                                           albumlist_src_n_targets,
                                           GDK_ACTION_LINK);
    g_signal_connect(G_OBJECT(albumtreeview), "drag-begin",
                     G_CALLBACK(on_album_drag_begin),
                     NULL);
    g_signal_connect(G_OBJECT(albumtreeview), "drag-data-get",
                     G_CALLBACK(on_album_drag_data_get),
                     NULL);
    g_signal_connect(G_OBJECT(albumtreeview), "drag-end",
                     G_CALLBACK(on_album_drag_end),
                     NULL);

    return 1;
}

static void albumlist_prepend(album *album, char *name)
{
    GtkTreeIter iter;

    gtk_list_store_prepend(albumlist, &iter);

    gtk_list_store_set(albumlist, &iter,
                       ALBUM_PTR, album,
                       ALBUM_COLUMN, name,
                       -1);
}

void albumlist_redraw()
{
    char all_str[] = "All (%i Albums)";
    char all_buf[sizeof(all_str) + 11] = {0};
    daap_host *host = daap_host_get_visible();
    int count = 0;
    artist *cur_artist = NULL;
    album *cur_album = NULL;

    gtk_list_store_clear(albumlist);

    if (!host) return;

    cur_artist = daap_host_get_selected_artist(host);
    if (cur_artist)
    {
        while ((cur_album = daap_host_enum_album(host, cur_artist, cur_album)))
        {
            albumlist_prepend(cur_album, daap_host_get_albumname(host, cur_album));
            count++;
        }
    }
    else
    {
        while ((cur_artist = daap_host_enum_artists(host, cur_artist)))
        {
            while ((cur_album = daap_host_enum_album(host, cur_artist, cur_album)))
            {
                albumlist_prepend(cur_album, daap_host_get_albumname(host, cur_album));
                count++;
            }
        }
    }

    sprintf(all_buf, all_str, count);
    albumlist_prepend(NULL, all_buf);
}

/* callback */
static void on_album_selection_changed(GtkTreeSelection *selection,
                                       __UNUSED__ gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    album *album;
    daap_host *host = daap_host_get_visible();

    if (!host) return;

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gtk_tree_model_get (model, &iter,
                            ALBUM_PTR, &album,
                            -1);

        daap_host_set_selected_album(host, album);
    }
}

/* addref on the host */
static void on_album_drag_begin(__UNUSED__ GtkWidget *widget,
                                __UNUSED__ GdkDragContext *drag_context,
                                __UNUSED__ gpointer user_data)
{
    daap_host *host = daap_host_get_visible();
    daap_host_addref(host);
}

/* release the host */
static void on_album_drag_end(__UNUSED__ GtkWidget *widget,
                              __UNUSED__ GdkDragContext *drag_context,
                              __UNUSED__ gpointer user_data)
{
    daap_host *host = daap_host_get_visible();
    daap_host_release(host);
}

static void on_album_drag_data_get(__UNUSED__ GtkWidget *widget,
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
        album *album;
        daap_host *host = daap_host_get_visible();
        int song_id, n_songs, i;
        selection_song_reference *songrefs;
        DAAP_ClientHost_DatabaseItem song;

        reference = g_object_get_data(G_OBJECT(drag_context),
                                      "gtk-tree-view-source-row");
        if (!reference) return;
        path = gtk_tree_row_reference_get_path(reference);
        if (!path) return;

        gtk_tree_model_get_iter(GTK_TREE_MODEL(albumlist), &iter, path);

        gtk_tree_model_get(GTK_TREE_MODEL(albumlist), &iter,
                           ALBUM_PTR, &album,
                           -1);
        gtk_tree_path_free(path);

        /* get the number of songs */
        n_songs = 0;
        song_id = -1;
        while ((song_id = daap_host_enum_artist_album_songs(host, NULL,
                                             song_id,
                                             daap_host_get_selected_artist(host),
                                             album)) != -1)
        {
            n_songs++;
        }

        songrefs = malloc(sizeof(selection_song_reference) * n_songs);

        song_id = -1;
        i = 0;
        while ((song_id = daap_host_enum_artist_album_songs(host, &song,
                                             song_id,
                                             daap_host_get_selected_artist(host),
                                             album)) != -1)
        {
            songrefs[i].host = host;
            songrefs[i].id = song.id;
            i++;
            if (i > n_songs)
                fprintf(stderr, "FIXME: songs changed during dnd. need to lock\n");
        }

        gtk_selection_data_set(data, data->target, 8, (void*)songrefs,
                               sizeof(selection_song_reference) * n_songs);

        free(songrefs);
    }
}

/* sort */
gint albumlist_default_sort(GtkTreeModel *model,
                            GtkTreeIter *a,
                            GtkTreeIter *b,
                            __UNUSED__ gpointer user_data)
{
    gchar *name[2];
    gchar *name_s[2];
    gpointer ptr[2];

    int ret = 0;

    gtk_tree_model_get(model, a,
                       ALBUM_PTR, &ptr[0],
                       ALBUM_COLUMN, &name[0],
                       -1);

    gtk_tree_model_get(model, b,
                       ALBUM_PTR, &ptr[1],
                       ALBUM_COLUMN, &name[1],
                       -1);

    if (!ptr[0]) { ret = -1; goto end; }
    if (!ptr[1]) { ret = 1; goto end; }

    name_s[0] = name[0];
    name_s[1] = name[1];
    if (strncasecmp(name_s[0], "the ", 4) == 0)
        name_s[0] += 4;
    if (strncasecmp(name_s[1], "the ", 4) == 0)
        name_s[1] += 4;
    ret = strcasecmp(name_s[0], name_s[1]);
    if (ret != 0) goto end;

end:
    g_free(name[0]);
    g_free(name[1]);

    return ret;
}


