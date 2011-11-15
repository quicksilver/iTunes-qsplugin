#import <Cocoa/Cocoa.h>

//#import <QSCore/QSCore.h>


#import "QSiTunesDatabase.h"

@interface QSiTunesObjectSource : QSObjectSource{
     NSMutableArray *recentTracks;
	BOOL showArtwork;
	QSiTunesDatabase *library;
}
- (NSAttributedString *)starsForRating:(int)rating;
- (QSObject *)trackObjectForInfo:(NSDictionary *)trackInfo inPlaylist:(NSString *)playlist;
- (NSImage *)imageForTrack:(NSDictionary *)trackDict;
- (NSArray *)childrenForObject:(QSObject *)object;
- (NSArray *)childrenForBrowseCriteria:(NSDictionary *)browseDict;
- (NSAppleScript *)iTunesScript;

- (id)currentTrackID;
- (void)showNotificationForTrack:(id)trackID info:(NSDictionary *)trackInfo;
- (QSObject *)browserObjectForTrack:(NSDictionary *)trackDict andCriteria:(NSString *)rootType;
- (NSArray *)browseMasters;

- (NSImage *)imageForTrack:(NSDictionary *)trackDict;
- (NSImage *)imageForTrack:(NSDictionary *)trackDict useNet:(BOOL)useNet;	
@end
