//
//  QSiTunesSelectionSource.m
//  iTunesModule
//
//  Created by Rob McBroom on 2013/07/30.
//
//

#import "QSiTunesSelectionSource.h"
#import "QSiTunesDefines.h"
#import "QSiTunesDatabase.h"

#define QSCurrentSelectionID @"Current iTunes Selection"

@implementation QSiTunesSelectionSource

- (id)init
{
    self = [super init];
    if (self) {
        iTunes = [QSiTunes() retain];
    }
    return self;
}

- (void)dealloc
{
    [iTunes release];
    [super dealloc];
}

- (id)resolveProxyObject:(id)proxy
{
	if (![iTunes isRunning]) {
		return nil;
	}
	if (proxy && ![proxy isKindOfClass:[NSString class]]) {
        proxy = [proxy identifier];
    }
    // if we're here via "Current Selection", proxy will be `nil`
    // if via Current iTunes Selection, proxy will be iTunes' bundle ID
    // behavior should be the same either way
    if (!proxy || [proxy isEqualToString:@"com.apple.iTunes"]) {
        proxy = QSCurrentSelectionID;
    }
    
    if ([proxy isEqualToString:QSCurrentSelectionID]) {
        // see if it's one or more tracks
        NSMutableArray *objects = [NSMutableArray array];
        NSArray *tracks = [[iTunes selection] get];
        NSString *trackID = nil;
        NSDictionary *trackInfo = nil;
        // you have to iterate through this - valueForKey/arrayByPerformingSelector won't work
        for (iTunesFileTrack *track in tracks) {
            trackID = [NSString stringWithFormat:@"%ld", (long)[track databaseID]];
            trackInfo = [[QSiTunesDatabase sharedInstance] trackInfoForID:trackID];
            [objects addObject:[[QSiTunesDatabase sharedInstance] trackObjectForInfo:trackInfo inPlaylist:nil]];
        }
        if ([objects count]) {
            return [QSObject objectByMergingObjects:objects];
        }
    }

    // no tracks selected, or selected playlist was explicitly requested
    if ([proxy isEqualToString:@"QSSelectedPlaylistProxy"] || [proxy isEqualToString:QSCurrentSelectionID]) {
        // see if the seleciton is a playlist
        iTunesBrowserWindow *window = [[iTunes browserWindows] objectAtIndex:0];
        NSString *name = [[window view] name];
        NSDictionary *thisPlaylist = [[QSiTunesDatabase sharedInstance] playlistInfoForName:name];
        if (thisPlaylist) {
            QSObject *newObject = [QSObject objectWithName:name];
            [newObject setObject:[thisPlaylist objectForKey:@"Playlist ID"] forType:QSiTunesPlaylistIDPboardType];
            [newObject setIdentifier:[thisPlaylist objectForKey:@"Playlist Persistent ID"]];
            [newObject setPrimaryType:QSiTunesPlaylistIDPboardType];
            return newObject;
        }
    }
    return nil;
}

@end
