//
//  QSiTunesBridge.h
//  iTunesPlugIn
//
//  Created by Nicholas Jitkoff on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "iTunes.h"

iTunesApplication *QSiTunes();  // returns the iTunes application
iTunesSource *QSiTunesLibrary(); // returns the main Library "source" (the container for all playlists)
iTunesLibraryPlaylist *QSiTunesMusic(); // returns the playlist containing all music
iTunesPlaylist *QSiTunesDJ(); // returns the iTunes DJ playlist
