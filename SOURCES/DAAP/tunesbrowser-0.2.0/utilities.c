
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

#ifdef HAVE_GNOME
#include <libgnomevfs/gnome-vfs.h>
#endif /* HAVE_GNOME */

#include "tunesbrowser.h"

/************* async commands through idle handler *********/
/* the async command queue is a way to insert a command
 * into a queue that will be run through a gtk idle handler,
 * the point of this is so things can be inserted from
 * any thread, and run from the main glib thread.
 *
 * This differs from simply inserting a cb into an idle handler
 * in two ways:
 *  * commands are always run in order.
 *  * all queued commands are run at once from a single idle handler
 */
typedef struct async_cmd_queueTAG async_cmd_queue;
struct async_cmd_queueTAG
{
    async_cmd_cb callback;
    void *cb_data;

    async_cmd_queue *prev;
    async_cmd_queue *next;
};

static async_cmd_queue *pending_async_cmds_head = NULL;
static async_cmd_queue *pending_async_cmds_tail = NULL;
static GMutex *pending_async_cmds_lock;

static gboolean idle_async_cmd_queue_func(__UNUSED__ gpointer data)
{
    async_cmd_queue *cur;
    async_cmd_queue *new_queue;

    g_mutex_lock(pending_async_cmds_lock);

    new_queue = pending_async_cmds_head;
    pending_async_cmds_head = NULL;
    pending_async_cmds_tail = NULL;

    g_mutex_unlock(pending_async_cmds_lock);

    cur = new_queue;
    while (cur)
    {
        async_cmd_queue *next = cur->next;

        cur->callback(cur->cb_data);

        free(cur);

        cur = next;
    }
    return FALSE;
}

void async_cmd_queue_add(async_cmd_cb callback, void *cb_data)
{
    async_cmd_queue *new_async_cmd;
    int need_add_idle = 0;

    g_mutex_lock(pending_async_cmds_lock);

    if (!pending_async_cmds_tail) need_add_idle = 1;

    new_async_cmd = malloc(sizeof(async_cmd_queue));
    new_async_cmd->next = NULL;
    new_async_cmd->prev = pending_async_cmds_tail;
    new_async_cmd->callback = callback;
    new_async_cmd->cb_data = cb_data;

    pending_async_cmds_tail = new_async_cmd;

    if (!pending_async_cmds_head) pending_async_cmds_head = new_async_cmd;
    else new_async_cmd->prev->next = new_async_cmd;

    g_mutex_unlock(pending_async_cmds_lock);

    if (need_add_idle)
    {
        g_idle_add(idle_async_cmd_queue_func, NULL);
    }
}

/******************* targets and selections *************/
#define daap_song_str "daap-song"
#define text_uri_list_str "text/uri-list"

const GtkTargetEntry partytarget_dst_targets[] =
{
    { daap_song_str, GTK_TARGET_SAME_APP, 0 }
#ifdef HAVE_GNOME
   ,{ text_uri_list_str, 0, 0 }
#endif /* HAVE_GNOME */
};
#ifdef HAVE_GNOME
const int partytarget_dst_n_targets = 2;
#else
const int partytarget_dst_n_targets = 1;
#endif /* HAVE_GNOME */

const GtkTargetEntry songlist_src_targets[] =
{
    { daap_song_str, GTK_TARGET_SAME_APP, 0 }
};
const int songlist_src_n_targets = 1;

const GtkTargetEntry artistlist_src_targets[] =
{
    { daap_song_str, GTK_TARGET_SAME_APP, 0 }
};
const int artistlist_src_n_targets = 1;

const GtkTargetEntry albumlist_src_targets[] =
{
    { daap_song_str, GTK_TARGET_SAME_APP, 0 }
};
const int albumlist_src_n_targets = 1;

GdkAtom daap_song_atom = 0;
GdkAtom text_uri_list_atom = 0;

void utilities_init()
{
    if (!g_thread_supported ()) g_thread_init (NULL);
    pending_async_cmds_lock = g_mutex_new();

    daap_song_atom = gdk_atom_intern(daap_song_str, FALSE);
    text_uri_list_atom = gdk_atom_intern(text_uri_list_str, FALSE);
}

void utilities_finalize()
{
    g_mutex_free(pending_async_cmds_lock);
}

