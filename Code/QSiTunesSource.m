#import "QSiTunesSource.h"
#import "QSiTunesDatabase.h"

@implementation QSiTunesObjectSource

- (id)init {
	if (self = [super init]) {
		//[[QSVoyeur sharedInstance] addPathToQueue:[self libraryLocation]];
		
		//      [(QSObjectCell *)theCell setImagePosition:NSImageBelow];
		
		_recentTracks = nil;
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesMonitorTracks"]) {
			_recentTracks = [NSMutableArray array];
			//iTunesMonitorDeactivate();
			//iTunesMonitorActivate([[NSBundle bundleForClass:[self class]]pathForResource:@"iTunesMonitor" ofType:@""]);
			
			
//			int thisID = iTunesGetCurrentTrack();
//			if (thisID > 0)
//				[recentTracks addObject:[NSString stringWithFormat:@"%d", thisID]];
//			
			[[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(iTunesStateChanged:) name:@"com.apple.iTunes.playerInfo" object:nil];
			//	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(quitMonitor:) name:NSApplicationWillTerminateNotification object:nil];
		}
		_iTunes = QSiTunes();
        [_iTunes setDelegate:self];
	}
	return self;
}
//-(void)quitMonitor:(NSNotification *)notif {
//	iTunesMonitorDeactivate();
//	
//}


- (void)iTunesStateChanged:(NSNotification *)notif {
	NSString *trackID = [[notif userInfo] objectForKey:@"Track ID"];
	// userInfo is missing some data, like Sample Rate - get better info
	NSMutableDictionary *trackInfo = [[[QSiTunesDatabase sharedInstance] trackInfoForID:trackID] mutableCopy];
	if ([[[notif userInfo] objectForKey:@"Player State"] isEqualToString:@"Playing"]) {
		
		NSString *newTrack = [self currentTrackID];
		if ([newTrack integerValue] <= 0) {
			return;
		}
        NSUInteger persistentIDNumber = [[trackInfo objectForKey:@"PersistentID"] integerValue];
        NSString *currentTrackPersistentID = [NSString stringWithFormat:@"%lX", (unsigned long)persistentIDNumber];
		NSString *lastPersistentID = @"0";
        // library entries use "Persistent ID"
        [trackInfo setObject:currentTrackPersistentID forKey:@"Persistent ID"];
        [trackInfo removeObjectForKey:@"PersistentID"];
		if ([[self recentTracks] count]) {
			lastPersistentID = [[[self recentTracks] objectAtIndex:0] objectForKey:@"Persistent ID"];
		}
		// don't add the track again when hitting pause, then play
		if (currentTrackPersistentID && ![currentTrackPersistentID isEqualToString:lastPersistentID]) {
			[[self recentTracks] insertObject:trackInfo atIndex:0];
		}
		while ([[self recentTracks] count] > 25) [[self recentTracks] removeLastObject];
		
		if (![[QSiTunesDatabase sharedInstance] trackInfoForID:newTrack]) {
			// playing a track that wasn't in the library on last scan
			[[QSiTunesDatabase sharedInstance] registerAdditionalTrack:trackInfo forID:newTrack];
		}
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesNotifyTracks"]) {
			[self showNotificationForTrack:newTrack info:trackInfo];
		}
		[[QSLib objectWithIdentifier:QSiTunesRecentTracksBrowser] unloadChildren];
	}
}


- (NSArray *)typesForProxyObject:(id)proxy {
	NSString *ident = [proxy identifier];
	if ([ident isEqualToString:@"QSCurrentTrackProxy"]) {
		return [NSArray arrayWithObject:QSiTunesTrackIDPboardType];
	} else if ([ident isEqualToString:@"QSCurrentPlaylistProxy"]) {
		return [NSArray arrayWithObject:QSiTunesPlaylistIDPboardType];
	} else if ([ident isEqualToString:@"QSCurrentSelectionProxy"]) {
		return [NSArray arrayWithObject:QSiTunesTrackIDPboardType]; 		
	}
	return nil;
}

- (id)resolveProxyObject:(id)proxy {
	if (![proxy isKindOfClass:[NSString class]])
		proxy = [proxy identifier];
	if ([proxy isEqualToString:@"QSRandomTrackProxy"]) {
		srand(time(NULL));
		NSArray *allTracks = [[QSiTunesDatabase sharedInstance] tracksMatchingCriteria:nil];
		long upper = [allTracks count];
		if (upper >= 1) {
			NSUInteger trackIndex = random() % upper;
			NSDictionary *randomTrack = [allTracks objectAtIndex:trackIndex];
			return [[QSiTunesDatabase sharedInstance] trackObjectForInfo:randomTrack inPlaylist:nil];
		}
	}
	// the rest of these require iTunes to be running
	if (![[self iTunes] isRunning]) {
		return nil;
	}
	if ([proxy isEqualToString:@"QSCurrentTrackProxy"]) {
		id object = [[QSiTunesDatabase sharedInstance] trackObjectForInfo:[self currentTrackInfo]
								inPlaylist:nil];
		//NSLog(@"object %@", object);
		return object;
	} else if ([proxy isEqualToString:@"QSCurrentAlbumProxy"]) {
		
		id newObject = [self browserObjectForTrack:[self currentTrackInfo] andCriteria:@"Album"];
		return newObject;
	} else if ([proxy isEqualToString:@"QSCurrentArtistProxy"]) {
		id newObject = [self browserObjectForTrack:[self currentTrackInfo] andCriteria:@"Artist"];
		return newObject;
	} else if ([proxy isEqualToString:@"QSCurrentPlaylistProxy"]) {
		NSString *name = [[[self iTunes] currentPlaylist] name];
		NSDictionary *thisPlaylist = [[QSiTunesDatabase sharedInstance] playlistInfoForName:name];
		
		QSObject *newObject = [QSObject objectWithName:name];
		[newObject setObject:[thisPlaylist objectForKey:@"Playlist ID"] forType:QSiTunesPlaylistIDPboardType];
		[newObject setIdentifier:[thisPlaylist objectForKey:@"Playlist Persistent ID"]];
		[newObject setPrimaryType:QSiTunesPlaylistIDPboardType];
		return newObject;
	}
	return nil;
}

