//
//  QSiTunesActionProvider.m
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "QSiTunesActionProvider.h"

@implementation QSiTunesActionProvider
- (id)init {
	if (self = [super init]) {
		_iTunes = QSiTunes();
	}
	return self;
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

#pragma mark - Standard iTunes actions

- (QSObject *)playItem:(QSObject *)dObject { 
	if ([dObject containsType:QSiTunesPlaylistIDPboardType]) {
		[self playPlaylist:dObject];
	} else if ([dObject containsType:QSiTunesBrowserPboardType]) {
		[self playBrowser:dObject];
	} else if ([dObject containsType:QSiTunesTrackIDPboardType]) {
		[self playTrack:dObject append:NO];
	}
	return nil;
}

- (void)playBrowser:(QSObject *)dObject
{
	NSArray *tracksToPlay = [self trackObjectsFromQSObject:dObject];
    [self playUsingDynamicPlaylist:tracksToPlay];
}

- (void)playTrack:(QSObject *)dObject append:(BOOL)append {
	NSDictionary *errorDict = nil;
	
	// get iTunesTrack objects to represent each track
	NSArray *trackResult = [self trackObjectsFromQSObject:dObject];
	NSArray *newTracks = [trackResult valueForKey:@"location"];
	
	if (append) {
		iTunesPlaylist *qs = [[QSiTunesLibrary() userPlaylists] objectWithName:QSiTunesDynamicPlaylist];
		[[self iTunes] add:newTracks to:qs];
	} else {
		NSString *playlist = [dObject objectForMeta:@"QSiTunesSourcePlaylist"];
		if (playlist) {
			/* Tracks play in the context of the music library by default. To get it
			   to play in the context of a playlist, we have to get an object representing
			   the playlist, then get a track object from that playlist's tracks.
			*/
			NSPredicate *findContainer = [NSPredicate predicateWithFormat:@"persistentID == %@", playlist];
			iTunesPlaylist *container = [[[QSiTunesLibrary() userPlaylists] filteredArrayUsingPredicate:findContainer] objectAtIndex:0];
			// ignore all but the first track
			NSString *trackInPlaylistID = [[trackResult objectAtIndex:0] persistentID];
			NSPredicate *findTrack = [NSPredicate predicateWithFormat:@"persistentID == %@", trackInPlaylistID];
			iTunesTrack *trackInPlaylist = [[[container tracks] filteredArrayUsingPredicate:findTrack] objectAtIndex:0];
			[trackInPlaylist playOnce:NO];
		} else {
			if ([trackResult count] == 1) {
				// for a single track, just play
				if ([[trackResult lastObject] videoKind] != iTunesEVdKNone) {
					// give iTunes focus when playing a video
					[[self iTunes] activate];
				}
				[[trackResult lastObject] playOnce:YES];
			} else {
				// for multiple tracks, create a playlist and play
				[self playUsingDynamicPlaylist:trackResult];
			}
		}
	}
	if (errorDict) {NSLog(@"Error: %@", errorDict);}
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
        @try {
            qs = [[[[self iTunes] classForScriptingClass:@"playlist"] alloc] init];
            [[library userPlaylists] insertObject:qs atIndex:0];
            [qs setName:QSiTunesDynamicPlaylist];
        }
        @catch (NSException *exception) {
            NSLog(@"Unable to create Quicksilver playlist. iTunes was not responding.");
            return;
        }
	}
	// filter out booklets, iTunes LP, and (optionally) videos
	BOOL skipVideos = ![[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesIncludeVideos"];
	NSString *filterString = [NSString stringWithFormat:@"kind != '%@' AND kind != '%@'", QSiTunesBookletKind, QSiTunesLPKind];
	if (skipVideos) {
		filterString = [filterString stringByAppendingFormat:@" AND videoKind == %i", iTunesEVdKNone];
	}
	NSPredicate *trackFilter = [NSPredicate predicateWithFormat:filterString];
	//NSLog(@"playlist filter: %@", [trackFilter predicateFormat]);
	NSArray *songsOnly = [trackList filteredArrayUsingPredicate:trackFilter];
	if (skipVideos && [songsOnly count] == 0) {
		// no results when skipping videos
		// see if all tracks are videos, and if so, play them anyway
		NSPredicate *videoFilter = [NSPredicate predicateWithFormat:@"videoKind != %i", iTunesEVdKNone];
		NSArray *videos = [trackList filteredArrayUsingPredicate:videoFilter];
		if ([videos count] == [trackList count]) {
			songsOnly = videos;
		}
	}
	// play the resulting tracks
	[[self iTunes] add:[songsOnly valueForKey:@"location"] to:qs];
	[qs playOnce:YES];
}

- (void)playPlaylist:(QSObject *)dObject
{
	// iTunes can only play one at a time, so grab the last one
	QSObject *lastPlaylist = [[dObject splitObjects] lastObject];
	iTunesPlaylist *playlist = [self playlistObjectFromQSObject:lastPlaylist];
	[playlist playOnce:YES];
}

- (QSObject *)appendTracks:(QSObject *)dObject toPlaylist:(QSObject *)iObject
{
	// get the tracks
	NSArray *trackResult = [self trackObjectsFromQSObject:dObject];
	NSArray *newTracks = [trackResult valueForKey:@"location"];
	// get the playlist
	NSArray *playlistResult = [[QSiTunesLibrary() playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", [iObject identifier]]];
	if ([playlistResult count] > 0) {
		[[self iTunes] add:newTracks to:[playlistResult lastObject]];
	}
	return nil;
}

- (QSObject *)revealItem:(QSObject *)dObject
{
	[[self iTunes] activate];
	if ([dObject containsType:QSiTunesPlaylistIDPboardType]) {
		[[self playlistObjectFromQSObject:dObject] reveal];
	} else if ([dObject containsType:QSiTunesTrackIDPboardType]) {
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

- (QSObject *)getLyrics:(QSObject *)dObject
{
	iTunesTrack *track = [[self trackObjectsFromQSObject:dObject] lastObject];
	NSString *lyricsText = [track lyrics];
	if ([lyricsText length]) {
		NSString *name = [NSString stringWithFormat:@"Lyrics for %@", [track name]];
		QSObject *lyrics = [QSObject objectWithName:name];
		[lyrics setObject:lyricsText forType:QSTextType];
		[lyrics setPrimaryType:QSTextType];
		return lyrics;
	}
	return nil;
}

- (QSObject *)toggleShuffle:(QSObject *)dObject
{
	for (QSObject *qsplaylist in [dObject splitObjects]) {
		iTunesPlaylist *playlist = [self playlistObjectFromQSObject:qsplaylist];
		if (playlist) {
			[playlist setShuffle:![playlist shuffle]];
		}
	}
	return nil;
}

- (QSObject *)toggleEnabled:(QSObject *)dObject
{
	for (iTunesTrack *track in [self trackObjectsFromQSObject:dObject]) {
		[track setEnabled:![track enabled]];
	}
	return nil;
}

- (QSObject *)selectEQPreset:(QSObject *)dObject
{
	iTunesEQPreset *eq = [dObject objectForType:QSiTunesEQPresetType];
	if ([[self iTunes] isRunning] && ![[[self iTunes] currentEQPreset] isEqual:eq]) {
        [[self iTunes] setEQEnabled:YES];
		[[self iTunes] setCurrentEQPreset:eq];
	}
	return nil;
}

#pragma mark - Quicksilver validation

- (NSArray *)validActionsForDirectObject:(QSObject *)dObject indirectObject:(QSObject *)iObject {
	if ([dObject objectForType:QSiTunesBrowserPboardType]) {
		NSDictionary *browseDict = [dObject objectForType:QSiTunesBrowserPboardType];
		if (![browseDict objectForKey:@"Criteria"])
			return nil;
	}
	return [NSArray arrayWithObject:kQSiTunesPlayItemAction];
}

- (NSArray *)validIndirectObjectsForAction:(NSString *)action directObject:(QSObject *)dObject
{
	if ([action isEqualToString:@"QSiTunesAddToPlaylistAction"]) {
		return [QSLib scoredArrayForType:QSiTunesPlaylistIDPboardType];
	}
	return nil;
}

#pragma mark - Internal helper methods

- (NSArray *)trackObjectsFromQSObject:(QSObject *)tracks
{
	NSArray *trackResult = nil;
	// get iTunesTrack objects to represent each track
	if ([tracks containsType:QSiTunesPlaylistIDPboardType]) {
		// from a playlist
		// the location property is missing unless you call `get` on the result
		trackResult = [(SBElementArray *)[[self playlistObjectFromQSObject:tracks] tracks] get];
	} else if ([tracks containsType:QSiTunesBrowserPboardType]) {
		// from browsing in Quicksilver
		NSMutableArray *formatStrings = [NSMutableArray arrayWithCapacity:1];
		NSDictionary *browseDict = nil;
		NSDictionary *criteriaDict = nil;
		NSString *formatString = nil;
		NSMutableArray *criteria = [NSMutableArray arrayWithCapacity:2];
        // the location property is missing unless you call `get` on the result
        // calling `get` on an array this size is very wasteful, so use fileTracks instead
		SBElementArray *allTracks = [QSiTunesMusic() fileTracks];
		// pre-fetch artist info
		NSArray *artists = [allTracks valueForKey:@"artist"];
		NSArray *albumArtists = [allTracks valueForKey:@"albumArtist"];
		NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];
		BOOL (^artistFilter)(NSString *, NSUInteger, BOOL *);
		BOOL first = YES;
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
					// using albumArtist in a predicate is way too slow, so instead…
					// get indexes for tracks with a matching album artist or artist and move on
                    NSIndexSet *matches = nil;
                    NSString *artist = [criteriaDict objectForKey:criteriaKey];
					artistFilter = ^BOOL(NSString *thisArtist, NSUInteger index, BOOL *stop){
						return [thisArtist isEqualToString:artist];
					};
					matches = [artists indexesOfObjectsWithOptions:NSEnumerationConcurrent passingTest:artistFilter];
					[indexes addIndexes:matches];
					matches = [albumArtists indexesOfObjectsWithOptions:NSEnumerationConcurrent passingTest:artistFilter];
					[indexes addIndexes:matches];
                    // don't do anything else for artist criteria
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
        if ([criteria count]) {
            formatString = [formatStrings componentsJoinedByString:@" OR "];
            NSPredicate *trackFilter = [NSPredicate predicateWithFormat:formatString argumentArray:criteria];
            //NSLog(@"playlist filter: %@", [trackFilter predicateFormat]);
            // TODO see if we can get the results to be in the same order as the objects passed in
            if ([indexes count]) {
                // artist was one of the criteria
                // only filter tracks that had matching artist or album artist
                trackResult = [[allTracks objectsAtIndexes:indexes] filteredArrayUsingPredicate:trackFilter];
            } else {
                // filter all tracks
                // every message to this filtered array will be slow
                // calling `get` here limits the slowness to one operation
                trackResult = [(SBElementArray *)[allTracks filteredArrayUsingPredicate:trackFilter] get];
            }
        } else {
            // artist was the only criteria - no further filtering
            trackResult = [allTracks objectsAtIndexes:indexes];
        }
	} else if ([tracks containsType:QSiTunesTrackIDPboardType]) {
		// from individual track objects
		NSString *searchFilter = @"persistentID == %@";
		NSArray *trackIDs = [[tracks splitObjects] arrayByPerformingSelector:@selector(identifier)];
		NSMutableArray *filters = [NSMutableArray arrayWithCapacity:[tracks count]];
		for (NSUInteger i = 0; i < [tracks count]; i++) {
			[filters addObject:searchFilter];
		}
		searchFilter = [filters componentsJoinedByString:@" OR "];
		iTunesLibraryPlaylist *libraryPlaylist = QSiTunesMusic();
		// the location property is missing unless you call `get` on the result
		trackResult = [(SBElementArray *)[[libraryPlaylist tracks] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:searchFilter argumentArray:trackIDs]] get];
	}
    // there may be some objects in the results that don't have a location - filter them
    NSIndexSet *fileTracks = [trackResult indexesOfObjectsWithOptions:NSEnumerationConcurrent passingTest:^BOOL(id track, NSUInteger i, BOOL *stop) {
        return [track respondsToSelector:@selector(location)];
    }];
	return [trackResult objectsAtIndexes:fileTracks];
}

- (iTunesPlaylist *)playlistObjectFromQSObject:(QSObject *)playlist
{
	NSArray *playlistResult = [[QSiTunesLibrary() playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"persistentID == %@", [playlist identifier]]];
	if ([playlistResult count] > 0) {
		return [playlistResult objectAtIndex:0];
	}
	return nil;
}

- (void)manaullyAppendTrack:(iTunesTrack *)track toPlaylist:(iTunesPlaylist *)playlist
{
	[track sendEvent:'core' id:'clon' parameters:'insh', playlist, 0];
}

@end

@implementation QSiTunesControlProvider

- (id)init {
	if (self = [super init]) {
		_iTunes = QSiTunes();
	}
	return self;
}

- (void)play
{
    if ([[self iTunes] respondsToSelector:@selector(playOnce:)]) {
        [[self iTunes] playOnce:YES];
    } else if ([[self iTunes] playerState] != iTunesEPlSPlaying) {
        [[self iTunes] playpause];
    }
}

- (void)pause
{
	[[self iTunes] pause];
}

- (void)togglePlayPause
{
	[[self iTunes] playpause];
}

- (void)stop
{
	[[self iTunes] stop];
}

- (void)next
{
	[[self iTunes] nextTrack];
}

- (void)previous
{
	[[self iTunes] backTrack];
}

- (void)volumeIncrease
{
	[[self iTunes] setSoundVolume:[[self iTunes] soundVolume] + 5];
}

- (void)volumeDecrease
{
	[[self iTunes] setSoundVolume:[[self iTunes] soundVolume] - 5];
}

- (void)volumeMute
{
	[[self iTunes] setMute:![[self iTunes] mute]];
}

- (void)ratingIncrease
{
	iTunesTrack *track = [[self iTunes] currentTrack];
	if ([track rating] < 100) {
		[track setRating:[track rating] + 20];
	}
}

- (void)ratingDecrease
{
	iTunesTrack *track = [[self iTunes] currentTrack];
	if ([track rating] > 0) {
		[track setRating:[track rating] - 20];
	}
}

- (void)rating0
{
	[[[self iTunes] currentTrack] setRating:0];
}

- (void)rating1
{
	[[[self iTunes] currentTrack] setRating:20];
}

- (void)rating2
{
	[[[self iTunes] currentTrack] setRating:40];
}

- (void)rating3
{
	[[[self iTunes] currentTrack] setRating:60];
}

- (void)rating4
{
	[[[self iTunes] currentTrack] setRating:80];
}

- (void)rating5
{
	[[[self iTunes] currentTrack] setRating:100];
}

- (void)equalizerToggle
{
    [[self iTunes] setEQEnabled:![[self iTunes] EQEnabled]];
}

@end
