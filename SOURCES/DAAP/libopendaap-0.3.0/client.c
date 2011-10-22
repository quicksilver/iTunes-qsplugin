/* client class
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
 */

#include "portability.h"
#include "thread.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(SYSTEM_POSIX)
# include "threadpool.h"
#endif
#include "client.h"
#include "discover.h"
#include "http_client.h"
#include "private.h"

#include "daap.h"
#include "daap_contentcodes.h"
#include "dmap_generics.h"

#include "authentication/hasher.h"

#include "debug/debug.h"

#define DEFAULT_DEBUG_CHANNEL "client"


static DAAP_SClientHost *DAAP_ClientHost_Create(DAAP_SClient *parent, char *host,
                                                char *sharename);
static int Priv_DAAP_ClientHost_GetDatabaseItems(DAAP_SClientHost *pCHThis,
                                                 int databaseid);
static int Priv_DAAP_ClientHost_GetDatabasePlaylists(DAAP_SClientHost *pCHThis,
                                                     int databaseid);

struct DAAP_ClientHost_FakeTAG
{
    char sharename[1005];
    char sharename_friendly[1005];
    unsigned char ip[4];
    unsigned short port;
    DAAP_ClientHost_Fake *next;

    int marked;
};

static int ClientHasHost_AndMark(DAAP_SClient *pClient, char *sharename)
{
    DAAP_ClientHost_Fake *cur_fake = pClient->fakehosts;
    DAAP_SClientHost *cur_real = pClient->hosts;
    while (cur_fake)
    {
        if (strcmp(cur_fake->sharename, sharename) == 0)
        {
            cur_fake->marked = 1;
            return 1;
        }
        cur_fake = cur_fake->next;
    }
    while (cur_real)
    {
        if (strcmp(cur_real->sharename, sharename) == 0)
        {
            cur_real->marked = 1;
            return 1;
        }
        cur_real = cur_real->next;
    }
    return 0;
}

static void DiscoverCB(SDiscover *disc, void *pv_pClient)
{
    DAAP_SClient *pClient = (DAAP_SClient*)pv_pClient;
    int added = 0;
    int deleted = 0;

    DAAP_ClientHost_Fake *cur_fake, *prev_fake;
    DAAP_SClientHost *cur_real;
    SDiscover_HostList *hosts;

    ts_mutex_lock(pClient->mtObjectLock);

    cur_fake = pClient->fakehosts;
    cur_real = pClient->hosts;
    while (cur_fake)
    {
        cur_fake->marked = 0;
        cur_fake = cur_fake->next;
    }
    while (cur_real)
    {
        cur_real->marked = 0;
        cur_real = cur_real->next;
    }

    Discover_GetHosts(disc, &hosts);
    while (hosts)
    {
        if (!ClientHasHost_AndMark(pClient, hosts->sharename))
        {
            char hostname_buf[20] = {0};
            sprintf(hostname_buf,
                    "%hhu.%hhu.%hhu.%hhu:%hu",
                    hosts->ip[0], hosts->ip[1],
                    hosts->ip[2], hosts->ip[3],
                    hosts->port);
            if (!DAAP_Client_AddHost(pClient, hostname_buf,
                        hosts->sharename, hosts->sharename_friendly))
            {
                /* if we failed to add the host, add it to the fake
                 * so we don't try and add it again... */
                DAAP_ClientHost_Fake *new_fake = malloc(sizeof(DAAP_ClientHost_Fake));
                strcpy(new_fake->sharename, hosts->sharename);
                strcpy(new_fake->sharename_friendly, hosts->sharename_friendly);
                new_fake->ip[0] = hosts->ip[0];
                new_fake->ip[1] = hosts->ip[1];
                new_fake->ip[2] = hosts->ip[2];
                new_fake->ip[3] = hosts->ip[3];
                new_fake->port = hosts->port;
                new_fake->next = pClient->fakehosts;
                new_fake->marked = 1;
                pClient->fakehosts = new_fake;
            }
            else
            {
                added++;
            }
        }
        hosts = hosts->next;
    }

    /* delete any that arn't there... */
    prev_fake = NULL;
    cur_fake = pClient->fakehosts;
    cur_real = pClient->hosts;
    while (cur_fake)
    {
        DAAP_ClientHost_Fake *next_fake = cur_fake->next;
        if (!cur_fake->marked)
        {
            if (prev_fake) prev_fake->next = cur_fake->next;
            else pClient->fakehosts = cur_fake->next;
            free(cur_fake);
            deleted++;
        }
        cur_fake = next_fake;
    }
    while (cur_real)
    {
        DAAP_SClientHost *next_real = cur_real->next;
        if (!cur_real->marked)
        {
            DAAP_SClientHost *prev = cur_real->prev;
            if (DAAP_ClientHost_Release(cur_real) != 0)
                TRACE("app still holds reference to deleted host\n");

            if (prev) prev->next = next_real;
            else pClient->hosts = next_real;
            if (next_real) next_real->prev = prev;

            deleted++;
        }
        cur_real = next_real;
    }

    ts_mutex_unlock(pClient->mtObjectLock);

    if (added || deleted)
    {
        TRACE("%i added, %i deleted\n", added, deleted);
        pClient->pfnCallbackStatus(pClient, DAAP_STATUS_hostschanged,
                0, pClient->pvCallbackStatusContext);
    }
}


DAAP_SClient *DAAP_Client_Create(DAAP_fnClientStatus pfnCallback,
                                 void *pvCallbackContext)
{
    DAAP_SClient *pClientNew = malloc(sizeof(DAAP_SClient));
    memset(pClientNew, 0, sizeof(DAAP_SClient));

    dmap_init();

    pClientNew->uiRef = 1;

    pClientNew->pfnCallbackStatus = pfnCallback;
    pClientNew->pvCallbackStatusContext = pvCallbackContext;

#if defined(SYSTEM_POSIX)
    pClientNew->tp = CP_ThreadPool_Create(4);
#endif

    pClientNew->discover = Discover_Create(
#if defined(SYSTEM_POSIX)
		    pClientNew->tp,
#endif
            DiscoverCB, (void*)pClientNew);

    pClientNew->update_watch = NULL;

    ts_mutex_create(pClientNew->mtObjectLock);

    return pClientNew;
}