- (NSTimeInterval)cacheTimeForProxy:(id)proxy
{
    NSString *ident = [proxy identifier];
    if ([ident isEqualToString:@"QSRandomTrackProxy"]) {
        return 8.0;
    }
    return 3.0;
}

- (NSDictionary *)trackInfoForID:(id)trackID
{
	NSMutableDictionary *trackInfo = nil;
	// check the in-memory database first
	id libraryTrackInfo;
	if (libraryTrackInfo = [[QSiTunesDatabase sharedInstance] trackInfoForID:trackID]) {
		trackInfo = libraryTrackInfo;
	}
	if (!trackInfo) {
		// fall back to querying iTunes (if it's running)
		if ([[self iTunes] isRunning]) {
			iTunesLibraryPlaylist *libraryPlaylist = QSiTunesMusic();
			NSArray *trackResult = [[libraryPlaylist tracks] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"databaseID == %@", trackID]];
			if ([trackResult count] > 0) {
				iTunesFileTrack *track = [trackResult lastObject];
                trackInfo = [NSMutableDictionary dictionaryWithCapacity:1];
				[trackInfo setObject:[track persistentID] forKey:@"Persistent ID"];
				[trackInfo setObject:[track artist] forKey:@"Artist"];
				[trackInfo setObject:[track albumArtist] forKey:@"Album Artist"];
				[trackInfo setObject:[track composer] forKey:@"Composer"];
				[trackInfo setObject:[track genre] forKey:@"Genre"];
				[trackInfo setObject:[track album] forKey:@"Album"];
				[trackInfo setObject:[track name] forKey:@"Name"];
				[trackInfo setObject:[NSNumber numberWithInteger:[track rating]] forKey:@"Rating"];
				[trackInfo setObject:[NSString stringWithFormat:@"%d", (int)[track albumRating]] forKey:@"Album Rating"];
				[trackInfo setObject:[track kind] forKey:@"Kind"];
				[trackInfo setObject:[NSNumber numberWithInteger:1] forKey:@"Artwork Count"];
                if ([track respondsToSelector:@selector(location)]) {
                    [trackInfo setObject:[track location] forKey:@"Location"];
                }
				[trackInfo setObject:[NSString stringWithFormat:@"%d", (int)[track playedCount]] forKey:@"Play Count"];
				if ([track playedDate]) {
					[trackInfo setObject:[track playedDate] forKey:@"Play Date"];
				}
				[trackInfo setObject:[NSString stringWithFormat:@"%d", (int)[track skippedCount]] forKey:@"Skip Count"];
				[trackInfo setObject:[track time] forKey:@"Total Time"];
				[trackInfo setObject:[NSString stringWithFormat:@"%d", (int)[track trackNumber]] forKey:@"Track Number"];
				[trackInfo setObject:[NSString stringWithFormat:@"%d", (int)[track year]] forKey:@"Year"];
			}
		}
	}
	return trackInfo;
}

- (id)currentTrackInfo {
	return [self trackInfoForID:[self currentTrackID]];
}

- (NSString *)currentTrackID {
	return [NSString stringWithFormat:@"%ld", (long)[[[self iTunes] currentTrack] databaseID]];
}

- (void)showCurrentTrackNotification {
	if ([[self iTunes] isRunning]) {
        NSMutableDictionary *trackInfo = [self currentTrackInfo];
        // if rating has changed recently, it might not be in the database yet
        NSNumber *rating = [NSNumber numberWithInteger:[[[self iTunes] currentTrack] rating]];
        [trackInfo setObject:rating forKey:@"Rating"];
		[self showNotificationForTrack:0 info:trackInfo];
	} else {
		NSBeep();
	}
}

