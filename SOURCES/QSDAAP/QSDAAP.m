#import <Foundation/Foundation.h>
#import "client.h"

#import "daap.h"
struct DAAP_SClientHostTAG
{
    unsigned int uiRef;
	
    DAAP_SClient *parent;
	
    char *host; /* FIXME: use an address container (IPv4 vs IPv6) */
    void *connection;
	
    char sharename_friendly[1005];
    char sharename[1005]; /* from mDNS */
	
    /* dmap/daap fields */
    int sessionid;
    int revision_number;
	
    int request_id;
	
    short version_major;
    short version_minor;
	
    int nDatabases;
    int databases_size;
    DAAP_ClientHost_Database *databases;
	
    void *dbitems;
    void *dbplaylists;
	
    int interrupt;
	
    DAAP_SClientHost *prev;
    DAAP_SClientHost *next;
	
    int marked; /* used for discover cb */
};
/*************** Status Callback from libopendaap *******/
static void DAAP_StatusCB( DAAP_SClient *client, DAAP_Status status,
                           int pos,  void* context)
{
	NSLog(@"status %d",status);
}

static int cb_enum_hosts( DAAP_SClient *client,
                         DAAP_SClientHost *host,
                          void *ctx)
{
 
	//NSLog(@"host %s",host->sharename_friendly);
	DAAP_ClientHost_Connect(host);
    return 1;
}


int main (int argc, const char * argv[]) {

	// init_daap(&argc, &argv);
	DAAP_SClient *client=DAAP_Client_Create(DAAP_StatusCB, NULL);
	 DAAP_Client_EnumerateHosts(client,
											cb_enum_hosts,
											NULL);
	DAAP_SClientHost *host=DAAP_Client_AddHost(client, "localhost","Baphomet", "Baphomet");
	DAAP_ClientHost_Connect(host);
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    // insert code here...
    NSLog(@"Hello, World!");
    [pool release];
    return 0;
}