int DAAP_Client_SetDebug(DAAP_SClient *pCThis, const char *const debug)
{
    return debug_init(debug);
}

unsigned int DAAP_Client_AddRef(DAAP_SClient *pCThis)
{
    return ++pCThis->uiRef;
}

unsigned int DAAP_Client_Release(DAAP_SClient *pCThis)
{
    if (--pCThis->uiRef)
        return pCThis->uiRef;

    while (pCThis->hosts)
    {
        DAAP_SClientHost *cur = pCThis->hosts;
        pCThis->hosts = cur->next;
        if (pCThis->hosts) pCThis->hosts->prev = NULL;
        cur->next = NULL;
        DAAP_ClientHost_Release(cur);
    }

    if (pCThis->update_watch)
    {
        HTTP_Client_WatchQueue_Destroy(pCThis->update_watch);
    }

    Discover_Release(pCThis->discover);
#if defined(SYSTEM_POSIX)
    CP_ThreadPool_Release(pCThis->tp);
#endif
    free(pCThis);

    return 0;
}

unsigned int DAAP_Client_EnumerateHosts(DAAP_SClient *pCThis,
                                        DAAP_fnClientEnumerateHosts pfnCallback,
                                        void *context)
{
    int count = 0;
    DAAP_SClientHost *cur = pCThis->hosts;

    ts_mutex_lock(pCThis->mtObjectLock);

    while(cur)
    {
        pfnCallback(pCThis, cur, context);
        cur = cur->next;
        count++;
    }
    ts_mutex_unlock(pCThis->mtObjectLock);
    return count;
}

/* hack */
DAAP_SClientHost *DAAP_Client_AddHost(DAAP_SClient *pCThis, char *host,
        char *sharename, char *sharename_friendly)
{
    DAAP_SClientHost *pClientHost = DAAP_ClientHost_Create(pCThis, host,
                                                           sharename_friendly);
    if (!pClientHost) return NULL;
    if (sharename) strcpy(pClientHost->sharename, sharename);
    if (pCThis->hosts)
        pCThis->hosts->prev = pClientHost;
    pClientHost->next = pCThis->hosts;
    pCThis->hosts = pClientHost;
    pClientHost->marked = 1;
    return pClientHost;
}

/* ClientHost */

/* private initial stuff */
static int Priv_DAAP_ClientHost_InitialTransaction(DAAP_SClientHost *pCHThis)
{
    HTTP_GetResult *httpRes;

    protoParseResult_serverinfo serverinfo;
    protoParseResult_login logininfo;
    protoParseResult_update updateinfo;

    char hash[33] = {0};
    char updateUrl[] = "/update?session-id=%i&revision-number=1";
    char buf[sizeof(updateUrl) + 11];

    /* get server-info */
    httpRes = HTTP_Client_Get(pCHThis->connection, "/server-info", NULL, NULL, 0);
    if (!httpRes)
        return 0;

    serverinfo.h.expecting = QUERY_SERVERINFORESPONSE;
    dmap_parseProtocolData(httpRes->contentlen, httpRes->data,
                           (protoParseResult *)&serverinfo);

    HTTP_Client_FreeResult(httpRes);

    pCHThis->version_major = serverinfo.daap_version.v1;
    pCHThis->version_minor = serverinfo.daap_version.v2;

    /* validate server info */
    if (serverinfo.dmap_version.v1 != 2 && serverinfo.dmap_version.v2 != 0)
    {
        FIXME("unknown version\n");
        return 0;
    }

    /* hack */
    free(serverinfo.hostname);

    /* get content codes */
    httpRes = HTTP_Client_Get(pCHThis->connection, "/content-codes", NULL, NULL, 0);

    if (!httpRes)
        return 0;

#if 0
    {
        FILE *f = fopen("/tmp/content-codes", "w");
        int i;
        for (i = 0; i < httpRes->contentlen; i++)
        {
            fputc(((unsigned char*)httpRes->data)[i], f);
        }
        fclose(f);
    }
#endif

    /* send content codes to the dmap parser. don't require any result */
    dmap_parseProtocolData(httpRes->contentlen, httpRes->data, NULL);

    HTTP_Client_FreeResult(httpRes);

    /* send login */
    httpRes = HTTP_Client_Get(pCHThis->connection, "/login", NULL, NULL, 0);

    if (!httpRes)
        return 0;

    logininfo.h.expecting = QUERY_LOGINRESPONSE;
    dmap_parseProtocolData(httpRes->contentlen, httpRes->data,
                           (protoParseResult*)&logininfo);

    HTTP_Client_FreeResult(httpRes);

    pCHThis->sessionid = logininfo.sessionid;

    /* get the current revision id */
    sprintf(buf, updateUrl, pCHThis->sessionid);

    GenerateHash(pCHThis->version_major, buf, 2, hash, 0);

    httpRes = HTTP_Client_Get(pCHThis->connection, buf, hash, NULL, 0);

    if (!httpRes)
        return 0;

    updateinfo.h.expecting = QUERY_UPDATERESPONSE;
    dmap_parseProtocolData(httpRes->contentlen, httpRes->data,
                           (protoParseResult*)&updateinfo);

    HTTP_Client_FreeResult(httpRes);

    pCHThis->revision_number = updateinfo.serverrevision;

    return 1;
}

