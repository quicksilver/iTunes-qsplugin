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

- (id)init {
	if (self = [super init]) {
		//[[QSVoyeur sharedInstance] addPathToQueue:[self libraryLocation]];
		extraTracks = [[NSMutableDictionary alloc] init];
		iTunesMusicLibrary = nil;
		tagDictionaries = nil;
		// find the Library location
		NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
		NSDictionary *userPref = [userDefaults persistentDomainForName:@"com.apple.iApps"];
		NSArray *recentDatabases = [userPref objectForKey:@"iTunesRecentDatabases"];
		libraryLocation = [[NSURL URLWithString:[recentDatabases objectAtIndex:0]] path];
		if (!libraryLocation) {
			libraryLocation = ITUNESLIBRARY;
		}
		libraryLocation = [[[NSFileManager defaultManager] fullyResolvedPathForPath:libraryLocation] retain];
	}
	return self;
}

- (void)dealloc {
    [extraTracks release];
	[iTunesMusicLibrary release];
	[tagDictionaries release];
	[libraryLocation release];
    [super dealloc];
}

- (NSString *)libraryID {
  	NSString *ident = [[self iTunesMusicLibrary] objectForKey:@"Library Persistent ID"];
  return ident;
}

- (void)registerAdditionalTrack:(NSDictionary *)info forID:(id)num {
	[extraTracks setObject:info forKey:num];
}
- (NSDictionary *)trackInfoForID:(NSString *)theID {
	NSDictionary *tracks = [[self iTunesMusicLibrary] objectForKey:@"Tracks"];
	NSDictionary *theTrack = [tracks objectForKey:theID];
	if (!theTrack) {
		//NSLog(@"can't find track %@ in %@", theID, [tracks allKeys]);
		theTrack = [extraTracks objectForKey:theID];
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
	NSMutableSet *thisSet;
	//NSLog(@"criteria %@", criteria);
	for (NSString *key in criteria) {
		NSDictionary *typeDicts;
		NSArray *valueArray;
		// if Artist is a criteria, check (Artist == X OR Album Artist == X)
		if ([key isEqualToString:@"Artist"]) {
			typeDicts = [[self tagDictionaries] objectForKey:key];
			NSMutableArray *artistArray = [typeDicts objectForKey:[[criteria objectForKey:key] lowercaseString]];
			typeDicts = [[self tagDictionaries] objectForKey:@"Album Artist"];
			[artistArray addObjectsFromArray:[typeDicts objectForKey:[[criteria objectForKey:key] lowercaseString]]];
			valueArray = artistArray;
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
	NSDictionary *trackInfo;
	
	NSMutableDictionary *tagDict;
	NSMutableArray *valueArray;
	NSString *thisValue;
	BOOL groupCompilations = [[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesGroupCompilations"];
	BOOL showPodcasts = NO; //[[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesShowPodcasts"];
  
	for (NSString *key in tracks) {
		trackInfo = [tracks objectForKey:key];
		
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

- (NSString *)libraryLocation {
	return libraryLocation;
}

- (BOOL)loadMusicLibrary {
	//if (VERBOSE) NSLog(@"Loading Music Library from: %@", [self libraryLocation]);
	
	if (![self libraryLocation]) return NO;
	[[QSTaskController sharedInstance] updateTask:@"Load iTunes Library" status:@"Loading Music Library" progress:-1];
	
	NSDictionary *loadedLibrary = [NSDictionary dictionaryWithContentsOfFile:[self libraryLocation]];
	if (!loadedLibrary) {
		loadedLibrary = [NSDictionary dictionary];
		//if (VERBOSE) NSLog(@"Music Library not Found");
		
		[[QSTaskController sharedInstance] removeTask:@"Load iTunes Library"];
		return NO;
	} 
	[self setITunesMusicLibrary:loadedLibrary];  
	[self createTagArrays];
	[[QSTaskController sharedInstance] removeTask:@"Load iTunes Library"];
	return YES;
}

- (NSDictionary *)iTunesMusicLibrary {
	if (!iTunesMusicLibrary) {
		[self loadMusicLibrary];
	}
	return iTunesMusicLibrary;  
}

- (void)setITunesMusicLibrary:(NSDictionary *)newITunesMusicLibrary {
	[iTunesMusicLibrary release];
	iTunesMusicLibrary = [newITunesMusicLibrary retain];
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

- (NSDictionary *)tagDictionaries { 
	if (!tagDictionaries) {
		[self createTagArrays];
	}
	return tagDictionaries;
}

- (void)setTagDictionaries:(NSDictionary *)newTagDictionaries {
	[tagDictionaries release];
	tagDictionaries = [newTagDictionaries retain];
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
	return [iTunesMusicLibrary count];
}

- (NSDictionary *)playlistInfoForID:(NSNumber *)theID {
	if (![theID isKindOfClass:[NSNumber class]]) {
		[NSApp activateIgnoringOtherApps:YES];
		//		NSRunCriticalAlertPanel(@"iTunes Change", @"the iTunes PlugIn format has changed. please remake old playlist triggers.", @"OK", nil, nil);
		return nil;
	}
	NSArray *playlists = [[self iTunesMusicLibrary] objectForKey:@"Playlists"];
	int i = [[playlists valueForKey:@"Playlist ID"] indexOfObject:theID];
	
	if (i == NSNotFound) return nil;
	return [playlists objectAtIndex:i];
}

- (NSDictionary *)playlistInfoForName:(NSString *)theName {
	NSArray *playlists = [[self iTunesMusicLibrary] objectForKey:@"Playlists"];
	int i = [[playlists valueForKey:@"Name"] indexOfObject:theName];
	
	if (i == NSNotFound) return nil;
	return [playlists objectAtIndex:i];
}

@end
