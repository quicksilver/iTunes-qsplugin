//
//  QSiTunesActionProvider.h
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "QSiTunesDefines.h"

@interface QSiTunesActionProvider : QSActionProvider

@property iTunesApplication *iTunes;

- (void)playPlaylist:(QSObject *)dObject;
- (void)playBrowser:(QSObject *)dObject;
- (void)playTrack:(QSObject *)dObject append:(BOOL)append;
- (void)playUsingDynamicPlaylist:(NSArray *)trackList;
- (NSArray *)trackObjectsFromQSObject:(QSObject *)tracks;
- (iTunesPlaylist *)playlistObjectFromQSObject:(QSObject *)playlist;
@end

@interface QSiTunesControlProvider : QSActionProvider

@property iTunesApplication *iTunes;

- (void)play;
- (void)pause;
- (void)togglePlayPause;
- (void)stop;
- (void)next;
- (void)previous;
- (void)volumeIncrease;
- (void)volumeDecrease;
- (void)volumeMute;
- (void)ratingIncrease;
- (void)ratingDecrease;
- (void)rating0;
- (void)rating1;
- (void)rating2;
- (void)rating3;
- (void)rating4;
- (void)rating5;
- (void)equalizerToggle;
@end