- (void)showNotificationForTrack:(id)trackID info:(NSDictionary *)trackInfo {
	if (!trackInfo) {
		trackInfo = [self trackInfoForID:trackID];
		// don't try to display a notification with nothing
		if (!trackInfo) {
			return;
		}
	}
	
	QSObject *track = [[QSiTunesDatabase sharedInstance] trackObjectForInfo:trackInfo inPlaylist:nil];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:@"QSEventNotification" object:@"QSiTunesTrackChangedEvent" userInfo:[NSDictionary dictionaryWithObject:track forKey:@"object"]];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:QSiTunesTrackChangeNotification
													   object:self userInfo:trackInfo];
	
	//NSLog(@"TRACKINFO %@", trackInfo);
	if (trackInfo) {
		NSString *name = [trackInfo objectForKey:@"Name"];
		NSString *artist = [trackInfo objectForKey:@"Artist"];
		NSString *album = [trackInfo objectForKey:@"Album"];
		NSString *location = [trackInfo objectForKey:@"Location"];
		
		//NSLog(@"%@ %@ %@", name, artist, album);
		if ([trackInfo objectForKey:@"Total Time"] == nil && !album && !artist && [location hasPrefix:@"http"]) {
			NSString *streamTitle = [[self iTunes] currentStreamTitle];
			
			if (streamTitle) {
				artist = name;
				name = streamTitle;
				//		NSLog(@"%@ %@ %@", name, artist, album);
				NSArray *components = [name componentsSeparatedByString:@" - "];
				if ([components count] == 2) {
					album = artist;
					artist = [components objectAtIndex:0];
					name = [components objectAtIndex:1];
				} 	
			}
		}
		
		NSMutableArray *array = [NSMutableArray array];
		if (artist) [array addObject:artist];
		if (album) [array addObject:album];
		NSString *text = [array componentsJoinedByString:@"\n"];

		
		NSImage *icon = nil;
		icon = [self imageForTrack:trackInfo];
		//NSLog(@"info : %@", trackInfo);
		if (!icon && ![trackInfo objectForKey:@"Location"]) {
			iTunesTrack *currentTrack = [[self iTunes] currentTrack];
			NSData *data = [[[currentTrack artworks] objectAtIndex:0] rawData];
			icon = [[NSImage alloc] initWithData:data];
		}
			
		if (!icon) icon = [QSResourceManager imageNamed:@"com.apple.iTunes"];
		
		QSShowNotifierWithAttributes([NSDictionary dictionaryWithObjectsAndKeys:
			@"QSiTunesTrackChangeNotification", QSNotifierType,
			name, QSNotifierTitle,
			text, QSNotifierText,
			icon, QSNotifierIcon,
			trackInfo, @"iTunesTrackInfo",
			[self starsForRating:[[trackInfo objectForKey:@"Rating"] integerValue]], QSNotifierDetails,	nil]);
	}
}

- (NSAttributedString *)starsForRating:(NSUInteger)rating
{
//	NSLog(@"rating %d", rating);
	if (rating == 0) return nil;
	NSString *string = @"";
	for (NSUInteger i = 0; i < rating; i += 20) {
        unsigned short starOrHalf = (rating - i == 10)?0xbd:0x2605;
		string = [string stringByAppendingFormat:@"%C", starOrHalf];
	}
    NSFont *starFont = [NSFont fontWithName:@"AppleGothic" size:20];
    if (!starFont) {
        starFont = [NSFont systemFontOfSize:20];
    }
    NSDictionary *attrs = @{NSFontNameAttribute: starFont};
	return [[NSAttributedString alloc] initWithString:string attributes:attrs];
}

- (NSArray *)recentTrackObjects {
	NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
	//NSDictionary *tracks = [[self iTunesMusicLibrary] objectForKey:@"Tracks"];
	
	for (NSDictionary *trackInfo in [self recentTracks]) {
//		currentTrack = [self trackInfoForID:trackID];
		if (trackInfo)
			[objects addObject:[[QSiTunesDatabase sharedInstance] trackObjectForInfo:trackInfo inPlaylist:nil]];
	}
	return objects; 	
}

- (BOOL)indexIsValidFromDate:(NSDate *)indexDate forEntry:(NSDictionary *)theEntry {
	NSDate *modDate = [[[NSFileManager defaultManager] attributesOfItemAtPath:[[[QSiTunesDatabase sharedInstance] libraryLocation] stringByResolvingSymlinksInPath] error:nil] fileModificationDate];
	return [modDate compare:indexDate] == NSOrderedAscending;
}

- (NSImage *)iconForEntry:(NSDictionary *)dict {
	return [QSResourceManager imageNamed:@"iTunesIcon"];
}

- (NSString *)identifierForObject:(QSObject *)object
{
	if ([[object primaryType] isEqualToString:QSiTunesPlaylistIDPboardType]) {
		//NSLog(@"getplaylistid");
		return nil;
		return [@"[iTunes Playlist] :" stringByAppendingString:[object objectForType:QSiTunesPlaylistIDPboardType]];
	}
	if ([[object primaryType] isEqualToString:QSiTunesBrowserPboardType]) {
		id value = [object objectForType:QSiTunesBrowserPboardType];
		if (!value) return nil;
		return [@"[iTunes Browser] :" stringByAppendingString:[[value allValues] componentsJoinedByString:@", "]];
	}
	return nil;
}



