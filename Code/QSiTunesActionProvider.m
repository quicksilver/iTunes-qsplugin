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

//- (QSObject *)playBrowser:(QSObject *)dObject party:(BOOL)party append:(BOOL)append next:(BOOL)next {
//	
//	NSDictionary *browseDict = [dObject objectForType:QSiTunesBrowserPboardType];
//	NSMutableDictionary *criteriaDict = [[[browseDict objectForKey:@"Criteria"] mutableCopy] autorelease];
//	
//	if ([[criteriaDict objectForKey:@"Artist"] isEqualToString:COMPILATION_STRING])
//		[criteriaDict removeObjectForKey:@"Artist"];
//	
//	NSMutableArray *criteria = [NSMutableArray arrayWithArray:[criteriaDict objectsForKeys:[NSArray arrayWithObjects:@"Genre", @"Artist", @"Composer", @"Album", nil]
//																		  notFoundMarker:[NSAppleEventDescriptor descriptorWithTypeCode:'msng']]];
//	NSDictionary *errorDict = nil;
//	
//	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesFastBrowserPlay"] && [browseDict objectForKey:@"Type"])
//		[criteria addObject:[browseDict objectForKey:@"Type"]];
//	else
//		[criteria addObject:[NSAppleEventDescriptor descriptorWithTypeCode:'msng']];
//	
//	
//	if (party) {
//		if (next) {
//			[[self iTunesScript] executeSubroutine:@"ps_play_next_criteria" arguments:criteria  error:&errorDict];
//		} else if (append) {
//			[[self iTunesScript] executeSubroutine:@"ps_add_criteria" arguments:criteria  error:&errorDict];
//		} else {
//			[[self iTunesScript] executeSubroutine:@"ps_play_criteria" arguments:criteria  error:&errorDict];
//		}
//	} else {
//		[[self iTunesScript] executeSubroutine:(append?@"append_with_criteria":@"play_with_criteria") arguments:criteria  error:&errorDict];
//	}
//	
//	
//	if (errorDict) {NSLog(@"Error: %@", errorDict);}
//	return nil;
//}

- (void)playBrowser:(QSObject *)dObject party:(BOOL)party append:(BOOL)append next:(BOOL)next
{
	NSMutableArray *formatStrings = [NSMutableArray arrayWithCapacity:1];
	NSDictionary *browseDict;
	NSDictionary *criteriaDict;
	NSString *formatString;
	NSMutableArray *criteria = [NSMutableArray arrayWithCapacity:2];
	BOOL first;
	for (QSObject *browseResult in [dObject splitObjects]) {
		browseDict = [browseResult objectForType:QSiTunesBrowserPboardType];
		criteriaDict = [browseDict objectForKey:@"Criteria"];
		
		// build a search query
		/*
		 // Adding kind and videoKind to the predicate seems like the right way to filter, but
		 while it will work, it's *very* slow for some reason. Better to just enumerate the result later.
		 NSMutableArray *criteria = [NSMutableArray arrayWithObjects:@"kind", @"PDF document", @"videoKind", [NSAppleEventDescriptor descriptorWithTypeCode:iTunesEVdKNone], nil];
		 NSString *formatString = @"%K != %@ AND %K == %@";
		 */
		formatString = @"(";
		first = YES;
		for (NSString *criteriaKey in [criteriaDict allKeys]) {
			if ([criteriaKey isEqualToString:@"Artist"] && [[criteriaDict objectForKey:@"Artist"] isEqualToString:COMPILATION_STRING]) {
				// don't use "Compilations" as a criteria
				continue;
			}
			if ([criteriaDict objectForKey:criteriaKey]) {
				[criteria addObject:[criteriaKey lowercaseString]];
				[criteria addObject:[criteriaDict objectForKey:criteriaKey]];
				if (first) {
					formatString = [formatString stringByAppendingString:@"%K == %@"];
					first = NO;
				} else {
					formatString = [formatString stringByAppendingString:@" AND %K == %@"];
				}
			}
		}
		formatString = [formatString stringByAppendingString:@")"];
		[formatStrings addObject:formatString];
	}
	formatString = [formatStrings componentsJoinedByString:@" OR "];
	NSPredicate *trackFilter = [NSPredicate predicateWithFormat:formatString argumentArray:criteria];
	iTunesLibraryPlaylist *libraryPlaylist = [[QSiTunesLibrary() libraryPlaylists] objectAtIndex:0];
	// TODO see if we can get the results to be in the same order as the objects passed in
	NSArray *tracksToPlay = [[libraryPlaylist fileTracks] filteredArrayUsingPredicate:trackFilter];
	if (party) {
		// iTunes DJ stuff
		if (next) {
			// play next
		} else if (append) {
			// append to iTunes DJ
		} else {
			// play
		}
	} else {
		[self playUsingDynamicPlaylist:tracksToPlay];
	}
}

