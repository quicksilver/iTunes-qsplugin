//
//  QSiTunesSelectionSource.h
//  iTunesModule
//
//  Created by Rob McBroom on 2013/07/30.
//
//

@class iTunesApplication;
@class QSiTunesDatabase;

@interface QSiTunesSelectionSource : QSObjectSource
{
    iTunesApplication *iTunes;
    QSiTunesDatabase *library;
}

@end
// TODO add support for AirPlay devices
// TODO improve "Current Selection" - have it grab the playlist if no track is selected
