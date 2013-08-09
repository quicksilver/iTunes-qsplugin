//
//  QSiTunesSelectionSource.h
//  iTunesModule
//
//  Created by Rob McBroom on 2013/07/30.
//
//

@class iTunesApplication;

@interface QSiTunesSelectionSource : QSObjectSource
{
    iTunesApplication *iTunes;
}

@end
// TODO add support for AirPlay devices
// TODO improve "Current Selection" - have it grab the playlist if no track is selected
