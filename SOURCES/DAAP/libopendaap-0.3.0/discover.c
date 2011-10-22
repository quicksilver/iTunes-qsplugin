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

#include "portability.h"
#include "thread.h"
#include <sys/select.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "mDNS/mDNSClientAPI.h"
#include "mDNS/mDNSPosix.h"

#include "debug.h"

#include "private.h"
#include "discover.h"
#include "threadpool.h"

#define DEFAULT_DEBUG_CHANNEL "discover"

/* set to 1, 2 or 3 for debug info from mDNS if debugging is compiled in */
extern int gMDNSPlatformPosixVerboseLevel;

/* callbacks from mDNS */
static void InfoCallback(mDNS *const m, DNSQuestion *question,
                         const ResourceRecord *const answer,
                         mDNSBool AddRecord)
{
    /* HACK - shouldn't access private data, but the mDNS api is broken */
    SDiscover *pDiscover = (SDiscover *)question->QuestionContext;

    if (answer->rrtype == kDNSType_SRV)
    {
        SDiscover_HostList *host = pDiscover->pending;

        ConvertDomainNameToCString(&answer->rdata->u.srv.target,
                host->hostname);
        host->port = answer->rdata->u.srv.port.b[0] << 8;
        host->port |= answer->rdata->u.srv.port.b[1];
        pDiscover->q_answer++;
    }
    else if (answer->rrtype == kDNSType_A)
    {
        SDiscover_HostList *host = pDiscover->pending;

        host->ip[0] = answer->rdata->u.ip.b[0];
        host->ip[1] = answer->rdata->u.ip.b[1];
        host->ip[2] = answer->rdata->u.ip.b[2];
        host->ip[3] = answer->rdata->u.ip.b[3];

        pDiscover->q_answer++;
    }
    /* FIXME: could use AAAA type for ipv6 */
}

static void NameCallback(mDNS *const m, DNSQuestion *question,
                         const ResourceRecord *const answer,
                         mDNSBool AddRecord)
{
    /* HACK - shouldn't access private data, but the mDNS api is broken */
    SDiscover *pDiscover = (SDiscover *)question->QuestionContext;

    if (answer->rrtype == kDNSType_PTR)
    {
        domainlabel name;
        domainname type, domain;
        ts_mutex_lock(pDiscover->mtObjectLock);

        SDiscover_HostList *new = malloc(sizeof(SDiscover_HostList));
        memset(new, 0, sizeof(SDiscover_HostList));

        if (!AddRecord) new->lost = 1;

        new->next = pDiscover->prenamed;
        pDiscover->prenamed = new;

        ConvertDomainNameToCString(&answer->rdata->u.name, new->sharename);

        DeconstructServiceName(&answer->rdata->u.name, &name, &type, &domain);
        ConvertDomainLabelToCString_unescaped(&name, new->sharename_friendly);

        pDiscover->q_answer++;
        pDiscover->pending_hosts++;

        ts_mutex_unlock(pDiscover->mtObjectLock);
    }
}

/* mDNS utility functions */
int discover_StartQuery(SDiscover *pDiscover,
                        DNSQuestion *q,
                        char *query_name, mDNSu16 query_type,
                        mDNSQuestionCallback cb)
{
    if (query_name)
        MakeDomainNameFromDNSNameString(&(q->qname), query_name);
    q->InterfaceID = mDNSInterface_Any;
    q->qtype = query_type;
    q->qclass = kDNSClass_IN;
    q->QuestionCallback = cb;
    q->QuestionContext = (void*)pDiscover;

    return mDNS_StartQuery(&pDiscover->mDNSStorage_query, q);
}

int discover_WaitQuery(SDiscover *pDiscover,
                       mDNS *m,
                       int extraFd)
{
    pDiscover->q_answer = 0;
    while (!pDiscover->q_answer)
    {
        fd_set readfds;
        int nfds = 0;
        int result;

        /* FIXME: timeout */
        struct timeval timeout;
        timeout.tv_sec = 40;
        timeout.tv_usec = 0;

        FD_ZERO(&readfds);

        mDNSPosixGetFDSet(m, &nfds,
                          &readfds, &timeout);

        if (extraFd != -1)
            FD_SET(extraFd, &readfds);
        if (extraFd >= nfds)
            nfds = extraFd + 1;

        result = select(nfds, &readfds, NULL, NULL, &timeout);
        if (result > 0)
        {
            if (extraFd != -1 && FD_ISSET(extraFd, &readfds))
                return -1;
            mDNSPosixProcessFDSet(m, &readfds);
        }
    }
    return pDiscover->q_answer;
}

