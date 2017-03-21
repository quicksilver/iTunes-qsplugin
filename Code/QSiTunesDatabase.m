//
//  QSiTunesDatabase.m
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "QSiTunesDatabase.h"
//#import <QSCore/QSMacros.h>

#import "QSiTunesDefines.h"

@implementation QSiTunesDatabase

+ (instancetype)sharedInstance
{
    static id _sharedInstance;
    if (!_sharedInstance) _sharedInstance = [[self alloc] init];
    return _sharedInstance;
}

- (id)init {
	if (self = [super init]) {
		//[[QSVoyeur sharedInstance] addPathToQueue:[self libraryLocation]];
		_extraTracks = [[NSMutableDictionary alloc] init];
		// find the Library location
		NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
		NSDictionary *userPref = [userDefaults persistentDomainForName:@"com.apple.iApps"];
		NSArray *recentDatabases = [userPref objectForKey:@"iTunesRecentDatabases"];
		_libraryLocation = [[NSURL URLWithString:[recentDatabases objectAtIndex:0]] path];
		if (!_libraryLocation) {
			_libraryLocation = ITUNESLIBRARY;
		}
		_libraryLocation = [[NSFileManager defaultManager] fullyResolvedPathForPath:_libraryLocation];
		[self loadMusicLibrary];
        [self createTagArrays];
	}
	return self;
}

- (NSString *)libraryID {
  	NSString *ident = [[self iTunesMusicLibrary] objectForKey:@"Library Persistent ID"];
  return ident;
}

- (void)registerAdditionalTrack:(NSDictionary *)info forID:(id)num {
	[[self extraTracks] setObject:info forKey:num];
}
- (NSDictionary *)trackInfoForID:(NSString *)theID {
	NSDictionary *tracks = [[self iTunesMusicLibrary] objectForKey:@"Tracks"];
	NSDictionary *theTrack = [tracks objectForKey:theID];
	if (!theTrack) {
		//NSLog(@"can't find track %@ in %@", theID, [tracks allKeys]);
		theTrack = [[self extraTracks] objectForKey:theID];
	}
	return theTrack;
}

- (NSArray *)trackInfoForIDs:(NSArray *)theIDs {
	NSMutableArray *tracks = [NSMutableArray array];
	for (id theID in theIDs) {
		id track = [self trackInfoForID:theID];
		if (track) [tracks addObject:track];
	}
	return tracks;
}

- (NSArray *)tracksMatchingCriteria:(NSDictionary *)criteria {
	if (![criteria count]) return [[[self iTunesMusicLibrary] objectForKey:@"Tracks"] allValues];
	NSMutableSet *items = nil;
	NSMutableSet *thisSet = nil;
	//NSLog(@"criteria %@", criteria);
	for (NSString *key in criteria) {
		NSDictionary *typeDicts = nil;
		NSArray *valueArray = nil;
		// if Artist is a criteria, check (Artist == X OR Album Artist == X)
		if ([key isEqualToString:@"Artist"]) {
			typeDicts = [[self tagDictionaries] objectForKey:key];
            NSArray *artists = [typeDicts objectForKey:[[criteria objectForKey:key] lowercaseString]];
            typeDicts = [[self tagDictionaries] objectForKey:@"Album Artist"];
            NSArray *albumArtists = [typeDicts objectForKey:[[criteria objectForKey:key] lowercaseString]];
            valueArray = [artists arrayByAddingObjectsFromArray:albumArtists];
		} else {
			typeDicts = [[self tagDictionaries] objectForKey:key];
			valueArray = [typeDicts objectForKey:[[criteria objectForKey:key] lowercaseString]];
		}
		thisSet = [NSMutableSet setWithArray:valueArray];
		if (!items) {
			items = thisSet;
		} else {
			[items intersectSet:thisSet];
		}
		//NSLog(@"match %@ = %@ %lu", key, [criteria objectForKey:key] , [items count]);
	}
	return [items allObjects];
} 

