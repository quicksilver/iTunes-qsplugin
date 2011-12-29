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
		iTunes = [QSiTunes() retain];
	}
	return self;
}

- (void)dealloc {
    [iTunes release];
    [super dealloc];
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

- (void)playBrowser:(QSObject *)dObject party:(BOOL)party append:(BOOL)append next:(BOOL)next
{
	NSArray *tracksToPlay = [self trackObjectsFromQSObject:dObject];
	if (party) {
		// iTunes DJ stuff
		iTunesPlaylist *iTunesDJ = QSiTunesDJ();
		NSDictionary *errorDict = nil;
		NSDictionary *browseDict;
		NSDictionary *criteriaDict;
		for (QSObject *browseResult in [dObject splitObjects]) {
			browseDict = [browseResult objectForType:QSiTunesBrowserPboardType];
			criteriaDict = [browseDict objectForKey:@"Criteria"];
		}
		NSDictionary *ascriteria = [NSMutableArray arrayWithArray:[criteriaDict objectsForKeys:[NSArray arrayWithObjects:@"Genre", @"Artist", @"Composer", @"Album", @"Fail Intentionally", nil] notFoundMarker:[NSAppleEventDescriptor descriptorWithTypeCode:'msng']]];
		//NSLog(@"criteria for AppleScript: %@", ascriteria);
		NSArray *newTracks = [tracksToPlay valueForKey:@"location"];
		if (next) {
			// play next
			[[self iTunesScript] executeSubroutine:@"ps_play_next_criteria" arguments:ascriteria  error:&errorDict];
		} else if (append) {
			// append to iTunes DJ
			[iTunes add:newTracks to:iTunesDJ];
		} else {
			// play
			[[self iTunesScript] executeSubroutine:@"ps_play_criteria" arguments:ascriteria  error:&errorDict];
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
	NSArray *trackResult = [self trackObjectsFromQSObject:dObject];
	NSArray *newTracks = [trackResult valueForKey:@"location"];
	
	if (party) {
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
//			//iTunesItem *t = [[iTunesDJ tracks] lastObject]; [t delete];
//			//[[iTunesDJ tracks] removeObjectsInArray:oldTracks];
//			[oldTracks arrayByPerformingSelector:@selector(delete)];
//			// add the new tracks
//			[iTunes add:newTracks to:iTunesDJ];
//			// put the old tracks back
//			[oldTracks arrayByPerformingSelector:@selector(duplicateTo:) withObject:iTunesDJ];
		} else if (append) {
			[iTunes add:newTracks to:iTunesDJ];
		} else {
			// Play first track, queue rest
			[[self iTunesScript] executeSubroutine:@"ps_play_track" 
										 arguments:[NSArray arrayWithObject:[NSAppleEventDescriptor aliasListDescriptorWithArray:paths]]
											 error:&errorDict];
		}
	} else if (append) {
		iTunesPlaylist *qs = [[QSiTunesLibrary() userPlaylists] objectWithName:QSiTunesDynamicPlaylist];
		[iTunes add:newTracks to:qs];
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
				if ([[trackResult lastObject] videoKind] != iTunesEVdKNone) {
					// give iTunes focus when playing a video
					[iTunes activate];
				}
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
	iTunesSource *library = QSiTunesLibrary();
	iTunesPlaylist *qs = [[library userPlaylists] objectWithName:QSiTunesDynamicPlaylist];
	if ([qs exists]) {
		// clear the old list
		[[qs tracks] removeAllObjects];
	} else {
		// create the dynamic playlist
		qs = [[[iTunes classForScriptingClass:@"playlist"] alloc] init];
		[[library userPlaylists] insertObject:qs atIndex:0];
		[qs setName:QSiTunesDynamicPlaylist];
	}
	// filter out PDFs and (optionally) videos
	BOOL includeVideos = [[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesIncludeVideos"];
	NSMutableArray *songsOnly = [NSMutableArray arrayWithCapacity:[trackList count]];
	for (iTunesFileTrack *track in trackList) {
		if ([[track kind] isEqualToString:QSiTunesBookletKind]) {
			continue;
		}
		// TODO if playlist is *only* videos, include them regardless of preferences
		if (!includeVideos && [track videoKind] != iTunesEVdKNone) {
			continue;
		}
		[songsOnly addObject:track];
	}
	[iTunes add:[songsOnly valueForKey:@"location"] to:qs];
	[qs playOnce:YES];
}

- (QSObject *)playPlaylist:(QSObject *)dObject
{
	iTunesPlaylist *playlist = [self playlistObjectFromQSObject:dObject];
	[playlist playOnce:YES];
	return nil;
}

- (QSObject *)appendTracks:(QSObject *)dObject toPlaylist:(QSObject *)iObject
{
	// get the tracks
	NSArray *trackResult = [self trackObjectsFromQSObject:dObject];
	NSArray *newTracks = [trackResult valueForKey:@"location"];
	// get the playlist
	NSArray *playlistResult = [[QSiTunesLibrary() playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", [iObject identifier]]];
	if ([playlistResult count] > 0) {
		[iTunes add:newTracks to:[playlistResult lastObject]];
	}
	return nil;
}

- (QSObject *)revealItem:(QSObject *)dObject
{
	[iTunes activate];
	if ([dObject containsType:QSiTunesPlaylistIDPboardType]) {
		[[self playlistObjectFromQSObject:dObject] reveal];
	} else if ([dObject containsType:QSiTunesTrackIDPboardType]) {
		// TODO this doesn't work on tracks
		NSArray *trackResult = [self trackObjectsFromQSObject:dObject];
		iTunesTrack *track = [trackResult lastObject];
		[track reveal];
	}
	return nil;
}

- (QSObject *)openBooklet:(QSObject *)dObject
{
	NSArray *itemsToCheck = [self trackObjectsFromQSObject:dObject];
	for (iTunesFileTrack *track in itemsToCheck) {
		if ([[track kind] isEqualToString:QSiTunesBookletKind]) {
			[[NSWorkspace sharedWorkspace] openURL:[track location]];
		}
	}
	return nil;
}

- (NSArray *)validIndirectObjectsForAction:(NSString *)action directObject:(QSObject *)dObject
{
	if ([action isEqualToString:@"QSiTunesAddToPlaylistAction"]) {
		return [QSLib arrayForType:QSiTunesPlaylistIDPboardType];
	}
	return nil;
}

- (NSArray *)trackObjectsFromQSObject:(QSObject *)tracks
{
	NSArray *trackResult = nil;
	// get iTunesTrack objects to represent each track
	if ([tracks containsType:QSiTunesPlaylistIDPboardType]) {
		// from a playlist
		trackResult = [[self playlistObjectFromQSObject:tracks] fileTracks];
	} else if ([tracks containsType:QSiTunesBrowserPboardType]) {
		// from browsing in Quicksilver
		NSMutableArray *formatStrings = [NSMutableArray arrayWithCapacity:1];
		NSDictionary *browseDict;
		NSDictionary *criteriaDict;
		NSString *formatString;
		NSMutableArray *criteria = [NSMutableArray arrayWithCapacity:2];
		NSArray *allTracks = [[[QSiTunesLibrary() libraryPlaylists] objectAtIndex:0] fileTracks];
		// pre-fetch artist info
		NSArray *artists = [allTracks valueForKey:@"artist"];
		NSArray *albumArtists = [allTracks valueForKey:@"albumArtist"];
		NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];
		NSIndexSet *matches;
		BOOL (^artistFilter)(id obj, NSUInteger index, BOOL *stop);
		BOOL first;
		for (QSObject *browseResult in [tracks splitObjects]) {
			browseDict = [browseResult objectForType:QSiTunesBrowserPboardType];
			criteriaDict = [browseDict objectForKey:@"Criteria"];
			
			// build a search query
			formatString = @"(";
			first = YES;
			for (NSString *criteriaKey in [criteriaDict allKeys]) {
				if ([criteriaKey isEqualToString:@"Artist"] && [[criteriaDict objectForKey:@"Artist"] isEqualToString:COMPILATION_STRING]) {
					// don't use "Compilations" as a criteria
					continue;
				}
				if ([criteriaKey isEqualToString:@"Artist"]) {
					// can't use albumArtist in a predicate (too slow), so instead…
					// get indexes for tracks with a matching album artist or artist and move on
					artistFilter = ^(id obj, NSUInteger index, BOOL *stop){
						return [obj isEqualToString:[criteriaDict objectForKey:criteriaKey]];
					};
					matches = [albumArtists indexesOfObjectsPassingTest:artistFilter];
					[indexes addIndexes:matches];
					matches = [artists indexesOfObjectsPassingTest:artistFilter];
					[indexes addIndexes:matches];
					continue;
				}
				if ([criteriaDict objectForKey:criteriaKey]) {
					[criteria addObject:[criteriaKey lowercaseString]];
					[criteria addObject:[criteriaDict objectForKey:criteriaKey]];
					if (first) {
						first = NO;
					} else {
						formatString = [formatString stringByAppendingString:@" AND "];
					}
					formatString = [formatString stringByAppendingString:@"%K == %@"];
				}
			}
			formatString = [formatString stringByAppendingString:@")"];
			[formatStrings addObject:formatString];
		}
		formatString = [formatStrings componentsJoinedByString:@" OR "];
		NSPredicate *trackFilter = [NSPredicate predicateWithFormat:formatString argumentArray:criteria];
		//NSLog(@"playlist filter: %@", [trackFilter predicateFormat]);
		// TODO see if we can get the results to be in the same order as the objects passed in
		if ([indexes count]) {
			// only filter tracks that had matching artist info
			trackResult = [[allTracks objectsAtIndexes:indexes] filteredArrayUsingPredicate:trackFilter];
		} else {
			// filter all tracks
			trackResult = [allTracks filteredArrayUsingPredicate:trackFilter];
		}
	} else if ([tracks containsType:QSiTunesTrackIDPboardType]) {
		// from individual track objects
		NSString *searchFilter = @"persistentID == %@";
		NSArray *trackIDs = [[tracks splitObjects] arrayByPerformingSelector:@selector(identifier)];
		NSMutableArray *filters = [NSMutableArray arrayWithCapacity:[tracks count]];
		for (int i = 0; i < [tracks count]; i++) {
			[filters addObject:searchFilter];
		}
		searchFilter = [filters componentsJoinedByString:@" OR "];
		iTunesLibraryPlaylist *libraryPlaylist = [[QSiTunesLibrary() libraryPlaylists] objectAtIndex:0];
		trackResult = [[libraryPlaylist fileTracks] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:searchFilter argumentArray:trackIDs]];
	}
	return trackResult;
}

- (iTunesPlaylist *)playlistObjectFromQSObject:(QSObject *)playlist
{
	NSArray *playlistResult = [[QSiTunesLibrary() playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", [playlist identifier]]];
	if ([playlistResult count] > 0) {
		return [playlistResult objectAtIndex:0];
	}
	return nil;
}

- (NSAppleScript *)iTunesScript {
	if (!iTunesScript)
		iTunesScript = [[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:
			[[NSBundle bundleForClass:[QSiTunesActionProvider class]]  pathForResource:@"iTunes" ofType:@"scpt"]] error:nil];
	return iTunesScript;
}

- (void)manaullyAppendTrack:(iTunesTrack *)track toPlaylist:(iTunesPlaylist *)playlist
{
	[track sendEvent:'core' id:'clon' parameters:'insh', playlist, 0];
}

@end

@implementation QSiTunesControlProvider

- (id)init {
	if (self = [super init]) {
		iTunes = [QSiTunes() retain];
	}
	return self;
}

- (void)dealloc {
    [iTunes release];
    [super dealloc];
}

- (void)play
{
	[iTunes playOnce:YES];
}

- (void)pause
{
	[iTunes pause];
}

- (void)togglePlayPause
{
	[iTunes playpause];
}

- (void)stop
{
	[iTunes stop];
}

- (void)next
{
	[iTunes nextTrack];
}

- (void)previous
{
	[iTunes previousTrack];
}

- (void)volumeIncrease
{
	[iTunes setSoundVolume:[iTunes soundVolume] + 5];
}

- (void)volumeDecrease
{
	[iTunes setSoundVolume:[iTunes soundVolume] - 5];
}

- (void)volumeMute
{
	[iTunes setMute:![iTunes mute]];
}

- (void)ratingIncrease
{
	iTunesTrack *track = [iTunes currentTrack];
	if ([track rating] < 100) {
		[track setRating:[track rating] + 20];
	}
}

- (void)ratingDecrease
{
	iTunesTrack *track = [iTunes currentTrack];
	if ([track rating] > 0) {
		[track setRating:[track rating] - 20];
	}
}

- (void)rating0
{
	[[iTunes currentTrack] setRating:0];
}

- (void)rating1
{
	[[iTunes currentTrack] setRating:20];
}

- (void)rating2
{
	[[iTunes currentTrack] setRating:40];
}

- (void)rating3
{
	[[iTunes currentTrack] setRating:60];
}

- (void)rating4
{
	[[iTunes currentTrack] setRating:80];
}

- (void)rating5
{
	[[iTunes currentTrack] setRating:100];
}

@end
