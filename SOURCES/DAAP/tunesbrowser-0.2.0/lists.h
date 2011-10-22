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



#ifndef __LISTS_H
#define __LISTS_H

#include "tunesbrowser.h"

/* sourcelist */
int sourcelist_init(GladeXML *xml);
void sourcelist_redraw();
/* fixme possibly remove this? */
void sourcelist_set_source_connect_state(daap_host *host, int connected);
void sourcelist_set_sensitive(int sensitive);

/* artistlist */
int artistlist_init(GladeXML *xml);
void artistlist_redraw();

/* albumlist */
int albumlist_init(GladeXML *xml);
void albumlist_redraw();

/* songlist */
int songlist_init(GladeXML *xml);
void songlist_redraw();
int songlist_get_selected_id();
void songlist_set_playing(daap_host *host, int id);

/* party shuffle */
int partyshuffle_init(GladeXML *xml);
void partyshuffle_removehost(daap_host *host); /* call when a host is being removed */
void partyshuffle_set_playing(daap_host *host, int id);
void partyshuffle_play_selected();
void partyshuffle_play_next();
gboolean partyshuffle_is_active();

#endif


