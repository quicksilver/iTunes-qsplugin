//
//  QSiTunesActionProvider.m
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "QSiTunesActionProvider.h"

//#import <QSCore/QSMacros.h>


#define kQSiTunesPlayItemAction @"QSiTunesPlayItemAction"
//#define kQSiTunesAppendItemAction @"QSiTunesAppendItemAction"


#define kQSiTunesPSPlayAction @"QSiTunesPSPlayAction"
#define kQSiTunesPSPlayNextAction @"QSiTunesPSPlayNextAction"
#define kQSiTunesPSAddAction @"QSiTunesPSAddAction"

@implementation QSiTunesActionProvider
- (id)init {
	if (self = [super init]) {
		iTunesScript = nil;
		iTunes = nil;
	}
	return self;
}


- (NSArray *)validActionsForDirectObject:(QSObject *)dObject indirectObject:(QSObject *)iObject {
	if ([dObject objectForType:QSiTunesPlaylistIDPboardType]) {
		return [NSArray arrayWithObject:kQSiTunesPlayItemAction];
	} else if ([dObject objectForType:QSiTunesBrowserPboardType]) {
		NSDictionary *browseDict = [dObject objectForType:QSiTunesBrowserPboardType];
		if (![browseDict objectForKey:@"Criteria"])
			return nil;
	}
	return [NSArray arrayWithObjects:kQSiTunesPSPlayNextAction, kQSiTunesPSAddAction, kQSiTunesPSPlayAction, kQSiTunesPlayItemAction, nil];
}

- (QSObject *)playItem:(QSObject *)dObject { 
	if ([dObject containsType:QSiTunesPlaylistIDPboardType]) {
		[self playPlaylist:dObject];
	} else if ([dObject containsType:QSiTunesBrowserPboardType]) {
		[self playBrowser:dObject  party:NO append:NO next:NO];
	} else if ([dObject containsType:QSiTunesTrackIDPboardType]) {
		[self playTrack:dObject party:NO append:NO next:NO];
	}
	return nil;
}


- (QSObject *)psPlayItem:(QSObject *)dObject {
	if ([dObject containsType:QSiTunesBrowserPboardType]) [self playBrowser:dObject party:YES append:NO next:NO];
	if ([dObject containsType:QSiTunesTrackIDPboardType]) [self playTrack:dObject party:YES append:NO next:NO];
	return nil;
}
- (QSObject *)psAppendItem:(QSObject *)dObject {
	if ([dObject containsType:QSiTunesBrowserPboardType]) [self playBrowser:dObject party:YES append:YES next:NO];
	if ([dObject containsType:QSiTunesTrackIDPboardType]) [self playTrack:dObject party:YES append:YES next:NO];
	return nil;
}
- (QSObject *)psPlayNextItem:(QSObject *)dObject {
	if ([dObject containsType:QSiTunesBrowserPboardType]) [self playBrowser:dObject party:YES append:NO next:YES];
	if ([dObject containsType:QSiTunesTrackIDPboardType]) [self playTrack:dObject party:YES append:NO next:YES];
	return nil;
}

/*
 - (QSObject *)appendItem:(QSObject *)dObject { 
	 if ([dObject objectForType:QSiTunesBrowserPboardType])
		 [self playBrowser:dObject append:YES];
	 if ([dObject objectForType:QSiTunesTrackIDPboardType])
		 [self playTrack:dObject append:YES];
	 return nil;
 }
 */

- (QSObject *)playBrowser:(QSObject *)dObject party:(BOOL)party append:(BOOL)append next:(BOOL)next {
	
	NSDictionary *browseDict = [dObject objectForType:QSiTunesBrowserPboardType];
	NSMutableDictionary *criteriaDict = [[[browseDict objectForKey:@"Criteria"] mutableCopy] autorelease];
	
	if ([[criteriaDict objectForKey:@"Artist"] isEqualToString:COMPILATION_STRING])
		[criteriaDict removeObjectForKey:@"Artist"];
	
	NSMutableArray *criteria = [NSMutableArray arrayWithArray:[criteriaDict objectsForKeys:[NSArray arrayWithObjects:@"Genre", @"Artist", @"Composer", @"Album", nil]
																		  notFoundMarker:[NSAppleEventDescriptor descriptorWithTypeCode:'msng']]];
	NSDictionary *errorDict = nil;
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesFastBrowserPlay"] && [browseDict objectForKey:@"Type"])
		[criteria addObject:[browseDict objectForKey:@"Type"]];
	else
		[criteria addObject:[NSAppleEventDescriptor descriptorWithTypeCode:'msng']];
	
	
	if (party) {
		if (next) {
			[[self iTunesScript] executeSubroutine:@"ps_play_next_criteria" arguments:criteria  error:&errorDict];
		} else if (append) {
			[[self iTunesScript] executeSubroutine:@"ps_add_criteria" arguments:criteria  error:&errorDict];
		} else {
			[[self iTunesScript] executeSubroutine:@"ps_play_criteria" arguments:criteria  error:&errorDict];
		}
	} else {
		[[self iTunesScript] executeSubroutine:(append?@"append_with_criteria":@"play_with_criteria") arguments:criteria  error:&errorDict];
	}
	
	
	if (errorDict) {NSLog(@"Error: %@", errorDict);}
	return nil;
}

