//
//  QSiTunesMonitor.h
//  Quicksilver
//
//  Created by Nicholas Jitkoff on 7/3/04.
//  Copyright 2004 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


#define iTunesTrackChangedNotification @"iTunesTrackChanged"
#define iTunesStateChangedNotification @"iTunesStateChanged"
int iTunesMonitorPID();
BOOL iTunesMonitorActivate(NSString *monitorPath);
BOOL iTunesMonitorDeactivate();