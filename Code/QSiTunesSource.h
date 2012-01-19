#import <Cocoa/Cocoa.h>

//#import <QSCore/QSCore.h>

#import "QSiTunesDefines.h"
#import "QSiTunesDatabase.h"

@interface QSiTunesObjectSource : QSObjectSource {
     NSMutableArray *recentTracks;
	BOOL showArtwork;
	QSiTunesDatabase *library;
	iTunesApplication *iTunes;
}
- (NSAttributedString *)starsForRating:(int)rating;
- (QSObject *)trackObjectForInfo:(NSDictionary *)trackInfo inPlaylist:(NSString *)playlist;
- (NSImage *)imageForTrack:(NSDictionary *)trackDict;
- (NSArray *)childrenForObject:(QSObject *)object;
- (NSArray *)childrenForBrowseCriteria:(NSDictionary *)browseDict;

- (NSString *)currentTrackID;
- (id)currentTrackInfo;
- (NSDictionary *)trackInfoForID:(id)trackID;
- (void)showNotificationForTrack:(id)trackID info:(NSDictionary *)trackInfo;
- (QSObject *)browserObjectForTrack:(NSDictionary *)trackDict andCriteria:(NSString *)rootType;
- (NSArray *)browseMasters;

- (NSImage *)imageForTrack:(NSDictionary *)trackDict;
@end

@interface QSiTunesControlSource : QSObjectSource {

}
@end
