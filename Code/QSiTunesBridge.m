//
//  QSiTunesBridge.m
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "QSiTunesBridge.h"
//#import <QSFoundation/NSAppleScript_BLTRExtensions.h>


NSAppleScript *gQSiTunesScript = nil;

NSAppleScript *QSiTunesScript() {
	if (!gQSiTunesScript) {
			gQSiTunesScript = [[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:
				[[NSBundle bundleForClass:NSClassFromString(@"QSiTunesObjectSource")]  pathForResource:@"iTunes" ofType:@"scpt"]] error:nil];

	}
			return gQSiTunesScript;
}


long QSiTunesCurrentTrackID() {
	static AppleEvent trackEvent = {typeNull, 0} ;
	if (trackEvent.descriptorType == typeNull) {		
		OSType adrITunes = 'hook';
		AEBuildAppleEvent ('core', 'getd', typeApplSignature, &adrITunes, sizeof(adrITunes) , kAutoGenerateReturnID, kAnyTransactionID, &trackEvent, NULL,
						   "'----':'obj ' {form:'prop', want:type('prop') , seld:type('pDID'), from:'obj ' {form:'prop', want:type('prop'), seld:type('pTrk'), from:'null'() } } ");
	}
	unsigned long trackID = 0;
	AppleEvent reply; 	   
	// Send Track ID Event
	OSStatus err = AESend(&trackEvent, &reply, kAEWaitReply, kAENormalPriority, kNoTimeOut, NULL, NULL);
	if (err) return 0;
	AEGetParamPtr(&reply, keyDirectObject, cLongInteger, NULL, &trackID, sizeof(trackID) , NULL);
	AEDisposeDesc(&reply);
	return trackID;
}

NSDictionary *QSiTunesFetchInfoForID(NSNumber *theID) {
NSLog(@"Fetch Info For %@", theID);
	NSDictionary *error = nil;
	NSAppleEventDescriptor *info = [QSiTunesScript() executeSubroutine:@"extract_track_data" arguments:theID  error:&error];
	
	
	if (error) return nil;
	
	NSDictionary *dict = [info objectValue];
	
	//NSLog(@"Fetch Info For %@", [dict description]);
	NSString *name = [dict objectForKey:[NSNumber numberWithUnsignedLong:'pnam']];
	NSString *artist = [dict objectForKey:[NSNumber numberWithUnsignedLong:'pArt']];
	NSString *album = [dict objectForKey:[NSNumber numberWithUnsignedLong:'pAlb']];
	NSString *location = [dict objectForKey:[NSNumber numberWithUnsignedLong:'pLoc']];
	NSString *kind = [dict objectForKey:[NSNumber numberWithUnsignedLong:'pKnd']];
	NSNumber *rating = [dict objectForKey:[NSNumber numberWithUnsignedLong:'pRte']];
	
	if ([location isKindOfClass:[NSURL class]]) location = [(NSURL *)location absoluteString];
	
	//NSLog(@"loc %@ %d", location, 'msng');
	if ([location isKindOfClass:[NSAppleEventDescriptor class]]) location = nil;
	if ([album isKindOfClass:[NSAppleEventDescriptor class]]) album = nil;
	if ([artist isKindOfClass:[NSAppleEventDescriptor class]]) artist = nil;
	if ([kind isKindOfClass:[NSAppleEventDescriptor class]]) kind = nil;
	if ([rating isKindOfClass:[NSAppleEventDescriptor class]]) rating = nil;
	//if ([location isKindOfClass:[NSArray class]]) location = [location lastObject];
	
	NSMutableDictionary *trackInfo = [NSMutableDictionary dictionary];
	if (name) [trackInfo setObject:name forKey:@"Name"];
	if (location) [trackInfo setObject:location forKey:@"Location"];
	if (album) [trackInfo setObject:album forKey:@"Album"];
	if (artist) [trackInfo setObject:artist forKey:@"Artist"];
	if (rating) [trackInfo setObject:rating forKey:@"Rating"];
	if (kind) [trackInfo setObject:kind forKey:@"Kind"];
	[trackInfo setObject:[NSNumber numberWithInt:1] forKey:@"Artwork Count"];
	//if (fDEV) NSLog(@"info %@", trackInfo);
	return trackInfo;
}

iTunesApplication *QSiTunes()
{
	return [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
}

iTunesSource *QSiTunesLibrary()
{
	NSArray *librarySource = [[QSiTunes() sources] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"kind == %@", [NSAppleEventDescriptor descriptorWithTypeCode:iTunesESrcLibrary]]];
	if ([[librarySource lastObject] exists]) {
		return [librarySource lastObject];
	}
	return nil;
}
