#import <Carbon/Carbon.h>
#import "QSiTunesSource.h"
#import "QSiTunesDefines.h"
#define QSiTunesRecentTracksBrowser @"QSiTunesRecentTracksBrowser"

@implementation QSiTunesObjectSource
mSHARED_INSTANCE_CLASS_METHOD

+ (void)initialize {
	NSImage *image;
	NSArray *images = [[NSArray arrayWithObjects:@"iTunesLibraryPlaylistIcon", @"iTunesSmartPlaylistIcon",
		@"iTunesPlaylistIcon", @"iTunesPartyShufflePlaylistIcon", @"iTunesPurchasedMusicPlaylistIcon", @"iTunesQuicksilverPlaylistIcon", @"iTunesAlbumBrowserIcon",
		@"iTunesArtistBrowserIcon", @"iTunesComposerBrowserIcon", @"iTunesGenreBrowserIcon", nil] objectEnumerator];
	NSBundle *bundle = [NSBundle bundleForClass:[QSiTunesObjectSource class]];
	for (NSString *name in images) {
		image = [[NSImage alloc] initWithContentsOfFile:[bundle pathForImageResource:name]];
		[image setName:name];
		[image createIconRepresentations];
	}
}
- (void)dealloc {
	[super dealloc];
}
- (id)init {
	if (self = [super init]) {
		//[[QSVoyeur sharedInstance] addPathToQueue:[self libraryLocation]];
		
		NS_DURING
			[self bind:@"showArtwork"
			  toObject:[NSUserDefaultsController sharedUserDefaultsController]
		   withKeyPath:@"values.QSiTunesShowArtwork"
			   options:nil];
		NS_HANDLER
			;
		NS_ENDHANDLER
		
		//      [(QSObjectCell *)theCell setImagePosition:NSImageBelow];
		
		library = [[QSiTunesDatabase alloc] init];
		
		recentTracks = nil;
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesMonitorTracks"]) {
			recentTracks = [[NSMutableArray array] retain];
			//iTunesMonitorDeactivate();
			//iTunesMonitorActivate([[NSBundle bundleForClass:[self class]]pathForResource:@"iTunesMonitor" ofType:@""]);
			
			
//			int thisID = iTunesGetCurrentTrack();
//			if (thisID > 0)
//				[recentTracks addObject:[NSString stringWithFormat:@"%d", thisID]];
//			
			[[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(iTunesStateChanged:) name:@"com.apple.iTunes.playerInfo" object:nil];
			//	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(quitMonitor:) name:NSApplicationWillTerminateNotification object:nil];
			
		}
	}
	return self;
}
//-(void)quitMonitor:(NSNotification *)notif {
//	iTunesMonitorDeactivate();
//	
//}


