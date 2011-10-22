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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


#include <gst/gst.h>

#include "lists.h"
#include "tunesbrowser.h"

static GstElement *pipeline_thread, *srcelem, *decoder, *audiosink;

static int songstarted = 0;
static int playing = 0;

enum source_type
{
    SOURCE_PIPE,
    SOURCE_FILE
};
static void audioplayer_destroy_pipeline();
static void audioplayer_setup_pipeline(enum source_type);

__UNUSED__ static char *get_state_str(GstElementState state)
{
    switch (state)
    {
        case GST_STATE_VOID_PENDING:
            return "GST_STATE_VOID_PENDING";
            break;
        case GST_STATE_NULL:
            return "GST_STATE_NULL";
            break;
        case GST_STATE_READY:
            return "GST_STATE_READY";
            break;
        case GST_STATE_PAUSED:
            return "GST_STATE_PAUSED";
            break;
        case GST_STATE_PLAYING:
            return "GST_STATE_PLAYING";
            break;
    }
    return "unknown state";
}

/*********** callbacks ************/
static void async_cmd_cb_gst_state(void *cb_data)
{
    GstElementState state = (GstElementState)cb_data;
    gst_element_set_state (GST_ELEMENT (pipeline_thread), state);
}

static void async_cmd_cb_gst_pipe(void *cb_data)
{
    int pipe_fd = (int)cb_data;
    audioplayer_setup_pipeline(SOURCE_PIPE);
    g_object_set (G_OBJECT (srcelem), "fd", pipe_fd, NULL);
}

static void async_cmd_cb_gst_file(void *cb_data)
{
    char *file = (char*)cb_data;

    audioplayer_setup_pipeline(SOURCE_FILE);
    g_object_set (G_OBJECT (srcelem), "location", file, NULL);

    free(file);
}

static void async_cmd_cb_play_finished(__UNUSED__ void *cb_data)
{
    audioplayer_destroy_pipeline();
    daap_audiocb_finished();
}

static void async_cmd_cb_gst_setup_clock(__UNUSED__ void *data)
{
    GstClock *clock = gst_bin_get_clock(GST_BIN(pipeline_thread));
    songstarted = gst_clock_get_time(clock) / GST_SECOND;
}

/**************** inserts *****************/
static void async_cmd_queue_add_gst_state(GstElementState state)
{
    async_cmd_queue_add(async_cmd_cb_gst_state, (void*)state);
}

static void async_cmd_queue_add_gst_pipefd(int fd)
{
    async_cmd_queue_add(async_cmd_cb_gst_pipe, (void*)fd);
}

static void async_cmd_queue_add_gst_playfile(const char *file)
{
    char *file_new = malloc(strlen(file) + 1);
    strcpy(file_new, file);

    async_cmd_queue_add(async_cmd_cb_gst_file, (void*)file_new);
}

static void async_cmd_queue_add_play_finished()
{
    async_cmd_queue_add(async_cmd_cb_play_finished, NULL);
}

static void async_cmd_queue_add_setup_clock()
{
    async_cmd_queue_add(async_cmd_cb_gst_setup_clock, NULL);
}

/************* end gstreamer command setting **************************/

static void cb_eos(__UNUSED__ GstElement *elem, __UNUSED__ gpointer data)
{
    audioplayer_stop();
}

static void cb_error (__UNUSED__ GstElement *gstelement,
                      __UNUSED__ GstElement *arg1,
                      __UNUSED__ gpointer arg2,
                      gchar *arg3,
                      __UNUSED__ gpointer user_data)
{
    fprintf(stderr, "an error occured:\n%s\n\n", arg3);
}

static void cb_iterate(GstBin *bin, __UNUSED__ gpointer data)
{
    GstClock *clock = gst_bin_get_clock(bin);
    int seconds = gst_clock_get_time(clock) / GST_SECOND;

    seconds = seconds - songstarted;

    if (!playing) seconds = -1;
    set_songpos(seconds);

    return;
}