- (QSObject *)playTrack:(QSObject *)dObject party:(BOOL)party append:(BOOL)append next:(BOOL)next {
	//NSArray *trackIDs = [[dObject arrayForType:QSiTunesTrackIDPboardType] valueForKey:@"Track ID"];
	NSArray *paths = [dObject validPaths];
	
	if (!paths) return nil;
	
	//return;
	NSDictionary *errorDict = nil;
	
	//	[NSAppleEventDescriptor aliasDescriptorWithFile:[paths objectAtIndex:
	if (party) {
		if (next) {
			//ps_play_next_track can handle a list
			[[self iTunesScript] executeSubroutine:@"ps_play_next_track" 
										 arguments:[NSArray arrayWithObject:[NSAppleEventDescriptor aliasListDescriptorWithArray:paths]] 
											 error:&errorDict];

		} else if (append) {
				[[self iTunesScript] executeSubroutine:@"ps_add_track" 
											 arguments:[NSArray arrayWithObject:[NSAppleEventDescriptor aliasListDescriptorWithArray:paths]] 
												 error:&errorDict];

		} else {
			// Play first track, queue rest
			[[self iTunesScript] executeSubroutine:@"ps_play_track" 
										 arguments:[NSArray arrayWithObject:[NSAppleEventDescriptor aliasListDescriptorWithArray:paths]]
											 error:&errorDict];

			//	if (count > 1) {
			//				//ps_play_next_track can handle a list		
			//				[[self iTunesScript] executeSubroutine:@"ps_play_next_track" 
			//											 arguments:[NSArray arrayWithObject:[NSAppleEventDescriptor aliasListDescriptorWithArray:[paths subarrayWithRange:NSMakeRange(1, count-1)]]]
			//												 error:&errorDict];
			//			}
		}
	} else if (append) {
			[[self iTunesScript] executeSubroutine:@"append_track"
										 arguments:[NSArray arrayWithObject:[NSAppleEventDescriptor aliasDescriptorWithFile:[paths lastObject]]] 
											 error:&errorDict];

	} else {
		NSString *playlist = [dObject objectForMeta:@"QSiTunesSourcePlaylist"];
		if (playlist) {
			[[self iTunesScript] executeSubroutine:@"play_track_in_playlist"
										 arguments:[NSArray arrayWithObjects:
											 [NSAppleEventDescriptor aliasListDescriptorWithArray:paths] ,
											 playlist, nil]
											 error:&errorDict];

		} else {
			iTunesLibraryPlaylist *libraryPlaylist = [[[[[self iTunes] sources] objectAtIndex:0] libraryPlaylists] objectAtIndex:0];
			NSDictionary *trackInfo = [[dObject arrayForType:QSiTunesTrackIDPboardType] lastObject];
			NSString *trackID = [trackInfo objectForKey:@"Persistent ID"];
			NSArray *trackResult = [[libraryPlaylist fileTracks] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", trackID]];
			if ([trackResult count] > 0) {
				[[trackResult lastObject] playOnce:YES];
			}
		}
	}
	if (errorDict) {NSLog(@"Error: %@", errorDict);}
	return nil;
}

- (QSObject *)playPlaylist:(QSObject *)dObject
{
	NSString *playlistID = [dObject identifier];
	iTunesSource *library = [[[self iTunes] sources] objectAtIndex:0];
	NSArray *playlistResult = [[library playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", playlistID]];
	if ([playlistResult count] > 0) {
		[[playlistResult lastObject] playOnce:YES];
	}
	return nil;
}

- (iTunesApplication *)iTunes
{
//	if (!iTunes) {
//		iTunes = [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
//	}
//	return iTunes;
	return [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
}

- (NSAppleScript *)iTunesScript {
	if (!iTunesScript)
		iTunesScript = [[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:
			[[NSBundle bundleForClass:[QSiTunesActionProvider class]]  pathForResource:@"iTunes" ofType:@"scpt"]] error:nil];
	return iTunesScript;
}

@end
