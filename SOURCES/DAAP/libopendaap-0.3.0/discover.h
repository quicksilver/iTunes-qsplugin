/* discover class
 *
 * Copyright (c) 2004 David Hammerton
 * crazney@crazney.net
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

/* PRIVATE */

#ifndef _DISCOVER_H
#define _DISCOVER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDiscover_HostListTAG SDiscover_HostList;
struct SDiscover_HostListTAG
{
    char sharename[1005];
    char sharename_friendly[1005];
    char hostname[1005];
    unsigned char ip[4];
    unsigned short port;

    int lost;

    SDiscover_HostList *next;
};

/* type definitions */
typedef struct SDiscoverTAG SDiscover;

/* function pointer defintions */
typedef void (*fnDiscUpdated)(SDiscover *, void *);

/* Interface */
SDiscover *Discover_Create(
#if defined(SYSTEM_POSIX)
						   CP_SThreadPool *pthreadPool,
#endif
                           fnDiscUpdated pfnCallback,
                           void *arg);
unsigned int Discover_AddRef(SDiscover *pDiscThis);
unsigned int Discover_Release(SDiscover *pDiscThis);
unsigned int Discover_GetHosts(SDiscover *pDiscThis,
                               SDiscover_HostList **hosts);

#ifdef __cplusplus
}
#endif

#endif

