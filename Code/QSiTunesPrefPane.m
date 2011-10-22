

#import "QSiTunesPrefPane.h"



@implementation QSiTunesPrefPane
- (id)init {
    self = [super initWithBundle:[NSBundle bundleForClass:[QSiTunesPrefPane class]]];
    if (self) {
    }
    return self;
}

- (NSImage *) icon{
	return [QSResourceManager imageNamed:@"com.apple.itunes"];
}

- (NSString *) mainNibName{
return @"QSiTunesPrefPane";
}

@end
