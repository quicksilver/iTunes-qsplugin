//
//  QSiTunesActionProvider.h
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "QSiTunesDefines.h"

@interface QSiTunesActionProvider : QSActionProvider{
    NSAppleScript *iTunesScript;
	iTunesApplication *iTunes;
}
- (iTunesApplication *)iTunes;
- (NSAppleScript *)iTunesScript;
- (QSObject *) playPlaylist:(QSObject *)dObject;
- (QSObject *) playBrowser:(QSObject *)dObject party:(BOOL)party append:(BOOL)append next:(BOOL)next;
- (QSObject *) playTrack:(QSObject *)dObject party:(BOOL)party append:(BOOL)append next:(BOOL)next;
@end
