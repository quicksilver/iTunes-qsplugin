#import <Foundation/Foundation.h>
#import "iPodDB.h"
int dumpiTunesDBmem( char *filename );

int main (int argc, const char * argv[]) {
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

	//dumpiTunesDBmem(argv[1]);
	NSData *data=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];
		iPod_class.parse([data bytes]);
    [pool release];
    return 0;
}