- (void)iTunesStateChanged:(NSNotification *)notif {
	if ([[[notif userInfo] objectForKey:@"Player State"] isEqualToString:@"Playing"]) {
		
		NSString *newTrack = [self currentTrackID];
		if ([newTrack intValue] <= 0) return;
		[recentTracks insertObject:[notif userInfo] atIndex:0];
		while ([recentTracks count] > 10) [recentTracks removeLastObject];
		
		
		NSDictionary *trackInfo = nil;
		if (!trackInfo) {
			trackInfo = [notif userInfo];
			[library registerAdditionalTrack:trackInfo forID:newTrack];
		}
		
		[self showNotificationForTrack:newTrack info:trackInfo];
		[[QSObject objectWithIdentifier:QSiTunesRecentTracksBrowser] unloadChildren];
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
	if (!QSAppIsRunning(@"com.apple.iTunes") )
		return nil;
	if ([proxy isEqualToString:@"QSCurrentTrackProxy"]) {
		id object = [self trackObjectForInfo:[self currentTrackInfo]
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
		NSString *name = [[QSiTunes() currentPlaylist] name];
		NSDictionary *thisPlaylist = [library playlistInfoForName:name];
		
		QSObject *newObject = [QSObject objectWithName:name];
		[newObject setObject:[thisPlaylist objectForKey:@"Playlist ID"] forType:QSiTunesPlaylistIDPboardType];
		[newObject setIdentifier:[thisPlaylist objectForKey:@"Playlist Persistent ID"]];
		[newObject setPrimaryType:QSiTunesPlaylistIDPboardType];
		return newObject;
	} else if ([proxy isEqualToString:@"QSSelectedPlaylistProxy"]) {
		iTunesBrowserWindow *window = [[QSiTunes() browserWindows] objectAtIndex:0];
		NSString *name = [[window view] name];
		NSDictionary *thisPlaylist = [library playlistInfoForName:name];
		
		QSObject *newObject = [QSObject objectWithName:name];
		[newObject setObject:[thisPlaylist objectForKey:@"Playlist ID"] forType:QSiTunesPlaylistIDPboardType];
		[newObject setIdentifier:[thisPlaylist objectForKey:@"Playlist Persistent ID"]];
		[newObject setPrimaryType:QSiTunesPlaylistIDPboardType];
		return newObject;
		
		
	} else {// if ([ident isEqualToString:@"QSCurrentSelectionProxy"]) {
		// TODO get current iTunes selection with Scripting Bridge
//		iTunesBrowserWindow *window = [[QSiTunes() browserWindows] objectAtIndex:0];
//		SBObject *listing = [window selection];
//		NSArray *tracks = [[listing elementArrayWithCode:iTunesEKndTrackListing] arrayByPerformingSelector:@selector(databaseID)];
//		NSLog(@"iTunes selection count: %@", [tracks count]);
		NSArray *tracks = [[[self iTunesScript] executeSubroutine:@"current_selection_id" arguments:nil  error:nil] objectValue];
		if ([tracks isKindOfClass:[NSNull class]]) return nil;
		tracks = [library trackInfoForIDs:tracks];
		NSMutableArray *objects = [NSMutableArray array];
		for (NSDictionary *track in tracks) {
			[objects addObject:[self trackObjectForInfo:track inPlaylist:nil]];
		}
		return [QSObject objectByMergingObjects:objects];
		
	}
	return nil;
}

- (NSDictionary *)trackInfoForID:(id)trackID
{
	NSMutableDictionary *trackInfo = nil;
	// check the in-memory database first
	id libraryTrackInfo;
	if (libraryTrackInfo = [library trackInfoForID:trackID]) {
		trackInfo = libraryTrackInfo;
	}
	if (!trackInfo) {
		// fall back to querying iTunes (if it's running)
		if ([QSiTunes() isRunning]) {
			iTunesLibraryPlaylist *libraryPlaylist = [[QSiTunesLibrary() libraryPlaylists] objectAtIndex:0];
			NSArray *trackResult = [[libraryPlaylist fileTracks] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"databaseID == %@", trackID]];
			if ([trackResult count] > 0) {
				iTunesFileTrack *track = [trackResult lastObject];
				[trackInfo setObject:[track persistentID] forKey:@"PersistentID"];
				[trackInfo setObject:[track artist] forKey:@"Artist"];
				[trackInfo setObject:[track albumArtist] forKey:@"Album Artist"];
				[trackInfo setObject:[track composer] forKey:@"Composer"];
				[trackInfo setObject:[track genre] forKey:@"Genre"];
				[trackInfo setObject:[track album] forKey:@"Album"];
				[trackInfo setObject:[track name] forKey:@"Name"];
				[trackInfo setObject:[NSNumber numberWithLong:[track rating]] forKey:@"Rating"];
				[trackInfo setObject:[NSString stringWithFormat:@"%d", [track albumRating]] forKey:@"Album Rating"];
				[trackInfo setObject:[track kind] forKey:@"Kind"];
				[trackInfo setObject:[NSNumber numberWithInt:1] forKey:@"Artwork Count"];
				[trackInfo setObject:[track location] forKey:@"Location"];
				[trackInfo setObject:[NSString stringWithFormat:@"%d", [track playedCount]] forKey:@"Play Count"];
				[trackInfo setObject:[track playedDate] forKey:@"Play Date"];
				[trackInfo setObject:[NSString stringWithFormat:@"%d", [track skippedCount]] forKey:@"Skip Count"];
				[trackInfo setObject:[track time] forKey:@"Total Time"];
				[trackInfo setObject:[NSString stringWithFormat:@"%d", [track trackNumber]] forKey:@"Track Number"];
				[trackInfo setObject:[NSString stringWithFormat:@"%d", [track year]] forKey:@"Year"];
			}
		}
	}
	return trackInfo;
}

- (id)currentTrackInfo {
	return [self trackInfoForID:[self currentTrackID]];
}

- (NSString *)currentTrackID {
	return [NSString stringWithFormat:@"%d", [[QSiTunes() currentTrack] databaseID]];
}

- (void)showCurrentTrackNotification {	
	[self showNotificationForTrack:0 info:[self currentTrackInfo]];
}

- (void)showNotificationForTrack:(id)trackID info:(NSDictionary *)trackInfo {
	if (!trackInfo) {
		trackInfo = [self trackInfoForID:trackID];
	}
	
	QSObject *track = [self trackObjectForInfo:trackInfo inPlaylist:nil];
	
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
			NSString *streamTitle = [QSiTunes() currentStreamTitle];
			
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
		NSString *text = [array componentsJoinedByString:@"\r"];

		
		NSImage *icon = nil;
		icon = [self imageForTrack:trackInfo useNet:YES];
		//NSLog(@"info : %@", trackInfo);
		if (!icon && ![trackInfo objectForKey:@"Location"]) {
			iTunesTrack *currentTrack = [QSiTunes() currentTrack];
			NSData *data = [[[currentTrack artworks] objectAtIndex:0] rawData];
			icon = [[[NSImage alloc] initWithData:data] autorelease];
		}
			
			
		if (!icon) icon = [QSResourceManager imageNamed:@"com.apple.iTunes"];
		
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesNotifyTracks"]) {
			//NSLog(@" >> >> >> >>>%@ %@ %@", name, text, icon);
			
			
			
			QSShowNotifierWithAttributes([NSDictionary dictionaryWithObjectsAndKeys:
				@"QSiTunesTrackChangeNotification", QSNotifierType,
				name, QSNotifierTitle,
				text, QSNotifierText,
				icon, QSNotifierIcon,
				trackInfo, @"iTunesTrackInfo",
				[self starsForRating:[[trackInfo objectForKey:@"Rating"] intValue]], QSNotifierDetails,
				nil]);
		}
		
	}
}