- (NSArray *)objectsForEntry:(NSDictionary *)theEntry {
	[[QSiTunesDatabase sharedInstance] loadMusicLibrary];
	if (![[QSiTunesDatabase sharedInstance] isLoaded]) return nil;
	
	NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
	
	//Tracks
	QSObject *newObject = nil;
	
	
	/*
	 if (fDEV) {
		 NSDictionary *tracks = [iTunesMusicLibrary objectForKey:@"Tracks"];
		 NSString *key;
		 NSDictionary *trackInfo;
		 NSImage *icon = [QSResourceManager imageNamed:@"iTunesTrackIcon"];
		 NSEnumerator *trackEnumerator = [tracks keyEnumerator];
		 while(key = [trackEnumerator nextObject]) {
			 trackInfo = [tracks objectForKey:key];
			 newObject = [QSObject objectWithName:[trackInfo objectForKey:@"Name"]];
			 [newObject setObject:key forKey:QSiTunesTrackIDPboardType];
			 [newObject setPrimaryType:QSiTunesTrackIDPboardType];
			 [newObject setIcon:icon];
			 [objects addObject:newObject];
		 }
	 }
	 */
	
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesMonitorTracks"]) {
		newObject = [QSObject objectWithName:@"Recent Tracks"];
		[newObject setObject:[NSDictionary dictionary] forType:QSiTunesBrowserPboardType];  
		[newObject setPrimaryType:QSiTunesBrowserPboardType];
		[newObject setIdentifier:QSiTunesRecentTracksBrowser];
		//[newObject setIcon:[QSResourceManager imageNamed:@"iTunesIcon"]];
		[newObject setObject:@"Recent" forMeta:kQSObjectIconName];
		
		[objects addObject:newObject];
		
	}
	
	[objects addObjectsFromArray:[self browseMasters]];
	
	//Playlists
	NSArray *playlists = [[QSiTunesDatabase sharedInstance] playlists];
	//NSLog(@"Loading %d Playlists", [playlists count]);
	//	if (![playlists count])   NSLog(@"Library Dump: %@", [self iTunesMusicLibrary]);
	
	NSString *label = nil;
	for (NSDictionary *thisPlaylist in playlists) {
		label = [thisPlaylist objectForKey:@"Name"];
		if ([[thisPlaylist objectForKey:@"Master"] boolValue] && [label isEqualToString:@"Library"]) {
			continue;
		}
		if ([[thisPlaylist objectForKey:@"Folder"] boolValue]) {
			// skip folders
			continue;
		}
        NSString *parentID = [thisPlaylist objectForKey:@"Parent Persistent ID"];
		if (parentID) {
			// this playlist is inside a folder - get the parent's name
            NSArray *playlistResult = [[[QSiTunesDatabase sharedInstance] playlists] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"%K == %@", @"Playlist Persistent ID", parentID]];
            if ([playlistResult count] > 0) {
                NSDictionary *parent = [playlistResult objectAtIndex:0];
                label = [label stringByAppendingFormat:@" (in %@)", [parent objectForKey:@"Name"]];
            }
		}
		
		newObject = [QSObject objectWithName:[label stringByAppendingString:@" Playlist"]];
		[newObject setLabel:label];
		[newObject setObject:[thisPlaylist objectForKey:@"Playlist ID"] forType:QSiTunesPlaylistIDPboardType];
		[newObject setIdentifier:[thisPlaylist objectForKey:@"Playlist Persistent ID"]];
		[newObject setPrimaryType:QSiTunesPlaylistIDPboardType];
		[objects addObject:newObject];
	}
	return objects;
}

// Object Handler Methods
- (void)setQuickIconForObject:(QSObject *)object {
	if ([[object primaryType] isEqualToString:QSiTunesPlaylistIDPboardType]) {
		NSDictionary *playlistDict = [[QSiTunesDatabase sharedInstance] playlistInfoForID:[object objectForType:QSiTunesPlaylistIDPboardType]];
		if ([playlistDict objectForKey:@"Smart Criteria"])
			[object setIcon:[QSResourceManager imageNamed:@"iTunesSmartPlaylistIcon"]];
		else if ([[playlistDict objectForKey:@"Name"] isEqualToString:@"Quicksilver"])
			[object setIcon:[QSResourceManager imageNamed:@"iTunesQuicksilverPlaylistIcon"]];
		else if ([[playlistDict objectForKey:@"Name"] isEqualToString:@"Purchased Music"])
			[object setIcon:[QSResourceManager imageNamed:@"iTunesPurchasedMusicPlaylistIcon"]];
		else 
			[object setIcon:[QSResourceManager imageNamed:@"iTunesPlaylistIcon"]];
		return;
	}
	
	if ([[object primaryType] isEqualToString:QSiTunesTrackIDPboardType])
		[object setIcon:[QSResourceManager imageNamed:@"iTunesTrackIcon"]];
	
	if ([[object primaryType] isEqualToString:QSiTunesBrowserPboardType]) {
		NSImage *icon = nil;
		NSDictionary *browseDict = [object objectForType:QSiTunesBrowserPboardType];
		NSString *displayType = [browseDict objectForKey:@"Type"];
		//NSDictionary *criteriaDict = [browseDict objectForKey:@"Criteria"];
		
		NSString *name = [NSString stringWithFormat:@"iTunes%@BrowserIcon", displayType];
		icon = [QSResourceManager imageNamed:name];
		if (!icon && [name isEqualToString:@"iTunesTrackBrowserIcon"]) icon = [QSResourceManager imageNamed:@"iTunesTrackIcon"];
		if (!icon) {
			icon = [QSResourceManager imageNamed:@"iTunesIcon"];
		}
		[object setIcon:icon];
		
		return;
	}
}

