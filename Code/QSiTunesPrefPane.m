#import "QSiTunesPrefPane.h"

@implementation QSiTunesPrefPane
- (id)init
{
	self = [super initWithBundle:[NSBundle bundleForClass:[QSiTunesPrefPane class]]];
	return self;
}

- (NSImage *) icon
{
	return [QSResourceManager imageNamed:@"com.apple.itunes"];
}

- (NSString *) mainNibName
{
	return @"QSiTunesPrefPane";
}

- (BOOL)groupCompilations
{
	return [[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesGroupCompilations"];
}

- (void)setGroupCompilations:(BOOL)flag
{
	[[NSUserDefaults standardUserDefaults] setBool:flag forKey:@"QSiTunesGroupCompilations"];
	[NSApp requestRelaunch:nil];
}

- (BOOL)monitorTracks
{
	return [[NSUserDefaults standardUserDefaults] boolForKey:@"QSiTunesMonitorTracks"];
}

- (void)setMonitorTracks:(BOOL)flag
{
	[[NSUserDefaults standardUserDefaults] setBool:flag forKey:@"QSiTunesMonitorTracks"];
	[NSApp requestRelaunch:nil];	
}

@end