- (NSAttributedString *)starsForRating:(int)rating {
//	NSLog(@"rating %d", rating);
	if (rating == 0) return nil;
	NSString *string = [NSString stringWithFormat:@"%C%C%C%C%C",
								rating > 1?0x2606:0x2022,
							   rating > 20?0x2606:0x2022,
							   rating > 40?0x2606:0x2022,
							   rating > 60?0x2606:0x2022,
							   rating > 80?0x2606:0x2022];
	return [[[NSAttributedString alloc] initWithString:string
										  attributes:[NSDictionary dictionaryWithObject:[NSFont fontWithName:@"AppleGothic" size:20] forKey:NSFontNameAttribute]]autorelease];
}


- (NSArray *)recentTrackObjects {
	NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
	//NSDictionary *tracks = [[self iTunesMusicLibrary] objectForKey:@"Tracks"];
	
	for (NSDictionary *trackInfo in recentTracks) {
//		currentTrack = [self trackInfoForID:trackID];
		if (trackInfo)
			[objects addObject:[self trackObjectForInfo:trackInfo inPlaylist:nil]];
	}
	return objects; 	
}

- (BOOL)indexIsValidFromDate:(NSDate *)indexDate forEntry:(NSDictionary *)theEntry {
	NSDate *modDate = [[[NSFileManager defaultManager] fileAttributesAtPath:[library libraryLocation] traverseLink:YES] fileModificationDate];
	return [modDate compare:indexDate] == NSOrderedAscending;
}

- (NSImage *)iconForEntry:(NSDictionary *)dict {
	return [QSResourceManager imageNamed:@"iTunesIcon"];
}