int discover_DoQuery(SDiscover *pDiscover,
                     char *query_name, mDNSu16 query_type,
                     mDNSQuestionCallback cb)
{
    DNSQuestion qNew;
    mStatus status;

    status = discover_StartQuery(pDiscover, &qNew,
                                 query_name, query_type,
                                 cb);

    if (status != mStatus_NoError)
    {
        ERR("an error occured!\n");
        return 0;
    }

    discover_WaitQuery(pDiscover,
                       &pDiscover->mDNSStorage_query, -1);

    mDNS_StopQuery(&pDiscover->mDNSStorage_query, &qNew);

    return 0;
}

static void RemoveFromHaveList(SDiscover *pDiscover, SDiscover_HostList *pDeadHost)
{
    SDiscover_HostList **insert = NULL;
    SDiscover_HostList *cur = NULL;

    insert = &pDiscover->have;
    cur = pDiscover->have;
    while (cur)
    {
        if (strcmp(pDeadHost->sharename, cur->sharename) == 0)
        {
            *insert = cur->next;
            free(cur);
            return;
        }

        insert = &(cur->next);
        cur = cur->next;
    }
}

static void CheckoutHostsWorker(void *pvDiscoverThis, void *arg2)
{
    SDiscover *pDiscover = (SDiscover *)pvDiscoverThis;
    SDiscover_HostList *cur, *next;
    int cmd;

    ts_mutex_lock(pDiscover->mtWorkerLock);
    ts_mutex_lock(pDiscover->mtObjectLock);

    int finalized = 0;

    cur = pDiscover->prenamed;
    pDiscover->prenamed = NULL;
    while (cur)
    {
        int tmp;
        cmd = 0;

        next = cur->next;
        cur->next = pDiscover->pending;
        pDiscover->pending = cur;
        cur = next;

        if (pDiscover->pending->lost)
            continue; /* don't look up info on lost records */

        /* tell the query queue to check out the prenamed host.. */
        write(pDiscover->requestcontrol_pipe[1], &cmd, sizeof(int));

        ts_mutex_unlock(pDiscover->mtObjectLock);

        /* then wait for it to finish */
        read(pDiscover->yieldcontrol_pipe[0], &tmp, sizeof(int));

        ts_mutex_lock(pDiscover->mtObjectLock);

        /* some basic sanity testing */
        if (tmp != cmd)
            ERR("got a return that we didn't expect\n");
    }
    cur = pDiscover->pending;
    while (cur)
    {
        int tmp;

        if (cur->lost)
        {
            RemoveFromHaveList(pDiscover, cur);
            pDiscover->pending_hosts--;
            pDiscover->pending = cur->next;
            free(cur);
            cur = pDiscover->pending;
            finalized++;
            continue;
        }

        cmd = 1;
        /* tell the query queue to check the pending host */
        write(pDiscover->requestcontrol_pipe[1], &cmd, sizeof(int));

        ts_mutex_unlock(pDiscover->mtObjectLock);

        /* then wait for it to finish */
        read(pDiscover->yieldcontrol_pipe[0], &tmp, sizeof(int));

        ts_mutex_lock(pDiscover->mtObjectLock);

        /* some basic sanity testing */
        if (tmp != cmd)
            ERR("got a return that we didn't expect\n");

        pDiscover->pending_hosts--;
        pDiscover->pending = cur->next;
        cur->next = pDiscover->have;
        pDiscover->have = cur;
        cur = pDiscover->pending;
        finalized++;
    }
    if (pDiscover->pending_hosts)
        FIXME("BAD: still answers??? (%i)\n", pDiscover->pending_hosts);

    if (finalized && pDiscover->pfnUpdateCallback)
    {
        pDiscover->pfnUpdateCallback(pDiscover,
                pDiscover->pvCallbackArg);
    }

    ts_mutex_unlock(pDiscover->mtObjectLock);
    ts_mutex_unlock(pDiscover->mtWorkerLock);
}