- (BOOL)loadIconForObject:(QSObject *)object {
    if (![[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesShowArtwork"]) {
        return NO;
    }
	if ([[object primaryType] isEqualToString:QSiTunesBrowserPboardType]) {
		NSDictionary *browseDict = [object objectForType:QSiTunesBrowserPboardType];
		NSString *displayType = [browseDict objectForKey:@"Type"];
		NSDictionary *criteriaDict = [browseDict objectForKey:@"Criteria"];
		NSString *album = [criteriaDict objectForKey:@"Album"];
		
		//	if (0 && [displayType isEqualToString:@"Genre"]) {
		//			NSImage *genreImage = imageForCommonGenre([criteriaDict objectForKey:@"Genre"]);
		//			if (genreImage) {
		//				[object setIcon:genreImage];
		//				return YES;
		//			}
		//		}
		if ([displayType isEqualToString:@"Album"] && album) {
			NSArray *valueArray = [[QSiTunesDatabase sharedInstance] tracksMatchingCriteria:criteriaDict];
            if ([valueArray count]) {
                // get the icon from the first non-video track
                for (NSDictionary *track in valueArray) {
                    if (![[track objectForKey:@"Has Video"] boolValue]
                        && ![[track objectForKey:@"Kind"] isEqualToString:@"iTunes LP"]) {
                        [object setIcon:[self imageForTrack:track]];
                        return YES;
                    }
                }
                // if they were all videos, just use the first one
                NSDictionary *iconTrack = [valueArray objectAtIndex:0];
                [object setIcon:[self imageForTrack:iconTrack]];
                return YES;
            }
		}
	} else if ([[object primaryType] isEqualToString:QSiTunesTrackIDPboardType]) {
		NSImage *icon = [self imageForTrack:[object objectForType:QSiTunesTrackIDPboardType]];
		if (icon) {
			[object setIcon:icon];
			return YES;
		}
	}
	return NO;
}

- (NSImage *)imageForTrack:(NSDictionary *)trackDict
{
	NSImage *icon = nil;
	NSSize iconSize = QSMaxIconSize;
	if ([trackDict objectForKey:@"Location"]) {
		NSString *URLString = [trackDict objectForKey:@"Location"];
		if (!URLString) return nil;
		NSURL *locationURL = [NSURL URLWithString:URLString];
		if ([[locationURL scheme] isEqualToString:@"file"]) {
			NSString *path = [locationURL path];
			BOOL shadowsAndGloss = ![[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesPlainArtwork"];
			icon = [NSImage imageWithPreviewOfFileAtPath:path ofSize:iconSize asIcon:shadowsAndGloss];
		} else if ([[self iTunes] isRunning] && [[self iTunes] currentStreamURL]) {
			NSURL *artworkURL = [NSURL URLWithString:[[self iTunes] currentStreamURL]];
			icon = [[NSImage alloc] initWithContentsOfURL:artworkURL];
		}
	} else if ([[self iTunes] isRunning]) {
		// get image with Scripting Bridge
		NSString *searchFilter = @"persistentID == %@";
		NSArray *trackID = trackDict[@"Persistent ID"];
		SBElementArray *tracks = (SBElementArray *)[[QSiTunesMusic() tracks] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:searchFilter, trackID]];
		if ([tracks count]) {
			iTunesTrack *track = tracks[0];
			if ([track artworks]) {
				icon = [[[track artworks] objectAtIndex:0] data];
				if (![icon isKindOfClass:[NSImage class]]) {
					icon = nil;
				}
			}
		}
	}
	return icon;
}

- (BOOL)objectHasChildren:(QSObject *)object
{
	if ([[object primaryType] isEqualToString:QSiTunesPlaylistIDPboardType]) {
		NSDictionary *playlistDict = [[QSiTunesDatabase sharedInstance] playlistInfoForID:[object objectForType:QSiTunesPlaylistIDPboardType]];
		return [(NSArray *)[playlistDict objectForKey:@"Playlist Items"] count];
	} else if ([[object primaryType] isEqualToString:QSiTunesBrowserPboardType]) {
		return YES;  
	} else if ([[object primaryType] isEqualToString:QSiTunesTrackIDPboardType]) {
		return YES;
	}
	return NO;
}
- (BOOL)objectHasValidChildren:(QSObject *)object
{
	return YES;
}

- (NSString *)detailsOfObject:(QSObject *)object {
	NSString *details = nil;
	if ([[object primaryType] isEqualToString:QSiTunesPlaylistIDPboardType]) {
		// Playlist details
		NSDictionary *info = [[QSiTunesDatabase sharedInstance] playlistInfoForID:[object objectForType:QSiTunesPlaylistIDPboardType]];
		NSUInteger count = [(NSArray *)[info objectForKey:@"Playlist Items"] count];
		if (count) {
            NSString *ess = (count == 1) ? @"" : @"s";
			details = [NSString stringWithFormat:@"%d track%@", (int)count, ess];
			return details;
		}
	} else if ([[object primaryType] isEqualToString:QSiTunesBrowserPboardType]) {
		// Browser item details
		details = [[[[object objectForType:QSiTunesBrowserPboardType] objectForKey:@"Criteria"] allValues] componentsJoinedByString:[NSString stringWithFormat:@" %C ", (unsigned short)0x25B8]];
		if (![details isEqualToString:[object displayName]]) {
			return details;
		}
	} else if ([[object primaryType] isEqualToString:QSiTunesTrackIDPboardType]) {
		// Track details
		NSDictionary *info = [object objectForType:QSiTunesTrackIDPboardType];
		NSString *artist = [info objectForKey:@"Artist"];
		NSString *album = [info objectForKey:@"Album"];
		NSString *year = [info objectForKey:@"Year"];
		details = artist;
		if (album) {
			details = [details stringByAppendingFormat:@" - %@", album];
		}
		if (year) {
			details = [details stringByAppendingFormat:@" (%@)", year];
		}
		return details;
	} else if ([[object primaryType] isEqualToString:QSProxyType]) {
		QSObject *random = [object resolvedObject];
		if ([[random primaryType] isEqualToString:QSiTunesTrackIDPboardType]) {
			NSDictionary *info = [random objectForType:QSiTunesTrackIDPboardType];
			NSString *artist = [info objectForKey:@"Artist"];
			NSString *title = [info objectForKey:@"Name"];
			return [NSString stringWithFormat:@"%@, by %@", title, artist];
		}
	}
	return nil;
}

- (NSArray *)iTunesGetChildren
{
	// this repeats the list created in objectsForEntry, but executes MUCH faster than re-running that method
	NSMutableArray *children = [NSMutableArray arrayWithCapacity:1];
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesMonitorTracks"]) {
		QSObject *recents = [QSLib objectWithIdentifier:QSiTunesRecentTracksBrowser];
		if (recents) {
			[children addObject:[QSLib objectWithIdentifier:QSiTunesRecentTracksBrowser]];
		}
	}
	QSAction *showCurrentTrack = (QSAction *)[QSLib objectWithIdentifier:@"QSiTunesShowCurrentTrack"];
	if (showCurrentTrack) {
		[children addObject:showCurrentTrack];
	}
	[children addObjectsFromArray:[self browseMasters]];
	[children addObjectsFromArray:[QSLib arrayForType:QSiTunesPlaylistIDPboardType]];
	return children;
}

- (BOOL)loadChildrenForBundle:(QSObject *)object {
	NSArray *children = [self iTunesGetChildren];
	if (children) {
		[object setChildren:children];
		return YES;  
	}
	return NO;
}

- (BOOL)loadChildrenForObject:(QSObject *)object {
	if ([[object primaryType] isEqualToString:QSFilePathType]) {
		NSArray *children = [self iTunesGetChildren];
		[object setChildren:children];
		return YES; 	
	}
	
	NSArray *children = [self childrenForObject:object];
	
	if (children) {
		[object setChildren:children];
		return YES;  
	}
	return NO;
}

- (QSObject *)browserObjectForTrack:(NSDictionary *)trackDict andCriteria:(NSString *)rootType {
	NSString *value = nil;
	if ([rootType isEqualToString:@"Artist"] && [trackDict objectForKey:@"Album Artist"]) {
		value = [trackDict objectForKey:@"Album Artist"];
	} else {
		value = [trackDict objectForKey:rootType];
	}
	if (!value) return nil;
	id newObject;
	NSMutableDictionary *childCriteria = [NSMutableDictionary dictionaryWithObjectsAndKeys:
		[NSDictionary dictionaryWithObject:value forKey:rootType] , @"Criteria",
		[[QSiTunesDatabase sharedInstance] nextSortForCriteria:rootType] , @"Result", rootType, @"Type", nil];
	newObject = [QSObject objectWithName:value];
	[newObject setObject:childCriteria forType:QSiTunesBrowserPboardType];  
	[newObject setPrimaryType:QSiTunesBrowserPboardType];
	return newObject;
}

- (NSArray *)childrenForObject:(QSObject *)object {
	if ([[object primaryType] isEqualToString:QSiTunesTrackIDPboardType]) {
		
		NSDictionary *trackDict = [object objectForType:QSiTunesTrackIDPboardType];
		
		NSArray *sortTags = [NSArray arrayWithObjects:@"Genre", @"Artist", @"Composer", @"Album", nil]; 	
		
		
		NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
		QSObject *newObject = nil;
		for (NSString *rootType in sortTags) {
			newObject = [self browserObjectForTrack:trackDict andCriteria:rootType];
			if (newObject)
				[objects addObject:newObject];
		}
		return objects;
		
		
	} else  if ([[object primaryType] isEqualToString:QSiTunesPlaylistIDPboardType]) {
		NSDictionary *playlistDict = [[QSiTunesDatabase sharedInstance] playlistInfoForID:[object objectForType:QSiTunesPlaylistIDPboardType]];
		NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
		
		NSArray *trackIDs = [[playlistDict objectForKey:@"Playlist Items"] valueForKeyPath:@"Track ID.stringValue"];
		
		NSArray *tracks = [[QSiTunesDatabase sharedInstance] trackInfoForIDs:trackIDs];
		for (NSDictionary *currentTrack in tracks) {
			id object = [[QSiTunesDatabase sharedInstance] trackObjectForInfo:currentTrack inPlaylist:[playlistDict objectForKey:@"Playlist Persistent ID"]];
			if (object) [objects addObject:object];
			else NSLog(@"Ignoring Track %@", currentTrack);
		}
		return objects;
	} else if ([[object primaryType] isEqualToString:QSiTunesBrowserPboardType]) {
		if ([[object identifier] isEqualToString:QSiTunesRecentTracksBrowser])
			return [self recentTrackObjects];
		
		NSArray *criterion = [object arrayForType:QSiTunesBrowserPboardType];
		NSMutableArray *children = [NSMutableArray array];
		for (NSDictionary *criteria in criterion) {
			[children addObjectsFromArray:[self childrenForBrowseCriteria:criteria]];
		}
		return children;
	}
	return nil;
}

- (NSArray *)childrenForBrowseCriteria:(NSDictionary *)browseDict {
	NSDictionary *criteriaDict = [browseDict objectForKey:@"Criteria"];
	NSString *displayType = [browseDict objectForKey:@"Result"];
	
	NSArray *trackArray = [[QSiTunesDatabase sharedInstance] tracksMatchingCriteria:criteriaDict];
	
	//NSLog(@"%d items", [trackArray count]);
	
	NSDictionary *nextSort = [NSDictionary dictionaryWithObjectsAndKeys:
		@"Artist", @"Genre",
		@"Album", @"Artist",
		@"Album", @"Composer",
		@"Track", @"Album",
		@"Year", @"Album",
		nil];
	
	
	if (displayType) {
		if ([displayType isEqualToString:@"Track"]) {
			NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];  
			NSMutableArray *descriptors = [NSMutableArray array];
			//NSLog(@"count %d", [trackArray count]);
			if ([[criteriaDict allKeys] containsObject: @"Artist"] || [[criteriaDict allKeys] containsObject: @"Album"]) {
				[descriptors addObject:[[NSSortDescriptor alloc] initWithKey:@"Album" ascending:YES]];
				[descriptors addObject:[[NSSortDescriptor alloc] initWithKey:@"Disc Number" ascending:YES]];
				[descriptors addObject:[[NSSortDescriptor alloc] initWithKey:@"Track Number" ascending:YES]];
			} else {
				[descriptors addObject:[[NSSortDescriptor alloc] initWithKey:@"Name" ascending:YES]];
			}
			NSArray *sortedTracks = [trackArray sortedArrayUsingDescriptors:descriptors];
			for (NSDictionary *trackInfo in sortedTracks) {
				[objects addObject:[[QSiTunesDatabase sharedInstance] trackObjectForInfo:trackInfo inPlaylist:nil]];
			}
			
			return objects;
			
		} else {
			NSArray *subsets = [[QSiTunesDatabase sharedInstance] objectsForKey:displayType inArray:trackArray];
			
			NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
			NSMutableArray *usedKeys = [NSMutableArray arrayWithCapacity:1];
			
			QSObject *newObject = nil;
			NSString *childSort = [nextSort objectForKey:displayType];
			if (!childSort)
				childSort = @"Track";
			
			NSMutableDictionary *childBrowseDict;
			 {
				childBrowseDict = [browseDict mutableCopy];
				if (!childBrowseDict) childBrowseDict = [NSMutableDictionary dictionaryWithCapacity:1];
				[childBrowseDict setObject:childSort forKey:@"Result"];
				[childBrowseDict setObject:displayType forKey:@"Type"];
				
				NSString *details = [[[childBrowseDict objectForKey:@"Criteria"] allValues] componentsJoinedByString:[NSString stringWithFormat:@" %C ", (unsigned short)0x25B8]];
				NSString *name = [NSString stringWithFormat:@"All %@s", displayType];
				
				if (details) name = [name stringByAppendingFormat:@" (%@) ", details];
				newObject = [QSObject objectWithName:name];
				[newObject setObject:childBrowseDict forType:QSiTunesBrowserPboardType];
				[newObject setPrimaryType:QSiTunesBrowserPboardType];
				[objects addObject:newObject];  
			}
			
			for (NSString *thisItem in subsets) {
				if ([usedKeys containsObject:[thisItem lowercaseString]]) continue;
				childBrowseDict = [NSMutableDictionary dictionaryWithCapacity:1];
				
				NSMutableDictionary *childCriteriaDict = [criteriaDict mutableCopy];
				if (!childCriteriaDict) childCriteriaDict = [NSMutableDictionary dictionaryWithCapacity:1];
				[childCriteriaDict setObject:thisItem forKey:displayType];
				
				[childBrowseDict setObject:displayType forKey:@"Type"];
				[childBrowseDict setObject:childSort forKey:@"Result"];
				[childBrowseDict setObject:childCriteriaDict forKey:@"Criteria"];
				newObject = [QSObject objectWithName:thisItem];
				[newObject setObject:childBrowseDict forType:QSiTunesBrowserPboardType];  
				[newObject setPrimaryType:QSiTunesBrowserPboardType];
				
				[usedKeys addObject:[thisItem lowercaseString]];
				[objects addObject:newObject];  
			}
			
			return objects;
			
		}
		
	} else {
		return [self browseMasters];
	}
}

- (NSArray *)browseMasters {
	NSArray *sortTags = [NSArray arrayWithObjects:@"Genre", @"Artist", @"Composer", @"Album", @"Track", nil]; 	
	NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
	QSObject *newObject = nil;
	for (NSString *rootType in sortTags) {
		NSMutableDictionary *childCriteria = [NSMutableDictionary dictionaryWithObjectsAndKeys:rootType, @"Result", rootType, @"Type", nil];
		newObject = [QSObject objectWithName:[NSString stringWithFormat:@"Browse %@s", rootType]];
		[newObject setObject:childCriteria forType:QSiTunesBrowserPboardType];  
		[newObject setPrimaryType:QSiTunesBrowserPboardType];
		[objects addObject:newObject];
	}
	return objects;
}

#pragma mark Scripting Bridge delegate

- (id)eventDidFail:(const AppleEvent *)event withError:(NSError *)error
{
    NSLog(@"iTunes Communication Error: %@", [error localizedDescription]);
    return nil;
}

@end

@implementation QSiTunesControlSource

- (instancetype)init
{
    self = [super init];
    if (self) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(checkForRelaunch:) name:@"QSProcessMonitorApplicationLaunched" object:nil];
    }
    return self;
}

