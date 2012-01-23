# iTunes Plug-in Notes #

## Scripting Bridge ##

The Scripting Bridge code requires `iTunes.h`, which will be generated automatically the first time you build. When it's created, it'll be in `DerivedSources` (not part of your project). The easiest way to open it for reference is to highlight the filename on the import line and hit ⌃⌘J for "Jump to Definition".

You should [familiarize yourself with Scripting Bridge][sbdoc] a bit before making any changes. Particularly the sections that discuss performance.

In the end though, you just need to test everything out and see what works. Here are some tips specific to iTunes:

  1. When you want to look through "all tracks", use the Music playlist, not the Library playlist.
  
        iTunesApplication *iTunes = [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
        iTunesSource *library = [[[[iTunes sources] get] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"kind == %i", iTunesESrcLibrary]] objectAtIndex:0];
        iTunesLibraryPlaylist *lp = [[[[library playlists] get] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"specialKind == %i", iTunesESpKMusic]] objectAtIndex:0];
  
     Using the Library playlist can make certain things take thousands of times longer. (That's not an exaggeration.) Specifically, filtering out tracks with predicates involving `kind`, `videoKind`, or `albumArtist` is unusably slow (though it will eventually work). 
  
  2. When filtering the Music playlist for certain criteria, always call `get` on the resulting array. This can be tricky, since `filteredArrayUsingPredicate:` returns an NSArray which doesn't have a `get`, but it can be made to work by casting it as an SBElementArray.
  
        NSArray *tracks = [(SBElementArray *)[[musicPlaylist tracks] filteredArrayUsingPredicate:trackFilter] get];
  
     Depending on what you search for, everything you do with the result can be very slow. By calling `get` on it right away and using the result of that for everything else, you're essentially limiting the slowness to a single call.
  
  3. Always use `tracks` instead of `fileTracks`. `fileTracks` can slow things down even if you use the `get` trick above. Using `tracks` is quick. The only difference I've found is that the `location` property is sometimes missing from `tracks` while `fileTracks` will always have it. (You need location to add a track to a playlist.) But it turns out that using `get` on the result as advised above soemhow causes `tracks` to provide a `location` property. So, if you follow the above advice, this shouldn't be a problem.

[sbdoc]: http://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/ScriptingBridgeConcepts/Introduction/Introduction.html

## The iTunes Library and Quicksilver's Catalog ##

Most plug-ins put things directly into Quicksilver's catalog. iTunes typically holds a large number of items and it's unlikely that someone will want them all in the catalog. (Do you need every individual song at your fingertips?)

You may have noticed that browsing through artists, albums, playlists, etc. is very fast in Quicksilver even though most of these things aren't in the catalog. The reason is that the plug-in reads in the entire iTunes library from `~/Music/iTunes/iTunes Music Library.xml` into memory using `dictionaryWithContentsOfFile:`. The `QSObject`s you see when browsing around are generated on-the-fly from what's in the dictionary.

About Quicksilver Plugins on Github
===================================

This repository contains the current source code of a the Quicksilver Plugin / Module. If you're having issues with this plugin, feel free to log them at the [Quicksilver issue tracker](https://github.com/quicksilver/Quicksilver/issues).

Always be sure to check the [Google Groups](http://groups.google.com/group/blacktree-quicksilver/topics?gvc=2) first incase there's a solution to your problem, as well as the [QSApp.com Wiki](http://qsapp.com/wiki/).


Before You Try It Out
---------------------

Before trying out any of these plugins, it's always a good idea to **BACKUP** all of your Quicksilver data.

This is easily done by backing up the following folders 

(`<user>` stands for your short user name):

`/Users/<user>/Library/Application Support/Quicksilver`  
`/Users/<user>/Library/Caches/Quicksilver`

	
Before Building
---------------

Before being able to build any of these plugins, you **MUST** set a new Source Tree for the `QSFramework` in the XCode Preferences.

This is done by going into the XCode preferences, clicking 'Source Trees' and adding a new one with the following options:

Setting Name: `QSFrameworks`  
Display Name: a suitable name, e.g. `Quicksilver Frameworks`  
Path: `/Applications/Quicksilver.app/Contents/Frameworks` (or path of Quicksilver.app if different)

For some plugins to compile correctly a source tree must also be set for `QS_SOURCE_ROOT` that points to the location of the [Quicksilver source code](https://github.com/quicksilver/Quicksilver) you've downloaded onto your local machine.

Setting Name: `QS_SOURCE_ROOT`	
Display Name: a suitable name, e.g. `Quicksilver source code root`	 
Path: `/Users/<user>/<path to Quicksilver source code>`

See the QSApp.com wiki for more information on [Building Quicksilver](http://qsapp.com/wiki/Building_Quicksilver).

Also check out the [Quicksilver Plugins Development Reference](http://projects.skurfer.com/QuicksilverPlug-inReference.mdown), especially the [Building and Testing section](http://projects.skurfer.com/QuicksilverPlug-inReference.mdown#building_and_testing).

Legal Stuff 
-----------

By downloading and/or using this software you agree to the following terms of use:

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this software except in compliance with the License.
    You may obtain a copy of the License at
    
      http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


Which basically means: whatever you do, I can't be held accountable if something breaks.