static int Priv_DAAP_ClientHost_GetDatabases(DAAP_SClientHost *pCHThis)
{
    char hash[33] = {0};
    char databasesUrl[] = "/databases?session-id=%i&revision-number=%i";
    char buf[sizeof(databasesUrl) + 11 + 11];

    protoParseResult_genericPreListing databases;

    HTTP_GetResult *httpRes;

    int i, j;
    char *strpos;
    int sizereq;

    sprintf(buf, databasesUrl, pCHThis->sessionid, pCHThis->revision_number);

    GenerateHash(pCHThis->version_major, buf, 2, hash, 0);

    httpRes = HTTP_Client_Get(pCHThis->connection, buf, hash, NULL, 0);

    if (!httpRes)
        return 0;

    databases.h.expecting = QUERY_GENERICLISTING;
    dmap_parseProtocolData(httpRes->contentlen, httpRes->data,
                           (protoParseResult*)&databases);

    HTTP_Client_FreeResult(httpRes);

    if (databases.totalcount != databases.returnedcount)
        FIXME("didn't return all db's, need to handle split\n");

    j = 0;
    sizereq = sizeof(DAAP_ClientHost_Database) * databases.returnedcount;
    for (i = 0; i < databases.returnedcount; i++)
    {
        dmapGenericContainer *item = &(databases.listitems[i]);
        DMAP_INT32 itemid;
        DMAP_STRING itemname;

        if (dmapGeneric_LookupContainerItem_INT32(item, dmap_l("itemid"), &itemid) !=
                DMAP_DATATYPE_INT32)
            continue;

        if (dmapGeneric_LookupContainerItem_STRING(item, dmap_l("itemname"), &itemname) !=
                DMAP_DATATYPE_STRING)
            continue;

        sizereq += strlen(itemname) + 1;

        j++;
    }

    if (pCHThis->databases) free(pCHThis->databases);

    pCHThis->databases_size = sizereq;
    pCHThis->databases = malloc(sizereq);

    if (pCHThis->dbitems)
    {
        int i;
        for (i = 0; i < pCHThis->nDatabases; i++)
        {
            free(&(pCHThis->dbitems->items[i]));
        }
        free(pCHThis->dbitems);
    }

    if (pCHThis->dbplaylists) free(pCHThis->dbplaylists);

    pCHThis->dbitems = malloc(sizeof(DatabaseItemContainer) * j);
    memset(pCHThis->dbitems, 0, sizeof(DatabaseItemContainer) * j);
    pCHThis->dbplaylists = malloc(sizeof(DatabasePlaylistContainer) * j);
    memset(pCHThis->dbplaylists, 0, sizeof(DatabasePlaylistContainer) * j);

    pCHThis->nDatabases = j;

    strpos = (char*)(((char*)pCHThis->databases) +
             (sizeof(DAAP_ClientHost_Database) * databases.returnedcount));

    j = 0;
    for (i = 0; i < databases.returnedcount; i++)
    {
        dmapGenericContainer *item = &(databases.listitems[i]);
        DAAP_ClientHost_Database *db = &(pCHThis->databases[j]);
        DMAP_INT32 itemid;
        DMAP_STRING itemname;

        if (dmapGeneric_LookupContainerItem_INT32(item, dmap_l("itemid"), &itemid) !=
                DMAP_DATATYPE_INT32)
            continue;

        if (dmapGeneric_LookupContainerItem_STRING(item, dmap_l("itemname"), &itemname) !=
                DMAP_DATATYPE_STRING)
            continue;

        db->id = itemid;
        strcpy(strpos, itemname);
        db->name = strpos;
        strpos += strlen(itemname) + 1;

        pCHThis->dbitems[j].id = itemid;
        pCHThis->dbplaylists[j].id = itemid;
        Priv_DAAP_ClientHost_GetDatabaseItems(pCHThis, itemid);
        Priv_DAAP_ClientHost_GetDatabasePlaylists(pCHThis, itemid);

        j++;
    }

    freeGenericPreListing(&databases);

    return 1;
}