- (QSObject *)playTrack:(QSObject *)dObject party:(BOOL)party append:(BOOL)append next:(BOOL)next {
	//NSArray *trackIDs = [[dObject arrayForType:QSiTunesTrackIDPboardType] valueForKey:@"Track ID"];
	NSArray *paths = [dObject validPaths];
	
	if (!paths) return nil;
	
	//return;
	NSDictionary *errorDict = nil;
	
	// get iTunesTrack objects to represent each track
	NSString *searchFilter = @"persistentID == %@";
	NSArray *trackIDs = [[dObject splitObjects] arrayByPerformingSelector:@selector(identifier)];
	NSMutableArray *filters = [NSMutableArray arrayWithCapacity:[dObject count]];
	for (int i = 0; i < [dObject count]; i++) {
		[filters addObject:searchFilter];
	}
	searchFilter = [filters componentsJoinedByString:@" OR "];
	iTunesLibraryPlaylist *libraryPlaylist = [[QSiTunesLibrary() libraryPlaylists] objectAtIndex:0];
	NSArray *trackResult = [[libraryPlaylist fileTracks] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:searchFilter argumentArray:trackIDs]];
	NSArray *newTracks = [trackResult arrayByPerformingSelector:@selector(location)];
	
	if (party) {
		iTunesApplication *iTunes = QSiTunes();
		iTunesPlaylist *iTunesDJ = QSiTunesDJ();
		if (next) {
			//ps_play_next_track can handle a list
			[[self iTunesScript] executeSubroutine:@"ps_play_next_track" 
										 arguments:[NSArray arrayWithObject:[NSAppleEventDescriptor aliasListDescriptorWithArray:paths]] 
											 error:&errorDict];
			// TODO get this to work via Scripting Bridge?
//			if (![[[iTunes currentTrack] container] isEqualTo:iTunesDJ]) {
//				// iTunes DJ wasn't already playing - start it
//				[iTunesDJ playOnce:YES];
//			}
//			NSInteger currentIndex = [[iTunes currentTrack] index];
//			// remove the old tracks from the end of the list
//			NSPredicate *oldFilter = [NSPredicate predicateWithFormat:@"index > %d AND index <= %d", currentIndex, [[iTunesDJ tracks] count]];
//			NSArray *oldTracks = [[iTunesDJ tracks] filteredArrayUsingPredicate:oldFilter];
//			[[iTunesDJ tracks] removeObjectsInArray:oldTracks];
//			// add the new tracks
//			[iTunes add:newTracks to:iTunesDJ];
//			// put the old tracks back
//			[iTunes add:[oldTracks arrayByPerformingSelector:@selector(location)] to:iTunesDJ]; // location only works on iTunesFileTracks, which these aren't :-(
		} else if (append) {
			[iTunes add:newTracks to:iTunesDJ];
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
		iTunesPlaylist *qs = [[QSiTunesLibrary() userPlaylists] objectWithName:QSiTunesDynamicPlaylist];
		[QSiTunes() add:newTracks to:qs];
	} else {
		NSString *playlist = [dObject objectForMeta:@"QSiTunesSourcePlaylist"];
		if (playlist) {
			[[self iTunesScript] executeSubroutine:@"play_track_in_playlist"
										 arguments:[NSArray arrayWithObjects:
											 [NSAppleEventDescriptor aliasListDescriptorWithArray:paths] ,
											 playlist, nil]
											 error:&errorDict];

		} else {
			if ([trackResult count] == 1) {
				// for a single track, just play
				[[trackResult lastObject] playOnce:YES];
			} else {
				// for multiple tracks, create a playlist and play
				[self playUsingDynamicPlaylist:trackResult];
			}
		}
	}
	if (errorDict) {NSLog(@"Error: %@", errorDict);}
	return nil;
}

- (void)playUsingDynamicPlaylist:(NSArray *)trackList
{
	iTunesApplication *iTunes = QSiTunes();
	iTunesSource *library = QSiTunesLibrary();
	iTunesPlaylist *qs = [[library userPlaylists] objectWithName:QSiTunesDynamicPlaylist];
	if (![qs exists]) {
		// create the dynamic playlist
		qs = [[[iTunes classForScriptingClass:@"playlist"] alloc] init];
		[[library userPlaylists] insertObject:qs atIndex:0];
		[qs setName:QSiTunesDynamicPlaylist];
	} else {
		// clear the old list
		[[qs tracks] removeAllObjects];
	}
	// filter out PDFs and videos
	NSMutableArray *songsOnly = [NSMutableArray arrayWithCapacity:[trackList count]];
	for (iTunesFileTrack *track in trackList) {
		if ([[track kind] isEqualToString:QSiTunesBookletKind]) {
			continue;
		}
		// TODO add a preference to include videos
		// TODO if playlist is *only* videos, include them regardless of preferences
		if ([track videoKind] != iTunesEVdKNone) {
			continue;
		}
		[songsOnly addObject:track];
	}
	[iTunes add:[songsOnly arrayByPerformingSelector:@selector(location)] to:qs];
	//[iTunes add:[trackList arrayByPerformingSelector:@selector(location)] to:qs];
	[qs playOnce:YES];
}

- (QSObject *)playPlaylist:(QSObject *)dObject
{
	NSString *playlistID = [dObject identifier];
	iTunesSource *library = QSiTunesLibrary();
	NSArray *playlistResult = [[library playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", playlistID]];
	if ([playlistResult count] > 0) {
		[[playlistResult lastObject] playOnce:YES];
	}
	return nil;
}

- (NSAppleScript *)iTunesScript {
	if (!iTunesScript)
		iTunesScript = [[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:
			[[NSBundle bundleForClass:[QSiTunesActionProvider class]]  pathForResource:@"iTunes" ofType:@"scpt"]] error:nil];
	return iTunesScript;
}

@end
