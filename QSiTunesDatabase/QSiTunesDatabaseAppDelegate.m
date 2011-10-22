//  QSiTunesDatabaseAppDelegate.m
//  QSiTunesDatabase
//
//  Created by Nicholas Jitkoff on 3/21/05.
//  Copyright __MyCompanyName__ 2005 . All rights reserved.

#import "QSiTunesDatabaseAppDelegate.h"
@implementation NSString (LowerKeyConversion)
- (NSString *)lowerUnderscoredString{
	NSMutableString *string=[[self mutableCopy]autorelease];
	[string replaceOccurrencesOfString:@" " withString:@"_" options:nil range:NSMakeRange(0,[self length])];
	return [string lowercaseString];
}
@end

@implementation QSiTunesDatabaseAppDelegate

- (NSManagedObjectModel *)managedObjectModel {
    if (managedObjectModel) return managedObjectModel;
	
	NSMutableSet *allBundles = [[NSMutableSet alloc] init];
	[allBundles addObject: [NSBundle mainBundle]];
	[allBundles addObjectsFromArray: [NSBundle allFrameworks]];
    
    managedObjectModel = [[NSManagedObjectModel mergedModelFromBundles: [allBundles allObjects]] retain];
    [allBundles release];
    
    return managedObjectModel;
}

/* Change this path/code to point to your App's data store. */
- (NSString *)applicationSupportFolder {
    NSString *applicationSupportFolder = nil;
    FSRef foundRef;
    OSErr err = FSFindFolder(kUserDomain, kApplicationSupportFolderType, kDontCreateFolder, &foundRef);
    if (err != noErr) {
        NSRunAlertPanel(@"Alert", @"Can't find application support folder", @"Quit", nil, nil);
        [[NSApplication sharedApplication] terminate:self];
    } else {
        unsigned char path[1024];
        FSRefMakePath(&foundRef, path, sizeof(path));
        applicationSupportFolder = [NSString stringWithUTF8String:(char *)path];
        applicationSupportFolder = [applicationSupportFolder stringByAppendingPathComponent:@"QSiTunesDatabase"];
    }
    return applicationSupportFolder;
}

- (NSManagedObjectContext *) managedObjectContext {
    NSError *error;
    NSString *applicationSupportFolder = nil;
    NSURL *url;
    NSFileManager *fileManager;
    NSPersistentStoreCoordinator *coordinator;
    
    if (managedObjectContext) {
        return managedObjectContext;
    }
    
    fileManager = [NSFileManager defaultManager];
    applicationSupportFolder = [self applicationSupportFolder];
    if ( ![fileManager fileExistsAtPath:applicationSupportFolder isDirectory:NULL] ) {
        [fileManager createDirectoryAtPath:applicationSupportFolder attributes:nil];
    }
    
    url = [NSURL fileURLWithPath: [applicationSupportFolder stringByAppendingPathComponent: @"QSiTunesDatabase.db"]];
    coordinator = [[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel: [self managedObjectModel]];
    if ([coordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:nil URL:url options:nil error:&error]){
        managedObjectContext = [[NSManagedObjectContext alloc] init];
        [managedObjectContext setPersistentStoreCoordinator: coordinator];
    } else {
        [[NSApplication sharedApplication] presentError:error];
    }    
    [coordinator release];
    
    return managedObjectContext;
}

- (NSUndoManager *)windowWillReturnUndoManager:(NSWindow *)window {
    return [[self managedObjectContext] undoManager];
}

- (IBAction) saveAction:(id)sender {
	NSLog(@"save");
    NSError *error = nil;
    if (![[self managedObjectContext] save:&error]) {
        [[NSApplication sharedApplication] presentError:error];
    }
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification{
	//[controller add:nil];
	NSLog(@"finish");
	NSManagedObject *newTrack = nil;

	
	NSDictionary *music=[NSDictionary dictionaryWithContentsOfFile:@"/Volumes/Lux/Alchemy/Elements-private-svn/com.apple.iTunes/QSiTunesDatabase/Cube iTunes Music Library.xml"];
	NSDictionary *tracks=[music objectForKey:@"Tracks"];
	NSLog(@"music %d", [tracks count]);
	[music retain];
	NSArray *trackDicts=[tracks allValues];
	NSEnumerator *e=[tracks objectEnumerator];
	NSDictionary *trackDict=nil;
	
	
	//NSArray *properties=[NSEntityDescription entityForName:@"Track" inManagedObjectContext:[self managedObjectContext]];
	NSArray *properties=[@"Location,Name,Artist,Album,Composer,Genre,Track ID" componentsSeparatedByString:@","];
	NSDictionary *translation=[NSDictionary dictionaryWithObjects:[properties valueForKey:@"lowerUnderscoredString"] forKeys:properties];
	NSEnumerator *ke;
	NSString *key;
	id value;		
	//NSLog(@"track %@",translation);

	while(trackDict=[e nextObject]){
		newTrack=[NSEntityDescription insertNewObjectForEntityForName:@"Track"
											   inManagedObjectContext:[self managedObjectContext]];
		//NSLog(@"track %@",trackDict);
		ke=[translation keyEnumerator];
		while(key=[ke nextObject]){
			if (value=[trackDict objectForKey:key]){
				[newTrack setValue:value forKey:[translation objectForKey:key]];
			}
		}
		
	}
	
	[self saveAction:nil];
}


- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    NSError *error;
    NSManagedObjectContext *context;
    int reply = NSTerminateNow;
    
    context = [self managedObjectContext];
    if (context != nil) {
        if ([context commitEditing]) {
            if (![context save:&error]) {
				
				// This default error handling implementation should be changed to make sure the error presented includes application specific error recovery. For now, simply display 2 panels.
                BOOL errorResult = [[NSApplication sharedApplication] presentError:error];
				
				if (errorResult == YES) { // Then the error was handled
					reply = NSTerminateCancel;
				} else {
					
					// Error handling wasn't implemented. Fall back to displaying a "quit anyway" panel.
					int alertReturn = NSRunAlertPanel(nil, @"Could not save changes while quitting. Quit anyway?" , @"Quit anyway", @"Cancel", nil);
					if (alertReturn == NSAlertAlternateReturn) {
						reply = NSTerminateCancel;	
					}
				}
            }
        } else {
            reply = NSTerminateCancel;
        }
    }
    return reply;
}

@end
