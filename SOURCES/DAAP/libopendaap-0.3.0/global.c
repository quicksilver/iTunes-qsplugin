/*
 *
 * Copyright (c) 2003 David Hammerton
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

#include "portability.h"

#ifdef SYSTEM_POSIX
#include <sys/time.h>
#include <time.h>
#endif

#include <stdio.h>

/* PRIVATE */

/* globals, tools, etc. */

/* returns time since its first call in milliseconds */
unsigned int CP_GetTickCount()
{
#ifdef SYSTEM_POSIX

    static int startSeconds = -1;
    struct timeval tv;

    unsigned int ticks = 0;

    gettimeofday(&tv, NULL);
    if (startSeconds < 0)
        startSeconds = tv.tv_sec;
    ticks += ((tv.tv_sec - startSeconds) * 1000);
    ticks += tv.tv_usec / 1000;
    return ticks;

#elif defined(SYSTEM_WIN32)
	return (unsigned int)GetTickCount();
#else
#error IMPLEMENT ME
#endif
}
