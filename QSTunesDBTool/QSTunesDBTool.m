#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
@implementation NSString (LowerKeyConversion)
- (NSString *)lowerUnderscoredString{
	NSMutableString *string=[[self mutableCopy]autorelease];
	[string replaceOccurrencesOfString:@" " withString:@"_" options:nil range:NSMakeRange(0,[self length])];
	return [string lowercaseString];
}
@end
int main (int argc, const char * argv[]) {
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

	NSString *modelPath=@"/Volumes/Lore/Forge/Build/QSiTunesDatabase.app/Contents/Resources/QSiTunesDatabase_DataModel.mom";
    NSManagedObjectModel *managedObjectModel = [[[NSManagedObjectModel alloc]initWithContentsOfURL:[NSURL fileURLWithPath:modelPath]]autorelease];
    NSManagedObjectContext *managedObjectContext;
	
	
	
	NSError *error;
    NSString *applicationSupportFolder = nil;
    NSURL *url;
    NSFileManager *fileManager;
    NSPersistentStoreCoordinator *coordinator;
    
    fileManager = [NSFileManager defaultManager];
    applicationSupportFolder = @"/Volumes/Lore/Desktop/";
    if ( ![fileManager fileExistsAtPath:applicationSupportFolder isDirectory:NULL] ) {
        [fileManager createDirectoryAtPath:applicationSupportFolder attributes:nil];
    }
    NSString *storePath=[applicationSupportFolder stringByAppendingPathComponent: @"QSiTunesDatabase.sql"];
	[fileManager removeFileAtPath:storePath handler:nil];
    url = [NSURL fileURLWithPath: storePath];
    coordinator = [[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel:  managedObjectModel];
    if ([coordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:nil URL:url options:nil error:&error]){
        managedObjectContext = [[NSManagedObjectContext alloc] init];
        [managedObjectContext setPersistentStoreCoordinator: coordinator];
    } else {
		NSLog(@"%@",error);
    }    
    [coordinator release];
    
	NSManagedObject *newTrack = nil;
	
	NSLog(@"Loading Music");
	NSDictionary *music=[NSDictionary dictionaryWithContentsOfFile:@"/Volumes/Lore/Forge/Development/Marmalade3/~Resources/Cube iTunes Music Library.xml"];
	
	NSDictionary *tracks=[music objectForKey:@"Tracks"];
	
	NSArray *trackDicts=[tracks allValues];
	NSEnumerator *e=[tracks objectEnumerator];
	NSDictionary *trackDict=nil;
	
	
	//NSArray *properties=[NSEntityDescription entityForName:@"Track" inManagedObjectContext:[self managedObjectContext]];
	NSArray *properties=[@"Location,Name,Artist,Album,Composer,Genre,Track ID" componentsSeparatedByString:@","];
	NSDictionary *translation=[NSDictionary dictionaryWithObjects:[properties valueForKey:@"lowerUnderscoredString"] forKeys:properties];
	NSEnumerator *ke;
	NSString *key;
	id value;		
	NSLog(@"processing tracks");
	
	while(trackDict=[e nextObject]){
		newTrack=[NSEntityDescription insertNewObjectForEntityForName:@"Track"
											   inManagedObjectContext: managedObjectContext];
		//NSLog(@"track %@",[trackDict valueForKey:@"Location"]);
		ke=[translation keyEnumerator];
		while(key=[ke nextObject]){
			
			if (value=[trackDict objectForKey:key]){
				[newTrack setValue:value forKey:[translation objectForKey:key]];
			}
		}
	
	}
	NSLog(@"Writing Data");
	error = nil;
    if (![managedObjectContext save:&error]) {
		NSLog(@"Error: %@",error);
    }
	NSLog(@"Done");
	
	[pool release];
    return 0;
}