- (void)createTagArrays {
	NSArray *sortTags = [NSArray arrayWithObjects:@"Genre", @"Artist", @"Album Artist", @"Composer", @"Album", nil];
	
	NSMutableDictionary *newTagDictionaries = [NSMutableDictionary dictionaryWithCapacity:[sortTags count]];
	
	for (NSString *thisTag in sortTags) {
		[newTagDictionaries setObject:[NSMutableDictionary dictionaryWithCapacity:1] forKey:thisTag];
	}
	
	NSDictionary *tracks = [[self iTunesMusicLibrary] objectForKey:@"Tracks"];
	NSDictionary *trackInfo = nil;
	
	NSMutableDictionary *tagDict = [NSMutableDictionary dictionary];
	NSMutableArray *valueArray = [NSMutableArray array];
	NSString *thisValue = nil;
	BOOL groupCompilations = [[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesGroupCompilations"];
	BOOL showPodcasts = NO; //[[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesShowPodcasts"];
  
	for (NSString *key in tracks) {
		trackInfo = [tracks objectForKey:key];
        
        /* skip tracks that have no URL - we can't do anything with them
           These could be purchased tracks that aren't downloaded. If Apple ever
           provides a way to download them, we should include them (and create a
           "download" action). */
        if (![trackInfo objectForKey:@"Location"]) {
            continue;
        }
		
		if (!showPodcasts && [[trackInfo objectForKey:@"Podcast"] boolValue]) {
			continue;
		}
    
		for (NSString *thisTag in sortTags) {
			if (groupCompilations && [thisTag isEqualToString:@"Artist"] && [[trackInfo objectForKey:@"Compilation"] boolValue]) {
				thisValue = COMPILATION_STRING;
			} else {
				thisValue = [trackInfo objectForKey:thisTag];
			}
			
			tagDict = [newTagDictionaries objectForKey:thisTag];
			NSString *lowercaseKey = [thisValue lowercaseString];
			//NSLog(thisValue);
			if (lowercaseKey) {
				if (!(valueArray = [tagDict objectForKey:lowercaseKey]) )
					[tagDict setObject:(valueArray = [NSMutableArray arrayWithCapacity:1]) forKey:lowercaseKey];
				[valueArray addObject:trackInfo];
			}
		}
	}
	[self setTagDictionaries:newTagDictionaries];
} 

- (BOOL)loadMusicLibrary {
	//if (VERBOSE) NSLog(@"Loading Music Library from: %@", [self libraryLocation]);
	
	if (![self libraryLocation]) return NO;
	QSTask *loadTask = [QSTask taskWithIdentifier:@"Load iTunes Library"];
	[loadTask setName:@"Loading Music Library"];
	[loadTask setIcon:[QSResourceManager imageNamed:@"iTunesIcon"]];
	[loadTask setShowProgress:NO];
	
	NSDictionary *loadedLibrary = [NSDictionary dictionaryWithContentsOfFile:[self libraryLocation]];
	if (!loadedLibrary) {
		loadedLibrary = [NSDictionary dictionary];
		//if (VERBOSE) NSLog(@"Music Library not Found");
		
		[loadTask stop];
		return NO;
	} 
	[self setITunesMusicLibrary:loadedLibrary];  
	[self createTagArrays];
	[loadTask stop];
	return YES;
}

- (NSString *)nextSortForCriteria:(NSString *)sortTag {
	NSDictionary *nextSort = [NSDictionary dictionaryWithObjectsAndKeys:
		@"Artist", @"Genre",
		@"Album", @"Artist",
		@"Album", @"Composer",
		@"Track", @"Album",
		@"Year", @"Album",
		nil];
	
	return [nextSort objectForKey:sortTag];
}

- (NSArray *)objectsForKey:(NSString *)key inArray:(NSArray *)array {
	NSMutableSet *values = [NSMutableSet setWithCapacity:1];
	BOOL groupCompilations = [key isEqualToString:@"Artist"] && [[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesGroupCompilations"];
	id value;
	
	for (id thisObject in array) {
		if (groupCompilations && [[thisObject objectForKey:@"Compilation"] boolValue])
			value = COMPILATION_STRING;
		else
			value = [thisObject objectForKey:key];
		if (value) [values addObject:value];
	}
	return [[values allObjects] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
}

/*
 -(void)getMusicPreferences {
	 NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	 [defaults addSuiteNamed:@"com.apple.iTunes"]; //Add Apple's iTunes item
	 NSData *musicFolderAliasData = [defaults dataForKey:@"alis:11345:Music Folder Location"];
	 NSData *preferencesData = [defaults dataForKey:@"pref:129:Preferences"];
	 
	 short location;
	 [preferencesData getBytes:&location range:NSMakeRange(14, 2)];
	 
	 bool customFolder = (location == 11345); //    [preferencesData isEqualTaData:];
	 
	 NSString *musicPath = nil;
	 
	 if (musicFolderAliasData && customFolder) {
		 musicPath = [[BDAlias aliasWithData:musicFolderAliasData] fullPath];
		 if (!musicPath) NSLog(@"Unable to find iTunes music library location");
		 else
			 NSLog(@"Using iTunes music library location: %@", [musicPath stringByAbbreviatingWithTildeInPath]);
	 }
	 if (!musicPath) {
		 NSLog(@"Using iTunes default music library location");
		 musicPath = [@"~/Music/iTunes/iTunes Music/" stringByExpandingTildeInPath];
	 }
	 
	 [self setMusicDirectory:musicPath];
 }
 */

- (NSArray *)playlists {
	return [[self iTunesMusicLibrary] objectForKey:@"Playlists"];
}

- (BOOL)isLoaded {
	return [[self iTunesMusicLibrary] count];
}

- (NSDictionary *)playlistInfoForID:(NSNumber *)theID {
	if (![theID isKindOfClass:[NSNumber class]]) {
		[NSApp activateIgnoringOtherApps:YES];
		//		NSRunCriticalAlertPanel(@"iTunes Change", @"the iTunes PlugIn format has changed. please remake old playlist triggers.", @"OK", nil, nil);
		return nil;
	}
	NSArray *playlists = [[self iTunesMusicLibrary] objectForKey:@"Playlists"];
	NSUInteger i = [[playlists valueForKey:@"Playlist ID"] indexOfObject:theID];
	
	if (i == NSNotFound) return nil;
	return [playlists objectAtIndex:i];
}

- (NSDictionary *)playlistInfoForName:(NSString *)theName {
	NSArray *playlists = [[self iTunesMusicLibrary] objectForKey:@"Playlists"];
	NSUInteger i = [[playlists valueForKey:@"Name"] indexOfObject:theName];
	
	if (i == NSNotFound) return nil;
	return [playlists objectAtIndex:i];
}

- (QSObject *)trackObjectForInfo:(NSDictionary *)trackInfo inPlaylist:(NSString *)playlist {
	if (!trackInfo) return nil;
	QSObject *newObject = [QSObject makeObjectWithIdentifier:[trackInfo objectForKey:@"Persistent ID"]];
	[newObject setName:[trackInfo objectForKey:@"Name"]];
	if ([trackInfo valueForKey:@"Has Video"]) {
		// set a default label
		[newObject setLabel:[NSString stringWithFormat:@"%@ (Video)", [newObject name]]];
		// override with more specific info (if found)
		NSArray *videoKinds = [NSArray arrayWithObjects:@"Music Video", @"Movie", @"TV Show", nil];
		for (NSString *vkind in videoKinds) {
			if ([trackInfo valueForKey:vkind]) {
				[newObject setLabel:[NSString stringWithFormat:@"%@ (%@)", [newObject name], vkind]];
			}
		}
	}
	[newObject setObject:trackInfo forType:QSiTunesTrackIDPboardType];
	if (playlist) [newObject setObject:playlist forMeta:@"QSiTunesSourcePlaylist"];
	
	NSString *path = [trackInfo objectForKey:@"Location"];
	if (path) path = [[NSURL URLWithString:path] path];
	if (path) [newObject setObject:[NSArray arrayWithObject:path] forType:QSFilePathType];
	[newObject setPrimaryType:QSiTunesTrackIDPboardType];
	return newObject;
}

@end
