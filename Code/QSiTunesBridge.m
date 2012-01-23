//
//  QSiTunesBridge.m
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "QSiTunesBridge.h"

static iTunesApplication *iTunes;

iTunesApplication *QSiTunes()
{
	if (!iTunes) {
		iTunes = [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
	}
	return iTunes;
}

iTunesSource *QSiTunesLibrary()
{
	NSArray *librarySource = [[[QSiTunes() sources] get] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"kind == %i", iTunesESrcLibrary]];
	if ([[librarySource objectAtIndex:0] exists]) {
		return [librarySource objectAtIndex:0];
	}
	return nil;
}

iTunesLibraryPlaylist *QSiTunesMusic()
{
	NSArray *iTunesMusicPlaylists = [[[QSiTunesLibrary() playlists] get] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"specialKind == %i", iTunesESpKMusic]];
	if ([[iTunesMusicPlaylists objectAtIndex:0] exists]) {
		return [iTunesMusicPlaylists objectAtIndex:0];
	}
	return nil;
}

iTunesPlaylist *QSiTunesDJ()
{
	NSArray *iTunesDJplaylists = [[[QSiTunesLibrary() playlists] get] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"specialKind == %i", iTunesESpKPartyShuffle]];
	if ([[iTunesDJplaylists objectAtIndex:0] exists]) {
		return [iTunesDJplaylists objectAtIndex:0];
	}
	return nil;
}