static int Priv_DAAP_ClientHost_GetDatabaseItems(DAAP_SClientHost *pCHThis,
                                                 int databaseid)
{
    char hash[33] = {0};
    char itemsUrl[] = "/databases/%i/items?session-id=%i&revision-number=%i&meta="
                      "dmap.itemid,dmap.itemname,daap.songalbum,daap.songartist,"
                      "daap.songbeatsperminute,daap.songbitrate,daap.songdisccount,"
                      "daap.songdiscnumber,daap.songgenre,daap.songsamplerate,"
                      "daap.songsize,daap.songtime,daap.songtrackcount,"
                      "daap.songtracknumber,daap.songuserrating,"
                      "daap.songyear,daap.songformat";
    char buf[sizeof(itemsUrl) + 11 + 11 + 11];

    protoParseResult_genericPreListing items;

    DatabaseItemContainer *dbcontainer = NULL;

    HTTP_GetResult *httpRes;

    int i, j;
    char *strpos;
    int sizereq;

    for (i = 0; i < pCHThis->nDatabases; i++)
    {
        DatabaseItemContainer *container = &(pCHThis->dbitems[i]);
        if (container->id == databaseid)
        {
            dbcontainer = container;
            break;
        }
    }

    if (!dbcontainer)
    {
        ERR("container not found, returning\n");
        freeGenericPreListing(&items);
        return 0;
    }


    sprintf(buf, itemsUrl, databaseid, pCHThis->sessionid, pCHThis->revision_number);

    GenerateHash(pCHThis->version_major, buf, 2, hash, 0);

    httpRes = HTTP_Client_Get(pCHThis->connection, buf, hash, NULL, 0);

    if (!httpRes)
        return 0;

    items.h.expecting = QUERY_GENERICLISTING;
    dmap_parseProtocolData(httpRes->contentlen, httpRes->data,
                           (protoParseResult*)&items);

    HTTP_Client_FreeResult(httpRes);

    if (items.totalcount != items.returnedcount)
        FIXME("didn't return all db's, need to handle split (%i vs %i)\n",
              items.totalcount, items.returnedcount);

    TRACE("returnedcount: %i\n", items.returnedcount);
    sizereq = sizeof(DAAP_ClientHost_DatabaseItem) * items.returnedcount;
    for (i = 0; i < items.returnedcount; i++)
    {
        dmapGenericContainer *item = &(items.listitems[i]);
        DMAP_INT32 buf32;
        DMAP_INT16 buf16;
        DMAP_INT8 buf8;
        DMAP_STRING buf;

        if (dmapGeneric_LookupContainerItem_INT32(item, dmap_l("itemid"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;

        if (dmapGeneric_LookupContainerItem_STRING(item, dmap_l("itemname"), &buf) !=
                DMAP_DATATYPE_STRING)
            continue;
        sizereq += strlen(buf) + 1;

        if (dmapGeneric_LookupContainerItem_STRING(item, daap_l("songalbum"), &buf) ==
                DMAP_DATATYPE_STRING)
            sizereq += strlen(buf) + 1;

        if (dmapGeneric_LookupContainerItem_STRING(item, daap_l("songartist"), &buf) ==
                DMAP_DATATYPE_STRING)
            sizereq += strlen(buf) + 1;

        /*
         * OPTIONAL - had to comment this out because mt-daapd doesn't provide all of these
         * all of the time.
         *
        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songbeatsperminute"), &buf16) !=
                DMAP_DATATYPE_INT16)
            continue;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songbitrate"), &buf16) !=
                DMAP_DATATYPE_INT16)
            continue;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songdisccount"), &buf16) !=
                DMAP_DATATYPE_INT16)
            continue;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songdiscnumber"), &buf16) !=
                DMAP_DATATYPE_INT16)
            continue;
            */

        if (dmapGeneric_LookupContainerItem_STRING(item, daap_l("songgenre"), &buf) ==
                DMAP_DATATYPE_STRING)
            sizereq += strlen(buf) + 1;

        if (dmapGeneric_LookupContainerItem_INT32(item, daap_l("songsamplerate"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;

        if (dmapGeneric_LookupContainerItem_INT32(item, daap_l("songsize"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;

        if (dmapGeneric_LookupContainerItem_INT32(item, daap_l("songtime"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;

        /* optional, see above re mt-daapd
        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songtrackcount"), &buf16) !=
                DMAP_DATATYPE_INT16)
            continue;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songtracknumber"), &buf16) !=
                DMAP_DATATYPE_INT16)
            continue;

        if (dmapGeneric_LookupContainerItem_INT8(item, daap_l("songuserrating"), &buf8) !=
                DMAP_DATATYPE_INT8)
            continue;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songyear"), &buf16) !=
                DMAP_DATATYPE_INT16)
            continue;
            */

        if (dmapGeneric_LookupContainerItem_STRING(item, daap_l("songformat"), &buf) !=
                DMAP_DATATYPE_STRING)
            continue;
        sizereq += strlen(buf) + 1;
    }

    if (dbcontainer->items) free(dbcontainer->items);

    dbcontainer->items_size = sizereq;
    dbcontainer->items = malloc(sizereq);

    strpos = (char*)(((char*)dbcontainer->items) +
             (sizeof(DAAP_ClientHost_DatabaseItem) * items.returnedcount));

    j = 0;
    for (i = 0; i < items.returnedcount; i++)
    {
        dmapGenericContainer *item = &(items.listitems[i]);
        DAAP_ClientHost_DatabaseItem *dbitem = &(dbcontainer->items[j]);
        DMAP_INT32 buf32;
        DMAP_INT16 buf16;
        DMAP_INT8 buf8;
        DMAP_STRING itemname, songalbum, songartist;
        DMAP_STRING songgenre, songformat;

        if (dmapGeneric_LookupContainerItem_INT32(item, dmap_l("itemid"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;
        dbitem->id = buf32;

        if (dmapGeneric_LookupContainerItem_STRING(item, daap_l("songformat"), &songformat) !=
                DMAP_DATATYPE_STRING)
            continue;

        if (dmapGeneric_LookupContainerItem_STRING(item, dmap_l("itemname"), &itemname) !=
                DMAP_DATATYPE_STRING)
            continue;

        if (dmapGeneric_LookupContainerItem_STRING(item, daap_l("songalbum"), &songalbum) !=
                DMAP_DATATYPE_STRING)
            songalbum = NULL;

        if (dmapGeneric_LookupContainerItem_STRING(item, daap_l("songartist"), &songartist) !=
                DMAP_DATATYPE_STRING)
            songartist = NULL;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songbeatsperminute"), &buf16) ==
                DMAP_DATATYPE_INT16)
            dbitem->songbeatsperminute = buf16;
        else
            dbitem->songbeatsperminute = 0;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songbitrate"), &buf16) ==
                DMAP_DATATYPE_INT16)
            dbitem->songbitrate = buf16;
        else
            dbitem->songbitrate = 0;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songdisccount"), &buf16) ==
                DMAP_DATATYPE_INT16)
            dbitem->songdisccount = buf16;
        else
            dbitem->songdisccount = 0;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songdiscnumber"), &buf16) ==
                DMAP_DATATYPE_INT16)
            dbitem->songdiscnumber = buf16;
        else
            dbitem->songdiscnumber = buf16;

        if (dmapGeneric_LookupContainerItem_STRING(item, daap_l("songgenre"), &songgenre) !=
                DMAP_DATATYPE_STRING)
            songgenre = NULL;

        if (dmapGeneric_LookupContainerItem_INT32(item, daap_l("songsamplerate"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;
        dbitem->songsamplerate = buf32;

        if (dmapGeneric_LookupContainerItem_INT32(item, daap_l("songsize"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;
        dbitem->songsize = buf32;

        if (dmapGeneric_LookupContainerItem_INT32(item, daap_l("songtime"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;
        dbitem->songtime = buf32;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songtrackcount"), &buf16) ==
                DMAP_DATATYPE_INT16)
            dbitem->songtrackcount = buf16;
        else
            dbitem->songtrackcount = 0;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songtracknumber"), &buf16) ==
                DMAP_DATATYPE_INT16)
            dbitem->songtracknumber = buf16;
        else
            dbitem->songtracknumber = 0;

        if (dmapGeneric_LookupContainerItem_INT8(item, daap_l("songuserrating"), &buf8) ==
                DMAP_DATATYPE_INT8)
            dbitem->songuserrating = buf8;
        else
            dbitem->songuserrating = 0;

        if (dmapGeneric_LookupContainerItem_INT16(item, daap_l("songyear"), &buf16) ==
                DMAP_DATATYPE_INT16)
            dbitem->songyear = buf16;
        else
            dbitem->songyear = 0;

        strcpy(strpos, itemname);
        dbitem->itemname = strpos;
        strpos += strlen(strpos)+1;

        if (songalbum)
        {
            strcpy(strpos, songalbum);
            dbitem->songalbum = strpos;
            strpos += strlen(strpos)+1;
        }
        else dbitem->songalbum = NULL;

        if (songartist)
        {
            strcpy(strpos, songartist);
            dbitem->songartist = strpos;
            strpos += strlen(strpos)+1;
        }
        else dbitem->songartist = NULL;

        if (songgenre)
        {
            strcpy(strpos, songgenre);
            dbitem->songgenre = strpos;
            strpos += strlen(strpos)+1;
        }
        else dbitem->songgenre = NULL;

        strcpy(strpos, songformat);
        dbitem->songformat = strpos;
        strpos += strlen(strpos)+1;

        j++;
    }

    dbcontainer->nItems = j;
    TRACE("items: %i\n", j);

    freeGenericPreListing(&items);

    return 1;
}

static int Priv_DAAP_ClientHost_GetDatabasePlaylistItems(DAAP_SClientHost *pCHThis,
                                                         int databaseid,
                                                         int playlistid)
{
    char hash[33] = {0};
    char playlistUrl[] = "/databases/%i/containers/%i/items?session-id=%i&revision-number=%i";
    char buf[sizeof(playlistUrl) + 11 + 11 + 11 + 11];

    protoParseResult_genericPreListing playlist;

    DatabasePlaylistContainer *dbcontainer = NULL;

    HTTP_GetResult *httpRes;

    int i;

    for (i = 0; i < pCHThis->nDatabases; i++)
    {
        DatabasePlaylistContainer *container = &(pCHThis->dbplaylists[i]);
        if (container->id == databaseid)
        {
            dbcontainer = container;
            break;
        }
    }

    if (!dbcontainer)
    {
        ERR("container not found, returning\n");
        freeGenericPreListing(&playlist);
    }

    sprintf(buf, playlistUrl, databaseid, playlistid, pCHThis->sessionid, pCHThis->revision_number);

    GenerateHash(pCHThis->version_major, buf, 2, hash, 0);

    httpRes = HTTP_Client_Get(pCHThis->connection, buf, hash, NULL, 0);

    if (!httpRes)
        return 0;

    playlist.h.expecting = QUERY_GENERICLISTING;

    dmap_parseProtocolData(httpRes->contentlen, httpRes->data,
                           (protoParseResult*)&playlist);

    HTTP_Client_FreeResult(httpRes);

    if (playlist.totalcount != playlist.returnedcount)
        FIXME("didn't return all playlists's, need to handle split\n");

//    TRACE("returnedcount: %i\n", playlist.returnedcount);

//    sizereq = sizeof(DAAP_ClientHost_DatabasePlaylist) * playlist.returnedcount;

    for (i = 0; i < playlist.returnedcount; i++)
    {
        dmapGenericContainer *item = &(playlist.listitems[i]);
#if 0
        DMAP_INT32 buf32;
        DMAP_STRING buf;

        if (dmapGeneric_LookupContainerItem_INT32(playlist, dmap_l("itemid"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;

        if (dmapGeneric_LookupContainerItem_STRING(playlist, dmap_l("itemname"), &buf) !=
                DMAP_DATATYPE_STRING)
            continue;
        sizereq += strlen(buf) + 1;
#endif
    }
#if 0
    if (dbcontainer->playlists) free(dbcontainer->playlists);

    dbcontainer->playlists_size = sizereq;
    dbcontainer->playlists = malloc(sizereq);

    strpos = (char*)(((char*)dbcontainer->playlists) +
             (sizeof(DAAP_ClientHost_DatabasePlaylist) * playlists.returnedcount));

    j = 0;
    for (i = 0; i < playlists.returnedcount; i++)
    {
        dmapGenericContainer *playlist = &(playlists.listitems[i]);
        DAAP_ClientHost_DatabasePlaylist *dbplaylist = &(dbcontainer->playlists[j]);
        DMAP_INT32 buf32;
        DMAP_STRING itemname;

        if (dmapGeneric_LookupContainerItem_INT32(playlist, dmap_l("itemcount"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;
        dbplaylist->count = buf32;

        if (dmapGeneric_LookupContainerItem_INT32(playlist, dmap_l("itemid"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;
        dbplaylist->id = buf32;

        if (dmapGeneric_LookupContainerItem_STRING(playlist, dmap_l("itemname"), &itemname) !=
                DMAP_DATATYPE_STRING)
            continue;

        strcpy(strpos, itemname);
        dbplaylist->itemname = strpos;
        strpos += strlen(strpos)+1;

        j++;
    }

    dbcontainer->nPlaylists = j;
    TRACE("playlists: %i\n", j);

#endif
    freeGenericPreListing(&playlist);
    return 1;
}

static int Priv_DAAP_ClientHost_GetDatabasePlaylists(DAAP_SClientHost *pCHThis,
                                                     int databaseid)
{
    char hash[33] = {0};
    char playlistUrl[] = "/databases/%i/containers?session-id=%i&revision-number=%i";
    char buf[sizeof(playlistUrl) + 11 + 11 + 11];

    protoParseResult_genericPreListing playlists;

    DatabasePlaylistContainer *dbcontainer = NULL;

    HTTP_GetResult *httpRes;

    int i, j;
    char *strpos;
    int sizereq;

    for (i = 0; i < pCHThis->nDatabases; i++)
    {
        DatabasePlaylistContainer *container = &(pCHThis->dbplaylists[i]);
        if (container->id == databaseid)
        {
            dbcontainer = container;
            break;
        }
    }

    if (!dbcontainer)
    {
        ERR("container not found, returning\n");
        freeGenericPreListing(&playlists);
    }

    sprintf(buf, playlistUrl, databaseid, pCHThis->sessionid, pCHThis->revision_number);

    GenerateHash(pCHThis->version_major, buf, 2, hash, 0);

    httpRes = HTTP_Client_Get(pCHThis->connection, buf, hash, NULL, 0);

    if (!httpRes)
        return 0;

    playlists.h.expecting = QUERY_GENERICLISTING;

    dmap_parseProtocolData(httpRes->contentlen, httpRes->data,
                           (protoParseResult*)&playlists);

    HTTP_Client_FreeResult(httpRes);

    if (playlists.totalcount != playlists.returnedcount)
        FIXME("didn't return all playlists's, need to handle split\n");

    TRACE("returnedcount: %i\n", playlists.returnedcount);

    sizereq = sizeof(DAAP_ClientHost_DatabasePlaylist) * playlists.returnedcount;

    for (i = 0; i < playlists.returnedcount; i++)
    {
        dmapGenericContainer *playlist = &(playlists.listitems[i]);
        DMAP_INT32 buf32;
        DMAP_STRING buf;

        if (dmapGeneric_LookupContainerItem_INT32(playlist, dmap_l("itemcount"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;

        if (dmapGeneric_LookupContainerItem_INT32(playlist, dmap_l("itemid"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;

        if (dmapGeneric_LookupContainerItem_STRING(playlist, dmap_l("itemname"), &buf) !=
                DMAP_DATATYPE_STRING)
            continue;
        sizereq += strlen(buf) + 1;
    }

    if (dbcontainer->playlists) free(dbcontainer->playlists);

    dbcontainer->playlists_size = sizereq;
    dbcontainer->playlists = malloc(sizereq);

    strpos = (char*)(((char*)dbcontainer->playlists) +
             (sizeof(DAAP_ClientHost_DatabasePlaylist) * playlists.returnedcount));

    j = 0;
    for (i = 0; i < playlists.returnedcount; i++)
    {
        dmapGenericContainer *playlist = &(playlists.listitems[i]);
        DAAP_ClientHost_DatabasePlaylist *dbplaylist = &(dbcontainer->playlists[j]);
        DMAP_INT32 buf32;
        DMAP_STRING itemname;

        if (dmapGeneric_LookupContainerItem_INT32(playlist, dmap_l("itemcount"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;
        dbplaylist->count = buf32;

        if (dmapGeneric_LookupContainerItem_INT32(playlist, dmap_l("itemid"), &buf32) !=
                DMAP_DATATYPE_INT32)
            continue;
        dbplaylist->id = buf32;

        if (dmapGeneric_LookupContainerItem_STRING(playlist, dmap_l("itemname"), &itemname) !=
                DMAP_DATATYPE_STRING)
            continue;

        strcpy(strpos, itemname);
        dbplaylist->itemname = strpos;
        strpos += strlen(strpos)+1;

        Priv_DAAP_ClientHost_GetDatabasePlaylistItems(pCHThis, databaseid, dbplaylist->id);

        j++;
    }

    dbcontainer->nPlaylists = j;
    TRACE("playlists: %i\n", j);

    freeGenericPreListing(&playlists);

    return 1;
}


/* private create */
static DAAP_SClientHost *DAAP_ClientHost_Create(DAAP_SClient *parent, char *host,
                                                char *sharename)
{
    DAAP_SClientHost *pClientHostNew = malloc(sizeof(DAAP_SClientHost));
    memset(pClientHostNew, 0, sizeof(DAAP_SClientHost));

    /* HACK */ pClientHostNew->interrupt = 66;

    pClientHostNew->uiRef = 1;

    /* we don't hold a reference to this.. as it will always exist
     * while this object exists.
     */
    pClientHostNew->parent = parent;

    strncpy(pClientHostNew->sharename_friendly, sharename,
            sizeof(pClientHostNew->sharename_friendly) - 1);

    pClientHostNew->host = malloc(strlen(host) + 1);
    strcpy(pClientHostNew->host, host);

    pClientHostNew->prev = NULL;
    pClientHostNew->next = NULL;

    return pClientHostNew;

}

/* public */
unsigned int DAAP_ClientHost_AddRef(DAAP_SClientHost *pCHThis)
{
    return ++pCHThis->uiRef;
}

unsigned int DAAP_ClientHost_Release(DAAP_SClientHost *pCHThis)
{
    if (--pCHThis->uiRef)
        return pCHThis->uiRef;

    if (pCHThis->connection) HTTP_Client_Close(pCHThis->connection);
    if (pCHThis->databases) free(pCHThis->databases);
    if (pCHThis->dbitems)
    {
        int i;
        for (i = 0; i < pCHThis->nDatabases; i++)
        {
            free(&(pCHThis->dbitems->items[i]));
        }
        free(pCHThis->dbitems);
    }

    free(pCHThis->host);
    free(pCHThis);

    return 0;
}

unsigned int DAAP_ClientHost_GetSharename(DAAP_SClientHost *pCHThis,
                                          char *buf,
                                          int bufsize)
{
    int reqsize;

    reqsize = strlen(pCHThis->sharename_friendly) + 1;

    if (bufsize < reqsize)
        return reqsize;

    strcpy(buf, pCHThis->sharename_friendly);
    return 0;
}

unsigned int DAAP_ClientHost_Connect(DAAP_SClientHost *pCHThis)
{
    if (pCHThis->connection)
    {
        ERR("already connected? %i\n", pCHThis->interrupt);
        goto err;
    }

    TRACE("connecting to %s\n", pCHThis->host);
    pCHThis->connection = HTTP_Client_Open(pCHThis->host);
    if (!pCHThis->connection)
    {
        ERR("couldn't open connection to host\n");
        goto err;
    }
    TRACE("connected\n");

    /* do initial login etc */
    if (!Priv_DAAP_ClientHost_InitialTransaction(pCHThis))
    {
        ERR("couldn't finish initial transaction with server.\n");
        goto err;
    }
    if (!Priv_DAAP_ClientHost_GetDatabases(pCHThis))
    {
        ERR("couldn't get database list\n");
        goto err;
    }

    return 0;
err:
    if (pCHThis->connection)
    {
        HTTP_Client_Close(pCHThis->connection);
        pCHThis->connection = NULL;
    }
    return -1;
}

unsigned int DAAP_ClientHost_Disconnect(DAAP_SClientHost *pCHThis)
{
    if (!pCHThis->connection)
        return -1;

    HTTP_Client_Close(pCHThis->connection);
    pCHThis->connection = NULL;
    return 0;
}

unsigned int DAAP_ClientHost_GetDatabases(DAAP_SClientHost *pCHThis,
                                          DAAP_ClientHost_Database *buffer,
                                          int *n, int bufsize)
{
    if (!pCHThis->connection)
        return -1;

    if (bufsize < pCHThis->databases_size)
        return pCHThis->databases_size;

    memcpy(buffer, pCHThis->databases, pCHThis->databases_size);
    *n = pCHThis->nDatabases;

    return 0;
}

int DAAP_ClientHost_GetDatabaseItems(DAAP_SClientHost *pCHThis,
                                     int databaseid,
                                     DAAP_ClientHost_DatabaseItem *buffer,
                                     int *n, int bufsize)
{
    int i;

    if (!pCHThis->connection)
        return -1;

    for (i = 0; i < pCHThis->nDatabases; i++)
    {
        if (pCHThis->dbitems[i].id == databaseid)
        {
            if (bufsize < pCHThis->dbitems[i].items_size)
                return pCHThis->dbitems[i].items_size;

            memcpy(buffer, pCHThis->dbitems[i].items,
                   pCHThis->dbitems[i].items_size);
            *n = pCHThis->dbitems[i].nItems;
            return 0;
        }
    }

    return -1;
}

int DAAP_ClientHost_GetAudioFile(DAAP_SClientHost *pCHThis,
                                 int databaseid, int songid,
                                 const char *songformat,
                                 DAAP_ClientHost_Song *song)
{
    char hash[33] = {0};
    char *hashUrl;

    char songUrl_42[] = "/databases/%i/items/%i.%s?session-id=%i&revision-id=%i";
    char songUrl_45[] = "daap://%s/databases/%i/items/%i.%s?session-id=%i";
    char buf[sizeof(songUrl_45) + 11 + 11 + 11 + 11 + 4];

    char requestid_45[] = "Client-DAAP-Request-ID: %u\r\n";
    char buf_45extra[sizeof(requestid_45) + 11];
    int requestid = 0;

    HTTP_Connection *http_connection;

    if (strlen(songformat) > 4) return -1;

    HTTP_GetResult *httpRes;

    if (pCHThis->version_major != 3)
    {
        sprintf(buf, songUrl_42, databaseid, songid, songformat,
                pCHThis->sessionid, pCHThis->revision_number);
    }
    else
    {
        sprintf(buf, songUrl_45, pCHThis->host, databaseid, songid,
                songformat, pCHThis->sessionid);
        requestid = ++pCHThis->request_id;
        sprintf(buf_45extra, requestid_45, requestid);
    }

    /* dodgy hack as the hash for 4.5 needs to not include daap:// */
    if (!strstr(buf, "daap://"))
        hashUrl = buf;
    else
        hashUrl = strstr(buf, "/databases");

    GenerateHash(pCHThis->version_major, hashUrl, 2, hash, requestid);

    /* use a seperate connection */
    http_connection = HTTP_Client_Open(pCHThis->host);

    /* 1 = Connection: Close*/
    TRACE("untested\n");
    httpRes = HTTP_Client_Get(http_connection, buf, hash,
                              requestid ? buf_45extra : NULL,
                              1);

    HTTP_Client_Close(http_connection);

    if (!httpRes)
        return -1;

    song->size = httpRes->contentlen;
    song->data = malloc(httpRes->contentlen);
    memcpy(song->data, httpRes->data, httpRes->contentlen);

    HTTP_Client_FreeResult(httpRes);

    return 0;
}

int DAAP_ClientHost_FreeAudioFile(DAAP_SClientHost *pCHThis,
                                  DAAP_ClientHost_Song *song)
{
    free(song->data);
    return 0;
}

typedef struct
{
    char *url;
    char *extra_header;
    int requestid;
#if defined(SYSTEM_POSIX)
    int fileid;
#elif defined(SYSTEM_WIN32)
	HANDLE fileid;
#else
	FILE *fileid;
#endif
    int interrupt;
} GetFile;

static int httpCallback(void *pv_pCHThis, int pos)
{
    DAAP_SClientHost *pCHThis = (DAAP_SClientHost*)pv_pCHThis;
    if (pCHThis->interrupt)
    {
        return 1;
    }
    if (pCHThis->parent->pfnCallbackStatus)
        pCHThis->parent->pfnCallbackStatus(pCHThis->parent,
                                           DAAP_STATUS_downloading,
                                           pos,
                                           pCHThis->parent->pvCallbackStatusContext);
    return 0;
}

static void AsyncGetFile(void *pv_pCHThis, void *pv_pGetFile)
{
    GetFile *pGetFile = (GetFile*)pv_pGetFile;
    DAAP_SClientHost *pCHThis = (DAAP_SClientHost*)pv_pCHThis;
    HTTP_Connection *http_connection;
    int ret;

    char hash[33] = {0};
    char *hashUrl;

    /* dodgy hack as the hash for 4.5 needs to not include daap:// */
    if (!strstr(pGetFile->url, "daap://"))
        hashUrl = pGetFile->url;
    else
        hashUrl = strstr(pGetFile->url, "/databases");

    pCHThis->interrupt = 0;

    GenerateHash(pCHThis->version_major, hashUrl, 2, hash, pGetFile->requestid);

    if (pCHThis->parent->pfnCallbackStatus)
        pCHThis->parent->pfnCallbackStatus(pCHThis->parent,
                                           DAAP_STATUS_negotiating,
                                           0,
                                           pCHThis->parent->pvCallbackStatusContext);

    /* use a seperate connection */
    http_connection = HTTP_Client_Open(pCHThis->host);
    if (!http_connection) goto err;

    ret = HTTP_Client_Get_ToFile(http_connection, pGetFile->url,
                                 hash, pGetFile->extra_header, pGetFile->fileid,
                                 /* 1 = Connection: close */
                                 httpCallback, pv_pCHThis, 1);

    /* finished prematurely, and not interrupted */
    if (!ret && !pCHThis->interrupt) goto err;

    HTTP_Client_Close(http_connection);
    http_connection = NULL;

    pCHThis->interrupt = 0;

    if (pCHThis->parent->pfnCallbackStatus)
        pCHThis->parent->pfnCallbackStatus(pCHThis->parent,
                                           DAAP_STATUS_idle,
                                           0,
                                           pCHThis->parent->pvCallbackStatusContext);

    free(pGetFile);
    DAAP_ClientHost_Release(pCHThis);

    return;

err:
    if (http_connection)
        HTTP_Client_Close(http_connection);

    free(pGetFile);
    DAAP_ClientHost_Release(pCHThis);

    if (pCHThis->parent->pfnCallbackStatus)
        pCHThis->parent->pfnCallbackStatus(pCHThis->parent,
                                           DAAP_STATUS_error,
                                           0,
                                           pCHThis->parent->pvCallbackStatusContext);
}

#if !defined(SYSTEM_POSIX)
typedef struct
{
	void *arg1, *arg2;
} tsApiWrap_data;

ts_thread_cb(tsApiWrap_AsyncGetFile)
{
	tsApiWrap_data *data = arg;

	AsyncGetFile(data->arg1, data->arg2);

	free(data);

	return ts_thread_defaultret;
}
#endif

int DAAP_ClientHost_AsyncGetAudioFile(DAAP_SClientHost *pCHThis,
                                      int databaseid, int songid,
                                      const char *songformat,
#if defined(SYSTEM_POSIX)
								      int fd)
#elif defined(SYSTEM_WIN32)
								      HANDLE fd)
#else
	                                  FILE *fd)
#endif
{
    /* FIXME: aac?? */
    char songUrl_42[] = "/databases/%i/items/%i.%s?session-id=%i&revision-id=%i";
    char songUrl_45[] = "daap://%s/databases/%i/items/%i.%s?session-id=%i";

    char requestid_45[] = "Client-DAAP-Request-ID: %u\r\n";

    GetFile *pGetFile = malloc(sizeof(GetFile) + (sizeof(songUrl_45) + 11 + 11 + 11 + 11) +
                               (sizeof(requestid_45) + 11));

    pGetFile->fileid = fd;
    pGetFile->url = (char*)((char*)pGetFile + sizeof(GetFile));
    pGetFile->extra_header = NULL;

    if (pCHThis->version_major != 3)
    {
        sprintf(pGetFile->url, songUrl_42, databaseid, songid,
                songformat, pCHThis->sessionid, pCHThis->revision_number);
    }
    else
    {
        sprintf(pGetFile->url, songUrl_45, pCHThis->host, databaseid, songid,
                songformat, pCHThis->sessionid);
        pGetFile->extra_header = pGetFile->url + strlen(pGetFile->url) + 1;
        pGetFile->requestid = ++pCHThis->request_id;
        sprintf(pGetFile->extra_header, requestid_45, pGetFile->requestid);
    }

    DAAP_ClientHost_AddRef(pCHThis);

#if defined(SYSTEM_POSIX)
    CP_ThreadPool_QueueWorkItem(pCHThis->parent->tp, AsyncGetFile,
                                (void*)pCHThis, (void*)pGetFile);
#else /*if defined(SYSTEM_WIN32) */
	{
	    ts_thread thread;
	    tsApiWrap_data *data = malloc(sizeof(tsApiWrap_data));
	    data->arg1 = (void*)pCHThis;
	    data->arg2 = (void*)pGetFile;
        ts_thread_create(thread, tsApiWrap_AsyncGetFile, data);
		ts_thread_close(thread);
	}
#endif

    return 0;
}

int DAAP_ClientHost_AsyncStop(DAAP_SClientHost *pCHThis)
{
    pCHThis->interrupt = 1;
    return 0;
}


/********** update watcher ***********/

static void update_watch_cb(void *pv_pCHThis)
{
    DAAP_SClientHost *pCHThis = (DAAP_SClientHost*)pv_pCHThis;
    FIXME("got an update from host %p (%s). Expect brokenness!\n",
            pCHThis, pCHThis->sharename_friendly);
    /* FIXME: since we don't handle updates just yet, we'll remove 
     * ourselves from the update watch.
     * This will result in 505s as the iTunes times out waiting for us.
     */
    HTTP_Client_WatchQueue_RemoveUpdateWatch(pCHThis->parent->update_watch,
                                             pCHThis->connection);
}

static void AsyncWaitUpdate(void *pv_pCHThis, void *unused)
{
    DAAP_SClientHost *pCHThis = (DAAP_SClientHost*)pv_pCHThis;
    TRACE("()\n");
    char hash[33] = {0};
    char updateUrl[] = "/update?session-id=%i&revision-number=%i&delta=%i";
    char buf[sizeof(updateUrl) + 11 + 11 + 11];

    HTTP_GetResult *httpRes;

    sprintf(buf, updateUrl, pCHThis->sessionid, pCHThis->revision_number,
            pCHThis->revision_number);

    GenerateHash(pCHThis->version_major, buf, 2, hash, 0);

    /* should return pretty quickly */
    HTTP_Client_WatchQueue_AddUpdateWatch(pCHThis->parent->update_watch,
                                          pCHThis->connection, buf, hash,
                                          update_watch_cb,
                                          pv_pCHThis);
}

static void update_watch_runloop(void *pv_pUpdateWatch, void *unused)
{
    HTTP_ConnectionWatch *watch = (HTTP_ConnectionWatch*)pv_pUpdateWatch;
    HTTP_Client_WatchQueue_RunLoop(watch);
}

int DAAP_ClientHost_AsyncWaitUpdate(DAAP_SClientHost *pCHThis)
{
    /* lazy create update_watch */
    ts_mutex_lock(pCHThis->parent->mtObjectLock);
    if (!pCHThis->parent->update_watch)
    {
        pCHThis->parent->update_watch = HTTP_Client_WatchQueue_New();
        if (!pCHThis->parent->update_watch)
        {
            ERR("couldn't create update watch\n");
            return 1;
        }
        /* and now run it in a new thread */
#if defined(SYSTEM_POSIX)
        CP_ThreadPool_QueueWorkItem(pCHThis->parent->tp, update_watch_runloop,
                                    (void*)pCHThis->parent->update_watch, NULL);
#else
#error implement me
#endif
    }
    ts_mutex_unlock(pCHThis->parent->mtObjectLock);
    TRACE("about to call async wait update\n");
#if defined(SYSTEM_POSIX)
    /* doesn't _really_ need to be done in a different thread, but
     * the HTTP GET could take a little while, and then AddUpdateWatch will return
     */
    TRACE("calling\n");
    CP_ThreadPool_QueueWorkItem(pCHThis->parent->tp, AsyncWaitUpdate,
                                (void*)pCHThis, NULL);
#else
#error please implement
#endif
    return 0;
}

int DAAP_ClientHost_AsyncStopUpdate(DAAP_SClientHost *pCHThis)
{
    /* that's naughty, the app called this without installing a watch */
    if (!pCHThis->parent->update_watch) return 0;

    HTTP_Client_WatchQueue_RemoveUpdateWatch(pCHThis->parent->update_watch,
                                             pCHThis->connection);
    return 0;
}