- (NSString *)identifierForObject:(id <QSObject>)object {
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
	[library loadMusicLibrary];
	if (![library isLoaded]) return nil;
	
	NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
	
	//Tracks
	QSObject *newObject;
	
	
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
	
	NSDictionary *actionDict = [NSDictionary dictionaryWithObjectsAndKeys:
		NSStringFromClass([self class]), kActionClass,
		@"showCurrentTrackNotification", kActionSelector,
		@"iTunesIcon", kActionIcon,
		@"Show Playing Track", @"name",
		nil];
	
	newObject = [QSAction actionWithDictionary:actionDict
								  identifier:@"QSiTunesShowCurrentTrack"
									  bundle:nil];
	
	//[newObject setName:@"Show Playing Track"];
	
	
	[objects addObject:newObject];
	
	
	[objects addObjectsFromArray:[self browseMasters]];
	
	//Playlists
	NSArray *playlists = [library playlists];
	//NSLog(@"Loading %d Playlists", [playlists count]);
	//	if (![playlists count])   NSLog(@"Library Dump: %@", [self iTunesMusicLibrary]);
	
	for (NSDictionary *thisPlaylist in playlists) {
		NSString *name = [thisPlaylist objectForKey:@"Name"];
		if ([thisPlaylist objectForKey:@"Master"] && [name isEqualToString:@"Library"])
			name = @"Music Library";
		else
			name = [name stringByAppendingString:@" Playlist"];
		
		
		newObject = [QSObject objectWithName:name];
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
		NSDictionary *playlistDict = [library playlistInfoForID:[object objectForType:QSiTunesPlaylistIDPboardType]];
		if ([[playlistDict objectForKey:@"Master"] boolValue])
			[object setIcon:[NSImage imageNamed:@"iTunesLibraryPlaylistIcon"]];
		else if ([[playlistDict objectForKey:@"Folder"] boolValue])
			[object setIcon:[QSResourceManager imageNamed:@"GenericFolderIcon"]];
		else if ([playlistDict objectForKey:@"Smart Criteria"])
			[object setIcon:[NSImage imageNamed:@"iTunesSmartPlaylistIcon"]];
		else if ([[playlistDict objectForKey:@"Name"] isEqualToString:@"iTunes DJ"])
			[object setIcon:[NSImage imageNamed:@"iTunesPartyShufflePlaylistIcon"]];
		else if ([[playlistDict objectForKey:@"Name"] isEqualToString:@"Quicksilver"])
			[object setIcon:[NSImage imageNamed:@"iTunesQuicksilverPlaylistIcon"]];
		else if ([[playlistDict objectForKey:@"Name"] isEqualToString:@"Purchased Music"])
			[object setIcon:[NSImage imageNamed:@"iTunesPurchasedMusicPlaylistIcon"]];
		else 
			[object setIcon:[NSImage imageNamed:@"iTunesPlaylistIcon"]];
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
		icon = [NSImage imageNamed:name];
		if (!icon && [name isEqualToString:@"iTunesTrackBrowserIcon"]) icon = [QSResourceManager imageNamed:@"iTunesTrackIcon"];
		if (!icon) {
			icon = [QSResourceManager imageNamed:@"iTunesIcon"];
		}
		[object setIcon:icon];
		
		return;
	}
}

- (BOOL)loadIconForObject:(QSObject *)object {
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
		if ([displayType isEqualToString:@"Album"] && album && ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesShowArtwork"]) ) {
			NSDictionary *typeDicts = [[library tagDictionaries] objectForKey:@"Album"];
			NSArray *valueArray = [typeDicts objectForKey:[album lowercaseString]];
			NSDictionary *firstTrack = [valueArray objectAtIndex:0];
			[object setIcon:[self imageForTrack:firstTrack]];
			return YES;
		}
	} else if ([[object primaryType] isEqualToString:QSiTunesTrackIDPboardType]) {
		NSImage *icon = nil;
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesShowArtwork"]) icon = [self imageForTrack:[object objectForType:QSiTunesTrackIDPboardType]];
		if (icon) {
			[object setIcon:icon];
			return YES;
		}
	}
	return NO;
}


/*
 Attached picture   "PIC"
 Frame size         $xx xx xx
 Text encoding      $xx
 Image format       $xx xx xx
 Picture type       $xx
 Description        <textstring > $00 (00)
 Picture data       <binary data>
 */
- (NSImage *)imageForTrack:(NSDictionary *)trackDict {return [self imageForTrack:trackDict useNet:NO /*FIXME*/];}

