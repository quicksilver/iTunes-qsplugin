//
//  QSiTunesDatabase.h
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

@interface QSiTunesDatabase : NSObject {
	NSDictionary *iTunesMusicLibrary;
    NSString *libraryLocation;
    NSDictionary *tagDictionaries;
	NSMutableDictionary *extraTracks;

}

+ (id)sharedInstance;
-(NSDictionary *)trackInfoForID:(NSString *)theID;
-(NSArray *)trackInfoForIDs:(NSArray *)theIDs;
- (NSDictionary *)iTunesMusicLibrary;
- (void)setITunesMusicLibrary:(NSDictionary *)newITunesMusicLibrary;

- (NSArray *)tracksMatchingCriteria:(NSDictionary *)criteria;
- (NSDictionary *)tagDictionaries;

- (void)setTagDictionaries:(NSDictionary *)newTagDictionaries;
- (NSString *)libraryLocation;
- (NSString *)libraryID;
- (BOOL)loadMusicLibrary;
- (NSArray *)playlists;
-(NSDictionary *)playlistInfoForID:(NSNumber *)theID;
-(NSDictionary *)playlistInfoForName:(NSString *)theName;
- (void)registerAdditionalTrack:(NSDictionary *)info forID:(id)num;
- (NSArray *)objectsForKey:(NSString *)key inArray:(NSArray *)array;


-(BOOL)isLoaded;
- (NSString *)nextSortForCriteria:(NSString *)sortTag;
@end