- (void)checkForRelaunch:(NSNotification *)note {
    NSString *launchedApp = note.userInfo[@"NSApplicationBundleIdentifier"];
    if ([launchedApp isEqualToString:@"com.apple.iTunes"]) {
        // rescan
        [[NSNotificationCenter defaultCenter] postNotificationName:QSCatalogSourceInvalidated object:@"QSiTunesControlSource"];
        [[NSNotificationCenter defaultCenter] postNotificationName:QSCatalogSourceInvalidated object:@"QSiTunesEQPresets"];
        [[NSNotificationCenter defaultCenter] postNotificationName:QSCatalogSourceInvalidated object:@"QSiTunesAirPlayDevices"];
    }
}

- (BOOL)indexIsValidFromDate:(NSDate *)indexDate forEntry:(NSDictionary *)theEntry {
	// rescan only if the indexDate is prior to the last launch
	NSDate *launched = [[NSRunningApplication currentApplication] launchDate];
	if (launched) {
		return ([launched compare:indexDate] == NSOrderedAscending);
	} else {
		// Quicksilver wasn't launched by LaunchServices - date unknown - rescan to be safe
		return NO;
	}
}

- (BOOL)entryCanBeIndexed:(NSDictionary *)theEntry
{
    // make sure controls are rescanned on every launch, not read from disk
    return NO;
}

