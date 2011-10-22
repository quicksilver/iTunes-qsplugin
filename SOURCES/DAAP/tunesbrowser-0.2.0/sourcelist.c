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

static void on_source_selection_changed(GtkTreeSelection *selection,
                                        gpointer data);

static GtkListStore *sourcelist;
static GtkTreeView *sourcetreeview;

static GdkPixbuf *remote_host_connected_image;
static GdkPixbuf *remote_host_notconnected_image;

enum
{
    IMAGE_COLUMN,
    SOURCE_COLUMN,
    HOST_COLUMN,
    SOURCE_N_COLUMNS
};

#define XSTR(s) STR(s)
#define STR(s) #s
static void load_images()
{
    remote_host_connected_image = gdk_pixbuf_new_from_file(XSTR(UIDIR) "/comp.blue.png", NULL);
    remote_host_notconnected_image = gdk_pixbuf_new_from_file(XSTR(UIDIR) "/comp.red.png", NULL);
}
#undef XSTR
#undef STR

int sourcelist_init(GladeXML *xml)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *select;

    load_images();

    sourcelist = gtk_list_store_new (SOURCE_N_COLUMNS,
                                     GDK_TYPE_PIXBUF,
                                     G_TYPE_STRING,
                                     G_TYPE_POINTER);
    sourcetreeview = GTK_TREE_VIEW(glade_xml_get_widget(xml, "source"));

    gtk_tree_view_set_model(sourcetreeview, GTK_TREE_MODEL(sourcelist));

    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("",
                                                      renderer,
                                                      "pixbuf", IMAGE_COLUMN,
                                                      NULL);
    gtk_tree_view_append_column (sourcetreeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Source",
                                                      renderer,
                                                      "text", SOURCE_COLUMN,
                                                      NULL);
    gtk_tree_view_append_column (sourcetreeview, column);

    select = gtk_tree_view_get_selection (GTK_TREE_VIEW (sourcetreeview));
    gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (select), "changed",
                      G_CALLBACK (on_source_selection_changed),
                      NULL);

    return 1;
}

static void sourcelist_append(daap_host *host)
{
    GtkTreeIter iter;
    GdkPixbuf *image;

    gtk_list_store_append(sourcelist, &iter);

    image = daap_host_is_connected(host) ?
        remote_host_connected_image : remote_host_notconnected_image;

    gtk_list_store_set(sourcelist, &iter,
                       IMAGE_COLUMN, image,
                       SOURCE_COLUMN, daap_host_get_sharename(host),
                       HOST_COLUMN, host,
                       -1);

}

void sourcelist_redraw()
{
    daap_host *cur = NULL;

    gtk_list_store_clear(sourcelist);

    while ((cur = daap_host_enumerate(cur)))
    {
        sourcelist_append(cur);
    }
}

void sourcelist_set_source_connect_state(daap_host *host, int connected)
{
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(sourcelist),
                                      &iter))
    {
        do
        {
            void *iter_host = NULL;
            gtk_tree_model_get(GTK_TREE_MODEL(sourcelist), &iter,
                               HOST_COLUMN, &iter_host,
                               -1);
            if (iter_host == host)
            {
                gtk_list_store_set(sourcelist, &iter,
                                   IMAGE_COLUMN,
                                   connected ? remote_host_connected_image :
                                               remote_host_notconnected_image,
                                   -1);
            }
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(sourcelist),
                                          &iter));
    }
}

void sourcelist_set_sensitive(int sensitive)
{
    gtk_widget_set_sensitive(GTK_WIDGET(sourcetreeview),
                             sensitive ? TRUE : FALSE);
}

/* callback */
static void on_source_selection_changed(GtkTreeSelection *selection,
                                        __UNUSED__ gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    daap_host *host;

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gtk_tree_model_get (model, &iter,
                            HOST_COLUMN, &host,
                            -1);

        daap_host_set_visible(host);
    }

}