- (NSImage *)imageForTrack:(NSDictionary *)trackDict useNet:(BOOL)useNet {
	//return nil;

	NSImage *icon = nil;
	if ([trackDict objectForKey:@"Location"]) {
		//NSString *kind = [trackDict objectForKey:@"Kind"];
		NSString *URLString = [trackDict objectForKey:@"Location"];
		
		if (!URLString) return nil;
		NSString *path = [[NSURL URLWithString:URLString] path];
		
#if QSAlchemy
		icon = [[QSReg instanceForPointID:@"QSFSFileTypePreviewers" withID:[path pathExtension]] iconForFile:path
                                                                                                ofType:[path pathExtension]];
#else
		icon = [[QSReg getClassInstance:@"QSMusicFilePreviewProvider"] iconForFile:path ofType:[path pathExtension]];
#endif

	}
	
#if QSAlchemy
	NSEnumerator *e = [[QSReg loadedInstancesForPointID:@"QSMusicArtworkSources"] objectEnumerator];
#else
		
		NSEnumerator *e = [[QSReg instancesForTable:@"QSMusicArtworkSources"] objectEnumerator];
#endif
	
	
	id source;

	while (!icon && (source = [e nextObject]) ) {
		icon = [source imageForTrackInfo:trackDict];
	}
	
	if (useNet && !icon) {
		e = [[QSReg instancesForTable:@"QSOnlineMusicArtworkSources"] objectEnumerator];
		while (!icon && (source = [e nextObject]) ) {
			NSString *imageURL = [source imageURLForTrackInfo:trackDict];
			NSData *data = nil;
			if (imageURL) {
				data = [NSData dataWithData:[[NSURL URLWithString:imageURL] resourceDataUsingCache:YES]];
				icon = [[[NSImage alloc] initWithData:data] autorelease];
				
			} else {
#ifdef DEBUG
				if (VERBOSE) NSLog(@"No Artwork Found");
#endif
			}
			
			if (data) {
				//NSLog(@"Loaded online artwork");
				
				[(id)[QSReg getClassInstance:@"QSCommonMusicArtworkPlugIn"] storeImageData:data ofType:[imageURL pathExtension] forTrackInfo:trackDict];
				
			}
		}
	}
	
	if (icon) {
		[icon createIconRepresentations];
		[icon createRepresentationOfSize:NSMakeSize(128, 128)];
	}
	return icon;
}




- (NSImage *)imageForTrackInfo:(NSDictionary *)info {
  NSString *trackid = [info objectForKey:@"Persistent ID"];
  NSString *libraryid = [library libraryID];
  
  int length = [trackid length];
  
 int dir1 = [[trackid substringWithRange:NSMakeRange(length-1, 1)] hexIntValue];
 int dir2 = [[trackid substringWithRange:NSMakeRange(length-2, 1)] hexIntValue];
 int dir3 = [[trackid substringWithRange:NSMakeRange(length-3, 1)] hexIntValue];
  
  NSString *path = [NSString stringWithFormat:@"%@/Album Artwork/Download/%@/%@/%@/%@-%@.itc",  
    [[library libraryLocation] stringByDeletingLastPathComponent] ,
    [NSString stringWithFormat:@"%02d", dir1] ,
        [NSString stringWithFormat:@"%02d", dir2] ,
        [NSString stringWithFormat:@"%02d", dir3] ,
    libraryid,
    trackid,
    nil];
   
  NSLog(@"path %@ %@", path, info);



  
//	NSString *album = [info objectForKey:@"Album"];
//	NSString *artist = [info objectForKey:@"Artist"];
//	BOOL compilation = [[info objectForKey:@"Compilation"] boolValue];
//	//if (!album) album = @"Unknown album";
//	NSString *fileName = @"*";
//	if (compilation) artist = @"Compilation";
//	
//	NSString *sofaFile = [NSString stringWithFormat:@"%@/%@/%@", artist, album, fileName];
//	sofaFile = [sofaFile stringByReplacing:@"." with:@""];
//	sofaFile = [sofaFile stringByReplacing:@":" with:@" "];
//	sofaFile = [sofaFile stringByReplacing:@"\"" with:@""];
//	//	sofaFile = [sofaFile stringByReplacing:@"  " with:@" "];
//	
//	NSString *sofaPath = [[SOFPATH stringByAppendingPathComponent:sofaFile] stringByResolvingWildcardsInPath]; 			
//	//	 	NSLog(@"Sofa art: %@", sofaPath);
//	if ([[NSFileManager defaultManager] fileExistsAtPath:sofaPath]) {
//		return [[[NSImage alloc] initWithContentsOfFile:sofaPath] autorelease];
//	}
	return nil;
}