- (NSArray *)objectsForEntry:(NSDictionary *)theEntry
{
	NSMutableArray *controlObjects = [NSMutableArray arrayWithCapacity:1];
	QSCommand *command = nil;
	NSDictionary *commandDict = nil;
	QSAction *newObject = nil;
	NSString *actionID = nil;
	NSDictionary *actionDict = nil;
	// create catalog objects using info specified in the plist (under QSCommands)
	NSArray *controls = [NSArray arrayWithObjects:@"QSiTunesShowTrackNotification", @"QSiTunesPlayPauseCommand", @"QSiTunesPlayCommand", @"QSiTunesPauseCommand", @"QSiTunesStopCommand", @"QSiTunesIncreaseVolume", @"QSiTunesDecreaseVolume", @"QSiTunesMute", @"QSiTunesPreviousSongCommand", @"QSiTunesNextSongCommand", @"QSiTunesIncreaseRating", @"QSiTunesDecreaseRating", @"QSiTunesSetRating0", @"QSiTunesSetRating1", @"QSiTunesSetRating2", @"QSiTunesSetRating3", @"QSiTunesSetRating4", @"QSiTunesSetRating5", @"QSiTunesEQToggleCommand", nil];
	for (NSString *control in controls) {
		command = [QSCommand commandWithIdentifier:control];
		if (command) {
			commandDict = [command commandDict];
			actionID = [commandDict objectForKey:@"directID"];
			actionDict = [[[commandDict objectForKey:@"directArchive"] objectForKey:@"data"] objectForKey:QSActionType];
			if (actionDict) {
				newObject = [QSAction actionWithDictionary:actionDict identifier:actionID];
				[controlObjects addObject:newObject];
			}
		}
	}
	return controlObjects;
}

