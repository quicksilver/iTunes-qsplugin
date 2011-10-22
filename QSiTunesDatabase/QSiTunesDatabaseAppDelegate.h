//  QSiTunesDatabaseAppDelegate.h
//  QSiTunesDatabase
//
//  Created by Nicholas Jitkoff on 3/21/05.
//  Copyright __MyCompanyName__ 2005 . All rights reserved.

#import <Cocoa/Cocoa.h>

@interface QSiTunesDatabaseAppDelegate : NSObject 
{
    IBOutlet NSWindow *window;
    
    NSManagedObjectModel *managedObjectModel;
    NSManagedObjectContext *managedObjectContext;
	IBOutlet NSArrayController *controller;
}

- (NSManagedObjectModel *)managedObjectModel;
- (NSManagedObjectContext *)managedObjectContext;

- (IBAction)saveAction:sender;

@end
