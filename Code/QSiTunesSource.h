#import "QSiTunesDefines.h"

@interface QSiTunesObjectSource : QSObjectSource <SBApplicationDelegate>

@property NSMutableArray *recentTracks;
@property iTunesApplication *iTunes;

- (NSAttributedString *)starsForRating:(NSUInteger)rating;
- (NSImage *)imageForTrack:(NSDictionary *)trackDict;
- (NSArray *)childrenForObject:(QSObject *)object;
- (NSArray *)childrenForBrowseCriteria:(NSDictionary *)browseDict;

- (NSString *)currentTrackID;
- (id)currentTrackInfo;
- (NSDictionary *)trackInfoForID:(id)trackID;
- (void)showNotificationForTrack:(id)trackID info:(NSDictionary *)trackInfo;
- (QSObject *)browserObjectForTrack:(NSDictionary *)trackDict andCriteria:(NSString *)rootType;
- (NSArray *)browseMasters;
@end

@interface QSiTunesControlSource : QSObjectSource
- (void)checkForRelaunch:(NSNotification *)note;
@end

@interface QSiTunesEQPresets : QSObjectSource

@end

@interface QSiTunesAirPlayDevices : QSObjectSource

@end

@interface QSCommand (iTunes)
- (NSDictionary *)commandDict;
@end