@end

@implementation QSiTunesEQPresets

- (BOOL)indexIsValidFromDate:(NSDate *)indexDate forEntry:(NSDictionary *)theEntry
{
	NSString *sourceFile = [@"~/Library/Preferences/com.apple.iTunes.eq.plist" stringByExpandingTildeInPath];
	// get the last modified date on the source file
	NSFileManager *manager = [NSFileManager defaultManager];
	if (![manager fileExistsAtPath:sourceFile isDirectory:NULL]) {
		return YES;
	}
	NSDate *modDate = [[manager attributesOfItemAtPath:sourceFile error:NULL] fileModificationDate];
	// compare dates and return whether or not the entry should be rescanned
	if ([indexDate compare:modDate] == NSOrderedAscending) {
		NSLog(@"rescanning EQ presets");
		return NO;
	}
	// don't rescan by default
	return YES;
}

- (BOOL)entryCanBeIndexed:(NSDictionary *)theEntry
{
    return NO;
}

- (NSArray *)objectsForEntry:(NSDictionary *)theEntry
{
	iTunesApplication *iTunes = QSiTunes();
	if ([iTunes isRunning]) {
		NSArray *EQPresets = [[iTunes EQPresets] get];
		QSObject *newObject = nil;
		NSMutableArray *objects = [NSMutableArray arrayWithCapacity:[EQPresets count]];
		for (iTunesEQPreset *eq in EQPresets) {
			NSString *name = [eq name];
			newObject = [QSObject makeObjectWithIdentifier:[NSString stringWithFormat:@"iTunes Preset:%@", name]];
			[newObject setName:[NSString stringWithFormat:@"%@ Equalizer Preset", name]];
			[newObject setDetails:@"iTunes Equalizer Preset"];
			[newObject setObject:eq forType:QSiTunesEQPresetType];
			[objects addObject:newObject];
		}
		return objects;
	}
	return nil;
}

- (void)setQuickIconForObject:(QSObject *)object
{
	[object setIcon:[QSResourceManager imageNamed:@"iTunesEQ"]];
}

@end

@implementation QSiTunesAirPlayDevices

- (BOOL)indexIsValidFromDate:(NSDate *)indexDate forEntry:(NSDictionary *)theEntry
{
    // rescan only if the indexDate is prior to the last launch
    NSDate *launched = [[NSRunningApplication currentApplication] launchDate];
    if (launched) {
        return ([launched compare:indexDate] == NSOrderedAscending);
    } else {
        // Quicksilver wasn't launched by LaunchServices - date unknown - rescan to be safe
        return NO;
    }
}

- (BOOL)entryCanBeIndexed:(NSDictionary *)theEntry
{
    // make sure devices are rescanned on every launch, not read from disk
    return NO;
}

- (NSArray *)objectsForEntry:(NSDictionary *)theEntry
{
    iTunesApplication *iTunes = QSiTunes();
    if ([iTunes isRunning]) {
        NSArray *AirPlayDevices = [[iTunes AirPlayDevices] get];
        QSObject *newObject = nil;
        NSMutableArray *objects = [NSMutableArray arrayWithCapacity:[AirPlayDevices count]];
        for (iTunesAirPlayDevice *apd in AirPlayDevices) {
            NSString *name = [apd name];
            newObject = [QSObject makeObjectWithIdentifier:[NSString stringWithFormat:@"iTunes AirPlay:%@", name]];
            [newObject setName:name];
            [newObject setDetails:@"iTunes AirPlay Device"];
            [newObject setObject:apd forType:QSiTunesAirPlayDeviceType];
            [objects addObject:newObject];
        }
        return objects;
    }
    return nil;
}

@end