/* discover thread. */
static void DISC_DiscoverHosts(void *pvDiscoverThis, void *arg2)
{
    SDiscover *pDiscover = (SDiscover *)pvDiscoverThis;
    mStatus status;
    DNSQuestion question;

    domainname type, domain;

    MakeDomainNameFromDNSNameString(&type, "_daap._tcp.");
    MakeDomainNameFromDNSNameString(&domain, "local.");

    ts_mutex_lock(pDiscover->mtObjectLock);

    status = mDNS_StartBrowse(&pDiscover->mDNSStorage_browse,
                              &question,
                              &type, &domain,
                              mDNSInterface_Any,
                              NameCallback, (void*)pDiscover);

    if (status != mStatus_NoError)
    {
        ERR("error\n");
        mDNS_StopQuery(&pDiscover->mDNSStorage_browse,
                       &question);
    }

    ts_mutex_unlock(pDiscover->mtObjectLock);

    while (pDiscover->uiRef > 1)
    {
        if (discover_WaitQuery(pDiscover,
                               &pDiscover->mDNSStorage_browse,
                               pDiscover->requestcontrol_pipe[0])
                == -1)
        {
            int command;
            read(pDiscover->requestcontrol_pipe[0], &command, sizeof(int));
            switch (command)
            {
                case 0: /* prenamed - need srv */
                    if (pDiscover->pending->lost)
                        break; /* don't bother looking for lost names */
                    if (!pDiscover->pending) ERR("something bad is about to happen.\n");
                    discover_DoQuery(pDiscover,
                         pDiscover->pending->sharename,
                         kDNSType_SRV,
                         InfoCallback);
                    break;
                case 1: /* pending - need Addr */
                    if (!pDiscover->pending) ERR("something bad is about to happen.\n");
                    discover_DoQuery(pDiscover,
                         pDiscover->pending->hostname,
                         kDNSType_A, /* FIXME use QType_ANY or _AAAA for ipv6 */
                         InfoCallback);
                    break;
            }
            write(pDiscover->yieldcontrol_pipe[1], &command, sizeof(int));
        }
        else if (pDiscover->q_answer)
        {
            CP_ThreadPool_QueueWorkItem(pDiscover->tp,
                                CheckoutHostsWorker,
                                (void*)pDiscover,
                                NULL);
        }
    }

    mDNS_StopQuery(&pDiscover->mDNSStorage_browse,
                   &question);
}

/* public interface */

SDiscover *Discover_Create(CP_SThreadPool *pThreadPool,
                           fnDiscUpdated pfnCallback,
                           void *arg)
{
    mStatus status;

    SDiscover *pDiscoverNew = malloc(sizeof(SDiscover));
    memset(pDiscoverNew, 0, sizeof(SDiscover));

    pDiscoverNew->uiRef = 1;

    pDiscoverNew->pfnUpdateCallback = pfnCallback;
    pDiscoverNew->pvCallbackArg = arg;

    status = mDNS_Init(&pDiscoverNew->mDNSStorage_query,
                       &pDiscoverNew->mDNSPlatformStorage_query,
                       pDiscoverNew->gRRCache_query,
                       DISC_RR_CACHE_SIZE,
                       mDNS_Init_DontAdvertiseLocalAddresses,
                       mDNS_Init_NoInitCallback,
                       mDNS_Init_NoInitCallbackContext);

    status = mDNS_Init(&pDiscoverNew->mDNSStorage_browse,
                       &pDiscoverNew->mDNSPlatformStorage_browse,
                       pDiscoverNew->gRRCache_browse,
                       DISC_RR_CACHE_SIZE,
                       mDNS_Init_DontAdvertiseLocalAddresses,
                       mDNS_Init_NoInitCallback,
                       mDNS_Init_NoInitCallbackContext);

    if (status != mStatus_NoError)
    {
        ERR("an error occured\n");
        return NULL;
    }

    pipe(pDiscoverNew->requestcontrol_pipe);
    pipe(pDiscoverNew->yieldcontrol_pipe);

    ts_mutex_create(pDiscoverNew->mtObjectLock);
    ts_mutex_create(pDiscoverNew->mtWorkerLock);
    CP_ThreadPool_AddRef(pThreadPool);

    pDiscoverNew->tp = pThreadPool;

    Discover_AddRef(pDiscoverNew); /* for the worker thread */

    CP_ThreadPool_QueueWorkItem(pThreadPool, DISC_DiscoverHosts,
                                (void *)pDiscoverNew, NULL);

    return pDiscoverNew;
}

unsigned int Discover_AddRef(SDiscover *pDiscover)
{
    unsigned int ret;
    ret = ++pDiscover->uiRef;
    return ret;
}

unsigned int Discover_Release(SDiscover *pDiscover)
{
    if (--pDiscover->uiRef)
    {
        return pDiscover->uiRef;
    }

    mDNS_Close(&pDiscover->mDNSStorage_query);
    mDNS_Close(&pDiscover->mDNSStorage_browse);

    close(pDiscover->requestcontrol_pipe[0]);
    close(pDiscover->requestcontrol_pipe[1]);
    close(pDiscover->yieldcontrol_pipe[0]);
    close(pDiscover->yieldcontrol_pipe[1]);

    free(pDiscover);
    return 0;
}

unsigned int Discover_GetHosts(SDiscover *pDiscThis,
                              SDiscover_HostList **hosts)
{
    *hosts = pDiscThis->have;
    return 0;
}