static void audioplayer_destroy_pipeline()
{
    /* unlink them first */
    gst_element_unlink_many (srcelem, decoder, audiosink, NULL);

    /* remove the source and decoder from the pipeline */
    gst_bin_remove_many (GST_BIN (pipeline_thread), srcelem, decoder, NULL);

    gst_object_unref(GST_OBJECT(pipeline_thread));
}

static void audioplayer_setup_pipeline(enum source_type stype)
{
    pipeline_thread = gst_thread_new ("pipeline");
#if (GST_VERSION_MINOR <= 6)
    gst_bin_set_post_iterate_function(GST_BIN(pipeline_thread),
                                      cb_iterate, NULL);
#else
    g_signal_connect_after(G_OBJECT(pipeline_thread), "iterate",
                           G_CALLBACK(cb_iterate),
                           NULL);
#endif

    /* create new ones */
    switch (stype)
    {
    case SOURCE_PIPE:
        srcelem = gst_element_factory_make ("fdsrc", "source");
        break;
    case SOURCE_FILE:
        srcelem = gst_element_factory_make ("gnomevfssrc", "source");
        break;
    }
    if (!srcelem) goto gst_element_err;

#if (GST_VERSION_MINOR < 8)
    /* older versions of gstreamer seem to crash when I use the
     * spider plugin
     */
    decoder = gst_element_factory_make ("mad", "decoder");
#else
    decoder = gst_element_factory_make ("spider", "decoder");
#endif

    if (!decoder) goto gst_element_err;

    audiosink = gst_element_factory_make ("alsasink", "play_audio");
    if (!audiosink)
        audiosink = gst_element_factory_make ("osssink", "play_audio");
    if (!audiosink)
        audiosink = gst_element_factory_make ("artsdsink", "play_audio");
    if (!audiosink)
        audiosink = gst_element_factory_make ("esdsink", "play_audio");
    if (!audiosink)
    {
        fprintf(stderr, "Failed to load gstreamer sink - tried all 3 of 'osssink', "
                        "'alsasink', 'artsdsink' and 'esdsink'.\n");
        goto gst_element_err;
    }
    gst_bin_add (GST_BIN (pipeline_thread), audiosink);

    /* now link them */
    gst_bin_add_many (GST_BIN (pipeline_thread), srcelem, decoder, NULL);
    gst_element_link_many (srcelem, decoder, audiosink, NULL);

    g_signal_connect(G_OBJECT(pipeline_thread), "eos",
                     G_CALLBACK(cb_eos),
                     NULL);
    g_signal_connect(G_OBJECT(pipeline_thread), "error",
                     G_CALLBACK(cb_error),
                     NULL);

    return;

gst_element_err:
    fprintf(stderr, "Couldn't load one of the elements. Please make sure the gstreamer package "
                    "that contains that element is installed on your system. "
                    "(see above for what element may be missing).\n");
    exit(EXIT_FAILURE);
}

void audioplayer_init()
{
    gst_init(0, NULL);

    return;

}

void audioplayer_finalize()
{
    g_object_unref(G_OBJECT(pipeline_thread));
}

static void audioplayer_loadpipe(int fd)
{
    async_cmd_queue_add_gst_pipefd(fd);
}

void audioplayer_playpipe(int fd)
{
    playing = 1;
    audioplayer_loadpipe(fd);

    async_cmd_queue_add_gst_state(GST_STATE_PLAYING);
    async_cmd_queue_add_setup_clock();
}

void audioplayer_playfile(const char *file)
{
    playing = 1;
    async_cmd_queue_add_gst_playfile(file);

    async_cmd_queue_add_gst_state(GST_STATE_PLAYING);
    async_cmd_queue_add_setup_clock();
}

void audioplayer_pause()
{
    async_cmd_queue_add_gst_state(GST_STATE_PAUSED);
}

void audioplayer_resume()
{
    async_cmd_queue_add_gst_state(GST_STATE_PLAYING);
}

void audioplayer_stop()
{
    if (!playing) return; /* already stopping (eos?) */
    playing = 0;
    async_cmd_queue_add_play_finished();
}

