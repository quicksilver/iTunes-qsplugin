//
//  QSiTunesBridge.h
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "iTunes.h"

extern NSAppleScript *gQSiTunesScript;

NSAppleScript *QSiTunesScript();
iTunesApplication *QSiTunes();
iTunesSource *QSiTunesLibrary();
