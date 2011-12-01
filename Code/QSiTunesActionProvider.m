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
		NSArray *newTracks = [tracksToPlay arrayByPerformingSelector:@selector(location)];
		if (next) {
			// play next
			[[self iTunesScript] executeSubroutine:@"ps_play_next_criteria" arguments:ascriteria  error:&errorDict];
		} else if (append) {
			// append to iTunes DJ
			[QSiTunes() add:newTracks to:iTunesDJ];
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
				if ([[trackResult lastObject] videoKind] != iTunesEVdKNone) {
					// give iTunes focus when playing a video
					[QSiTunes() activate];
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
	// filter out PDFs and (optionally) videos
	BOOL includeVideos = [[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesIncludeVideos"];
	NSMutableArray *songsOnly = [NSMutableArray arrayWithCapacity:[trackList count]];
	for (iTunesFileTrack *track in trackList) {
		if ([[track kind] isEqualToString:QSiTunesBookletKind]) {
			continue;
		}
		// TODO add a preference to include videos
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
	NSString *playlistID = [dObject identifier];
	iTunesSource *library = QSiTunesLibrary();
	NSArray *playlistResult = [[library playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", playlistID]];
	if ([playlistResult count] > 0) {
		[[playlistResult lastObject] playOnce:YES];
	}
	return nil;
}

- (QSObject *)appendTracks:(QSObject *)dObject toPlaylist:(QSObject *)iObject
{
	// get the tracks
	NSArray *trackResult = [self trackObjectsFromQSObject:dObject];
	NSArray *newTracks = [trackResult arrayByPerformingSelector:@selector(location)];
	// get the playlist
	NSArray *playlistResult = [[QSiTunesLibrary() playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", [iObject identifier]]];
	if ([playlistResult count] > 0) {
		[QSiTunes() add:newTracks to:[playlistResult lastObject]];
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
		NSArray *playlistResult = [[QSiTunesLibrary() playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", [tracks identifier]]];
		if ([playlistResult count] > 0) {
			trackResult = [[playlistResult objectAtIndex:0] fileTracks];
		}
	} else if ([tracks containsType:QSiTunesBrowserPboardType]) {
		// from browsing in Quicksilver
		NSMutableArray *formatStrings = [NSMutableArray arrayWithCapacity:1];
		NSDictionary *browseDict;
		NSDictionary *criteriaDict;
		NSString *formatString;
		NSMutableArray *criteria = [NSMutableArray arrayWithCapacity:2];
		BOOL first;
		for (QSObject *browseResult in [tracks splitObjects]) {
			browseDict = [browseResult objectForType:QSiTunesBrowserPboardType];
			criteriaDict = [browseDict objectForKey:@"Criteria"];
			
			// build a search query
			/*
			 // Adding kind and videoKind to the predicate seems like the right way to filter, but
			 while it will work, it's *very* slow for some reason. Better to just enumerate the result later.
			 Also, adding albumArtist to the predicate is unusably slow. iTunes hits 100% CPU for several minutes.
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
		iTunesLibraryPlaylist *libraryPlaylist = [[QSiTunesLibrary() libraryPlaylists] objectAtIndex:0];
		// TODO see if we can get the results to be in the same order as the objects passed in
		trackResult = [[libraryPlaylist fileTracks] filteredArrayUsingPredicate:trackFilter];
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

- (NSAppleScript *)iTunesScript {
	if (!iTunesScript)
		iTunesScript = [[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:
			[[NSBundle bundleForClass:[QSiTunesActionProvider class]]  pathForResource:@"iTunes" ofType:@"scpt"]] error:nil];
	return iTunesScript;
}

@end

@implementation QSiTunesControlProvider

- (void)play
{
	[QSiTunes() playOnce:YES];
}

- (void)pause
{
	[QSiTunes() pause];
}

- (void)togglePlayPause
{
	[QSiTunes() playpause];
}

- (void)stop
{
	[QSiTunes() stop];
}

- (void)next
{
	[QSiTunes() nextTrack];
}

- (void)previous
{
	[QSiTunes() previousTrack];
}

- (void)volumeIncrease
{
	iTunesApplication *iTunes = QSiTunes();
	[iTunes setSoundVolume:[iTunes soundVolume] + 10];
}

- (void)volumeDecrease
{
	iTunesApplication *iTunes = QSiTunes();
	[iTunes setSoundVolume:[iTunes soundVolume] - 10];
}

- (void)volumeMute
{
	iTunesApplication *iTunes = QSiTunes();
	[iTunes setMute:![iTunes mute]];
}

- (void)ratingIncrease
{
	iTunesTrack *track = [QSiTunes() currentTrack];
	if ([track rating] < 100) {
		[track setRating:[track rating] + 20];
	}
}

- (void)ratingDecrease
{
	iTunesTrack *track = [QSiTunes() currentTrack];
	if ([track rating] > 0) {
		[track setRating:[track rating] - 20];
	}
}

- (void)rating0
{
	[[QSiTunes() currentTrack] setRating:0];
}

- (void)rating1
{
	[[QSiTunes() currentTrack] setRating:20];
}

- (void)rating2
{
	[[QSiTunes() currentTrack] setRating:40];
}

- (void)rating3
{
	[[QSiTunes() currentTrack] setRating:60];
}

- (void)rating4
{
	[[QSiTunes() currentTrack] setRating:80];
}

- (void)rating5
{
	[[QSiTunes() currentTrack] setRating:100];
}

@end
