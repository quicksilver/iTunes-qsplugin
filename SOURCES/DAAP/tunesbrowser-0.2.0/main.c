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
#include <signal.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "lists.h"
#include "tunesbrowser.h"

/********** signal handlers ****************/

static void sighandler_sigsegv(__UNUSED__ int number, siginfo_t *siginfo, __UNUSED__ void *ucontext)
{
    fprintf(stderr, "Oops! TunesBrowser has crashed. Sorry about that!\n");
    fprintf(stderr, "This probably won't be of any use unless you feel like debugging,\n");
    fprintf(stderr, "but the crash occured because of %p being bad.\n", siginfo->si_ptr);
#if 0
    fprintf(stderr, "\n\nRaised SIGSTOP. You can now attach a debugger.\n");
    fprintf(stderr, "Attach to PID %i\n", getpid());
    raise(SIGSTOP);
#endif
    exit(EXIT_FAILURE);
}

static void sighandler_sigpipe(__UNUSED__ int number, __UNUSED__ siginfo_t *siginfo,
                               __UNUSED__ void *ucontext)
{
    fprintf(stderr, "Oops! TunesBrowser has crashed. Sorry about that!\n");
    fprintf(stderr, "I crashed because of a broken pipe! Better call a plumber.\n");
#if 0
    fprintf(stderr, "\n\nRaised SIGSTOP. You can now attach a debugger.\n");
    fprintf(stderr, "Attach to PID %i\n", getpid());
    raise(SIGSTOP);
#endif
    exit(EXIT_FAILURE);
}

static void install_sighandlers()
{
    struct sigaction sa;
    int ret;

    sa.sa_handler = (void*)sighandler_sigsegv;
    sa.sa_flags = SA_NOMASK | SA_SIGINFO;
    ret = sigaction(SIGSEGV, &sa, NULL);

    sa.sa_handler = (void*)sighandler_sigpipe;
    sa.sa_flags = SA_NOMASK | SA_SIGINFO;
    ret = sigaction(SIGPIPE, &sa, NULL);
}

/*******************************************/


void on_tunesbrowser_win_destroy(__UNUSED__ GtkObject *object,
                                 __UNUSED__ gpointer user_data)
{
    printf("exiting\n");
    gtk_main_quit();
}

#define XSTR(s) STR(s)
#define STR(s) #s

int main(int argc, char *argv[]) {
    GladeXML *xml;

    install_sighandlers();

    gtk_init(&argc, &argv);

    xml = glade_xml_new(XSTR(UIDIR) "/tunesbrowser.glade", NULL, NULL);

    glade_xml_signal_autoconnect(xml);

    utilities_init();

    misc_ui_init(xml);

    sourcelist_init(xml);
    artistlist_init(xml);
    albumlist_init(xml);
    songlist_init(xml);
    partyshuffle_init(xml);

    init_daap(&argc, &argv);
    audioplayer_init();

    /* seems gstreamer removes my signal handlers */
    install_sighandlers();

    gtk_main();

    return 0;
}
