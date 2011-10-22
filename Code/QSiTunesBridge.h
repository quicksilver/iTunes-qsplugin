//
//  QSiTunesBridge.h
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

extern NSAppleScript *gQSiTunesScript;

NSAppleScript *QSiTunesScript();


long QSiTunesCurrentTrackID();
NSDictionary *QSiTunesFetchInfoForID(NSNumber *theID);