- (BOOL)objectHasChildren:(id <QSObject>)object {
	if ([[object primaryType] isEqualToString:QSiTunesPlaylistIDPboardType]) {
		NSDictionary *playlistDict = [library playlistInfoForID:[object objectForType:QSiTunesPlaylistIDPboardType]];
		return [(NSArray *)[playlistDict objectForKey:@"Playlist Items"] count];
	} else if ([[object primaryType] isEqualToString:QSiTunesBrowserPboardType]) {
		return YES;  
	} else if ([[object primaryType] isEqualToString:QSiTunesTrackIDPboardType]) {
		return NO;
	}
	return NO;
}
- (BOOL)objectHasValidChildren:(id <QSObject>)object {
	return YES;
}

- (NSString *)detailsOfObject:(QSObject *)object {
	NSString *details;
	if ([[object primaryType] isEqualToString:QSiTunesPlaylistIDPboardType]) {
		// Playlist details
		NSDictionary *info = [library playlistInfoForID:[object objectForType:QSiTunesPlaylistIDPboardType]];
		int count = [(NSArray *)[info objectForKey:@"Playlist Items"] count];
		if (count) {
			details = [NSString stringWithFormat:@"%d track%@", count, ESS(count)];
			return details;
		}
	} else if ([[object primaryType] isEqualToString:QSiTunesBrowserPboardType]) {
		// Browser item details
		details = [[[[object objectForType:QSiTunesBrowserPboardType] objectForKey:@"Criteria"] allValues] componentsJoinedByString:[NSString stringWithFormat:@" %C ", 0x25B8]];
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
	}
	return nil;  
}

- (QSObject *)trackObjectForInfo:(NSDictionary *)trackInfo inPlaylist:(NSString *)playlist {
	if (!trackInfo) return nil;
	QSObject *newObject = [QSObject makeObjectWithIdentifier:[trackInfo objectForKey:@"Persistent ID"]];
	[newObject setName:[trackInfo objectForKey:@"Name"]];
	[newObject setObject:trackInfo forType:QSiTunesTrackIDPboardType];
	if (playlist) [newObject setObject:playlist forMeta:@"QSiTunesSourcePlaylist"];
	
	NSString *path = [trackInfo objectForKey:@"Location"];
	if (path) path = [[NSURL URLWithString:path] path];
	if (path) [newObject setObject:[NSArray arrayWithObject:path] forType:NSFilenamesPboardType];
	[newObject setPrimaryType:QSiTunesTrackIDPboardType];
	return newObject;
}

