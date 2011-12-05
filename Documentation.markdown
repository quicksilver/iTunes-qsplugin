<!-- Use your favorite Markdown tool to convert this and paste the HTML into the plist's extendedDescription. -->

## Preferences ##

#### Show Artwork ####

With this enabled, Quicksilver will show the album cover (for albums and tracks) or the poster frame (for videos). If disabled, a generic icon will be shown instead.

#### Group Compilations ####

With this enabled, tracks from different artists will be shown under a single album (if part of a compilation).

#### Monitor Recent Tracks ####

With this enabled, Quicksilver can store the last 10 tracks played by iTunes. It only includes tracks that have played since Quicksilver last started, and only while the preference was enabled.

#### Display Track Notifications ####

If **Monitor Recent Tracks** is enabled, you can optionally have Quicksilver display a notification every time a new track starts. The type of notification (built-in or Growl) can be controlled via Quicksilver's various preferences related to notifications.

#### Include Videos when Playing Albums ####

Some albums might have videos associated with them, but you probably don't want them to play when listening to an album. This allows you to control that behavior. The name is a bit misleading, as it will also apply if you select an artist and play them (which will play everything by that artist).

## Catalog ##

There are three catalog presets to choose from.

### iTunes Playlists ###

This will add the following to your main catalog:

  * All playlists
  * Entry points for browsing the library (Browse Artists, Browse Albums, etc.)
  * A "Recent Tracks" entry. (Select it and hit → or / to see them.)

These items can also be accessed by selecting iTunes in Quicksilver and hitting → or /.

Individual tracks are not added to the catalog as they're rarely sought out, and would really just slow Quicksilver down. Artists, Albums, Tracks, etc. can still be accessed quickly (see **Browsing** below).

### iTunes Controls ###

These are the same controls you can add triggers for (to control playback, adjust volume, and adjust rating). You can add them to the catalog in addition to (or instead of) assigning triggers to them.

### Scripts (iTunes) ###

This will add any AppleScripts you have in `~/Library/iTunes/Scripts/`. (There are none by default.)

## Browsing ##

You can quickly locate and play anything in your iTunes library, even if it's not stored in Quicksilver's catalog. You can do this by assigning triggers to search a certain criteria (like Artist or Genre) or by selecting "Browse [Criteria]" in Quicksilver.

## Actions ##

#### Play ####

You can play any of the following:

  * Playlists
  * Tracks (individually or with the comma trick)
  * Albums (individually or with the comma trick)
  * Artists (all tracks by the artist, individually or with the comma trick)
  * Genres (all tracks in a genre, individually or with the comma trick)
  * Composers (all tracks by the composer, individually or with the comma trick)

#### Add to Playlist… ####

Add artists, albums, tracks, etc. to an existing playlist by selecting it in the third pane.

#### Reveal ####

Select and show a playlist in the iTunes window.

#### Show Booklet(s) ####

With album(s) or artist(s) in the first pane, you can display the PDF artwork that accompanies some albums purchased from the iTunes store. Nothing will happen for albums that don't include artwork.

#### Play in iTunes DJ ####

Add tracks to the beginning of the list in iTunes DJ and start playing.

#### Play Next in iTunes DJ ####

Add tracks after the current track in iTunes DJ.

#### Add to End of iTunes DJ ####

Append tracks to the beginning of the list in iTunes DJ.

## Be Aware ##

The most accurate information comes from iTunes itself, but that only works if iTunes is running. In order to get information without requiring iTunes to be running at all times, we read it from disk (from `~/Music/iTunes/iTunes Music Library.xml`).

The information you see when browsing through your library comes from this XML file, but when you take an action like Play or Add to Playlist, the affected tracks are pulled from iTunes itself. This can lead to occasional inconsistencies in what you would see browsing in Quicksilver vs. what actually happens in iTunes.