- (NSArray *)iTunesGetChildren
{
	// this repeats the list created in objectsForEntry, but executes MUCH faster than re-running that method
	NSMutableArray *children = [NSMutableArray arrayWithCapacity:1];
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesMonitorTracks"]) {
		[children addObject:[QSObject objectWithIdentifier:QSiTunesRecentTracksBrowser]];
	}
	QSAction *showCurrentTrack = [QSAction objectWithIdentifier:@"QSiTunesShowCurrentTrack"];
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
	if ([[object primaryType] isEqualToString:NSFilenamesPboardType]) {
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
	NSString *value;
	if ([rootType isEqualToString:@"Artist"] && [trackDict objectForKey:@"Album Artist"]) {
		value = [trackDict objectForKey:@"Album Artist"];
	} else {
		value = [trackDict objectForKey:rootType];
	}
	if (!value) return nil;
	id newObject;
	NSMutableDictionary *childCriteria = [NSMutableDictionary dictionaryWithObjectsAndKeys:
		[NSDictionary dictionaryWithObject:value forKey:rootType] , @"Criteria",
		[library nextSortForCriteria:rootType] , @"Result", rootType, @"Type", nil];
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
		QSObject *newObject;
		int count = [sortTags count];
		int i;
		for (i = 0; i < count; i++) {
			NSString *rootType = [sortTags objectAtIndex:i];
			id newObject = [self browserObjectForTrack:trackDict andCriteria:rootType];
			if (newObject)
				[objects addObject:newObject];
		}
		return objects;
		
		
	} else  if ([[object primaryType] isEqualToString:QSiTunesPlaylistIDPboardType]) {
		NSDictionary *playlistDict = [library playlistInfoForID:[object objectForType:QSiTunesPlaylistIDPboardType]];
		NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
		
		NSArray *trackIDs = [[playlistDict objectForKey:@"Playlist Items"] valueForKeyPath:@"Track ID.stringValue"];
		
		NSArray *tracks = [library trackInfoForIDs:trackIDs];
		for (NSDictionary *currentTrack in tracks) {
			id object = [self trackObjectForInfo:currentTrack inPlaylist:[playlistDict objectForKey:@"Name"]];
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
	int i;
	
	NSArray *trackArray = [library tracksMatchingCriteria:criteriaDict];
	
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
				[descriptors addObject:[[[NSSortDescriptor alloc] initWithKey:@"Album" ascending:YES] autorelease]];
				[descriptors addObject:[[[NSSortDescriptor alloc] initWithKey:@"Disc Number" ascending:YES] autorelease]];
				[descriptors addObject:[[[NSSortDescriptor alloc] initWithKey:@"Track Number" ascending:YES] autorelease]];
			} else {
				[descriptors addObject:[[[NSSortDescriptor alloc] initWithKey:@"Name" ascending:YES] autorelease]];
			}
			NSArray *sortedTracks = [trackArray sortedArrayUsingDescriptors:descriptors];
			for (NSDictionary *trackInfo in sortedTracks) {
				[objects addObject:[self trackObjectForInfo:trackInfo inPlaylist:nil]];
			}
			
			return objects;
			
		} else {
			NSArray *subsets = [library objectsForKey:displayType inArray:trackArray];
			
			NSMutableArray *objects = [NSMutableArray arrayWithCapacity:1];
			NSMutableArray *usedKeys = [NSMutableArray arrayWithCapacity:1];
			
			QSObject *newObject;
			NSString *childSort = [nextSort objectForKey:displayType];
			if (!childSort)
				childSort = @"Track";
			
			NSMutableDictionary *childBrowseDict;
			 {
				childBrowseDict = [[browseDict mutableCopy] autorelease];
				if (!childBrowseDict) childBrowseDict = [NSMutableDictionary dictionaryWithCapacity:1];
				[childBrowseDict setObject:childSort forKey:@"Result"];
				[childBrowseDict setObject:displayType forKey:@"Type"];
				
				NSString *details = [[[childBrowseDict objectForKey:@"Criteria"] allValues] componentsJoinedByString:[NSString stringWithFormat:@" %C ", 0x25B8]];
				NSString *name = [NSString stringWithFormat:@"All %@s", displayType];
				
				if (details) name = [name stringByAppendingFormat:@" (%@) ", details];
				newObject = [QSObject objectWithName:name];
				[newObject setObject:childBrowseDict forType:QSiTunesBrowserPboardType];
				[newObject setPrimaryType:QSiTunesBrowserPboardType];
				[objects addObject:newObject];  
			}
			
			
			//  NSImage *icon = [NSImage imageNamed:[NSString stringWithFormat:@"iTunes%@BrowserIcon", displayType]];
			
			for (i = 0; i < [subsets count]; i++) {
				NSString *thisItem = [subsets objectAtIndex:i];
				if ([usedKeys containsObject:[thisItem lowercaseString]]) continue;
				childBrowseDict = [NSMutableDictionary dictionaryWithCapacity:1];
				
				NSMutableDictionary *childCriteriaDict = [[criteriaDict mutableCopy] autorelease];
				if (!childCriteriaDict) childCriteriaDict = [NSMutableDictionary dictionaryWithCapacity:1];
				[childCriteriaDict setObject:[subsets objectAtIndex:i] forKey:displayType];
				
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
	QSObject *newObject;
	int count = [sortTags count];
	int i;
	for (i = 0; i < count; i++) {
		NSString *rootType = [sortTags objectAtIndex:i];
		NSMutableDictionary *childCriteria = [NSMutableDictionary dictionaryWithObjectsAndKeys:rootType, @"Result", rootType, @"Type", nil];
		newObject = [QSObject objectWithName:[NSString stringWithFormat:@"Browse %@s", rootType]];
		[newObject setObject:childCriteria forType:QSiTunesBrowserPboardType];  
		[newObject setPrimaryType:QSiTunesBrowserPboardType];
		[objects addObject:newObject];
	}
	return objects;
}

- (BOOL)showArtwork {
	return showArtwork;
}

- (void)setShowArtwork:(BOOL)shouldShow {
	//	NSLog(@"%d", shouldShow);
	showArtwork = shouldShow;
}
- (NSAppleScript *)iTunesScript {
	return QSiTunesScript();
}
@end
