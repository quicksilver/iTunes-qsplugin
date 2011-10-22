/*
 *
 *
 * Copyright (c) 2004 Samuel Wood (sam.wood@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 */

// iPodDB.h: interface for the iPod classes.
//
//////////////////////////////////////////////////////////////////////

#ifndef __IPODDB_H__
#define __IPODDB_H__

#pragma once

#pragma warning( disable : 4786)

#include <map>
#include <vector>
#include <algorithm>

#ifdef _DEBUG
#undef ASSERT
#define ASSERT(x) assert(x)
#else
#define ASSERT(x) {}
#endif


using namespace std;

// mhod types
#define MHOD_TITLE			1
#define MHOD_LOCATION		2
#define MHOD_ALBUM			3
#define MHOD_ARTIST			4
#define MHOD_GENRE			5
#define MHOD_FILETYPE		6
#define MHOD_EQSETTING		7
#define MHOD_COMMENT		8
#define MHOD_COMPOSER		12
#define MHOD_GROUPING		13
#define MHOD_SPLPREF		50
#define MHOD_SPLDATA		51
#define MHOD_PLAYLIST		100

// not sure what this is for, but it's in some mhyps
#define MHOD_MHYP			52


// Smart Playlist stuff
#define SPLMATCH_AND	0		// AND rule - all of the rules must be true in order for the combined rule to be applied
#define SPLMATCH_OR		1		// OR rule

// Limit Types.. like limit playlist to 100 minutes or to 100 songs
#define LIMITTYPE_MINUTES	0x01
#define LIMITTYPE_MB		0x02
#define LIMITTYPE_SONGS		0x03
#define LIMITTYPE_HOURS		0x04
#define LIMITTYPE_GB		0x05

// Limit Sorts.. Like which songs to pick when using a limit type
// Special note: the values for LIMITSORT_LEAST_RECENTLY_ADDED, LIMITSORT_LEAST_OFTEN_PLAYED,
//		LIMITSORT_LEAST_RECENTLY_PLAYED, and LIMITSORT_LOWEST_RATING are really 0x10, 0x14,
//		0x15, 0x17, with the 'limitsort_opposite' flag set.  This is the same value as the
//		"positive" value (i.e. LIMITSORT_LEAST_RECENTLY_ADDED), and is really very terribly
//		awfully weird, so we map the values to iPodDB specific values with the high bit set.
//
//		On writing, we check the high bit and write the limitsort_opposite from that. That
//		way, we don't have to deal with programs using the class needing to set the wrong
//		limit and then make it into the "opposite", which would be frickin' annoying.
#define LIMITSORT_RANDOM					0x02
#define LIMITSORT_SONG_NAME					0x03
#define LIMITSORT_ALBUM						0x04
#define LIMITSORT_ARTIST					0x05
#define LIMITSORT_GENRE						0x07
#define LIMITSORT_MOST_RECENTLY_ADDED		0x10
#define LIMITSORT_LEAST_RECENTLY_ADDED		0x80000010  // See note above
#define LIMITSORT_MOST_OFTEN_PLAYED			0x14
#define LIMITSORT_LEAST_OFTEN_PLAYED		0x80000014  // See note above
#define LIMITSORT_MOST_RECENTLY_PLAYED		0x15
#define LIMITSORT_LEAST_RECENTLY_PLAYED		0x80000015  // See note above
#define LIMITSORT_HIGHEST_RATING			0x17
#define LIMITSORT_LOWEST_RATING				0x80000017  // See note above

// Smartlist Actions - Used in the rules.
/*
Note by Otto:
 really this is a bitmapped field...
 high byte
 bit 0 = "string" values if set, "int" values if not set
 bit 1 = "not", or to negate the check.
 lower 2 bytes
 bit 0 = simple "IS" query
 bit 1 = contains
 bit 2 = begins with
 bit 3 = ends with
 bit 4 = greater than
 bit 5 = unknown, but probably greater than or equal to
 bit 6 = less than
 bit 7 = unknown, but probably less than or equal to
 bit 8 = a range selection
 bit 9 = "in the last"
*/
#define SPLACTION_IS_INT				0x00000001		// Also called "Is Set" in iTunes
#define SPLACTION_IS_GREATER_THAN		0x00000010		// Also called "Is After" in iTunes
#define SPLACTION_IS_LESS_THAN			0x00000040		// Also called "Is Before" in iTunes
#define SPLACTION_IS_IN_THE_RANGE		0x00000100
#define SPLACTION_IS_IN_THE_LAST		0x00000200

#define SPLACTION_IS_STRING				0x01000001
#define SPLACTION_CONTAINS				0x01000002
#define SPLACTION_STARTS_WITH			0x01000004
#define SPLACTION_ENDS_WITH				0x01000008

#define SPLACTION_IS_NOT_INT			0x02000001		// Also called "Is Not Set" in iTunes
#define SPLACTION_IS_NOT_GREATER_THAN	0x02000010		// Note: Not available in iTunes 4.5 (untested on iPod)
#define SPLACTION_IS_NOT_LESS_THAN		0x02000040		// Note: Not available in iTunes 4.5 (untested on iPod)
#define SPLACTION_IS_NOT_IN_THE_RANGE	0x02000100		// Note: Not available in iTunes 4.5, but seems to work on the iPod
#define SPLACTION_IS_NOT_IN_THE_LAST	0x02000200

#define SPLACTION_IS_NOT				0x03000001
#define SPLACTION_DOES_NOT_CONTAIN		0x03000002
#define	SPLACTION_DOES_NOT_START_WITH	0x03000004		// Note: Not available in iTunes 4.5, but seems to work on the iPod
#define	SPLACTION_DOES_NOT_END_WITH		0x03000008		// Note: Not available in iTunes 4.5, but seems to work on the iPod

// these are to pass to AddRule() when you need a unit for the two "in the last" action types
// Or, in theory, you can use any time range... iTunes might not like it, but the iPod might.
#define SPLACTION_LAST_DAYS_VALUE		86400		// number of seconds in 24 hours
#define SPLACTION_LAST_WEEKS_VALUE		604800		// number of seconds in 7 days
#define SPLACTION_LAST_MONTHS_VALUE		2628000		// number of seconds in 30.4167 days ~= 1 month

// Hey, why limit ourselves to what iTunes can do? If the iPod can deal with it, excellent!
#define SPLACTION_LAST_HOURS_VALUE		3600		// number of seconds in 1 hour
#define SPLACTION_LAST_MINUTES_VALUE	60			// number of seconds in 1 minute
#define SPLACTION_LAST_YEARS_VALUE		31536000 	// number of seconds in 365 days

// fun ones.. Near as I can tell, all of these work. It's open like that. :)
#define SPLACTION_LAST_LUNARCYCLE_VALUE	2551443		// a "lunar cycle" is the time it takes the moon to circle the earth
#define SPLACTION_LAST_SIDEREAL_DAY		86164		// a "sidereal day" is time in one revolution of earth on its axis
#define SPLACTION_LAST_SWATCH_BEAT		86			// a "swatch beat" is 1/1000th of a day.. search for "internet time" on google
#define SPLACTION_LAST_MOMENT			90			// a "moment" is 1/40th of an hour, or 1.5 minutes
#define SPLACTION_LAST_OSTENT			600			// an "ostent" is 1/10th of an hour, or 6 minutes
#define SPLACTION_LAST_FORTNIGHT		1209600 	// a "fortnight" is 14 days
#define SPLACTION_LAST_VINAL			1728000 	// a "vinal" is 20 days
#define SPLACTION_LAST_QUARTER			7889231		// a "quarter" is a quarter year
#define SPLACTION_LAST_SOLAR_YEAR       31556926 	// a "solar year" is the time it takes the earth to go around the sun
#define SPLACTION_LAST_SIDEREAL_YEAR 	31558150	// a "sidereal year" is the time it takes the earth to reach the same point in space again, compared to the stars

// Smartlist fields - Used for rules.
#define SPLFIELD_SONG_NAME		0x02	// String
#define SPLFIELD_ALBUM			0x03	// String
#define SPLFIELD_ARTIST			0x04	// String
#define SPLFIELD_BITRATE		0x05	// Int	(e.g. from/to = 128)
#define SPLFIELD_SAMPLE_RATE	0x06	// Int  (e.g. from/to = 44100)
#define SPLFIELD_YEAR			0x07	// Int  (e.g. from/to = 2004)
#define SPLFIELD_GENRE			0x08	// String
#define SPLFIELD_KIND			0x09	// String
#define SPLFIELD_DATE_MODIFIED	0x0a	// Int/Mac Timestamp (e.g. from/to = bcf93280 == is before 6/19/2004)
#define SPLFIELD_TRACKNUMBER	0x0b	// Int  (e.g. from = 1, to = 2)
#define SPLFIELD_SIZE			0x0c	// Int  (e.g. from/to = 0x00600000 for 6MB)
#define SPLFIELD_TIME			0x0d	// Int  (e.g. from/to = 83999 for 1:23/83 seconds)
#define SPLFIELD_COMMENT		0x0e	// String
#define SPLFIELD_DATE_ADDED		0x10	// Int/Mac Timestamp (e.g. from/to = bcfa83ff == is after 6/19/2004)
#define SPLFIELD_COMPOSER		0x12	// String
#define SPLFIELD_PLAYCOUNT		0x16	// Int  (e.g. from/to = 1)
#define SPLFIELD_LAST_PLAYED	0x17	// Int/Mac Timestamp (e.g. from = bcfa83ff (6/19/2004), to = 0xbcfbd57f (6/20/2004))
#define SPLFIELD_DISC_NUMBER	0x18	// Int  (e.g. from/to = 1)
#define SPLFIELD_RATING			0x19	// Int/Stars Rating  (e.g. from/to = 60 (3 stars))
#define SPLFIELD_COMPILATION	0x1f	// Int  (e.g. is set -> SPLACTION_IS_INT/from=1, is not set -> SPLACTION_IS_NOT_INT/from=1)
#define SPLFIELD_BPM			0x23	// Int  (e.g. from/to = 60)
#define SPLFIELD_GROUPING		0x27	// String
#define SPLFIELD_PLAYLIST		0x28	// XXX - Unknown...not parsed correctly...from/to = 0xb6fbad5f for "Purchased Music".  Extra data after "to"...


#define SPLDATE_IDENTIFIER		0x2dae2dae2dae2dae


// useful functions
time_t mactime_to_wintime (const unsigned long mactime);
unsigned long wintime_to_mactime (const time_t time);
char * UTF16_to_char(unsigned short * str, int length);


// Pre-declare iPod_* classes
class iPod_mhbd;
class iPod_mhsd;
class iPod_mhlt;
class iPod_mhit;
class iPod_mhlp;
class iPod_mhyp;
class iPod_slst;
class iPod_mhip;
class iPod_mhod;

// Maximum string length that iTunes writes to the database
#define SPL_MAXSTRINGLENGTH 255

// a struct to hold smart playlist rules in mhods
struct SPLRule
{
	SPLRule() :
		field(0),
		action(0),
		length(0),
		fromvalue(0),
		fromdate(0),
		fromunits(0),
		tovalue(0),
		todate(0),
		tounits(0),
		unk1(0),
		unk2(0),
		unk3(0),
		unk4(0),
		unk5(0)
	{
		memset(string, 0, sizeof(string));
	}

	void SetString(const unsigned short *value)
	{
		if(value)
		{
		//	wcsncpy(string, value, SPL_MAXSTRINGLENGTH); 
		//	length = wcslen(string) * 2;
		}
		else
		{
			memset(string, 0, sizeof(string));
			length = 0;
		}

	}

	unsigned long field;
	unsigned long action;
	unsigned long length;
	unsigned short string[SPL_MAXSTRINGLENGTH + 1];

	// from and to are pretty stupid.. if it's a date type of field, then
	// value = 0x2dae2dae2dae2dae,
	// date = some number, like 2 or -2
	// units = unit in seconds, like 604800 = a week
	// but if this is actually some kind of integer comparison, like rating = 60 (3 stars)
	// value = the value we care about
	// date = 0
	// units = 1
	// So we leave these as they are, and will just deal with it in the rules functions.
	unsigned long long fromvalue;
	signed long long fromdate;
	unsigned long long fromunits;
	unsigned long long tovalue;
	signed long long todate;
	unsigned long long tounits;
	unsigned long unk1;
	unsigned long unk2;
	unsigned long unk3;
	unsigned long unk4;
	unsigned long unk5;
};


// PCEntry: Play Count struct for the entries in iPod_mhdp
struct PCEntry
{
	unsigned long playcount;
	unsigned long lastplayedtime;
	unsigned long bookmarktime; // used to be "unk1". It's the time, in milliseconds, at which you stop playing an M4B audiobook file, I think.
	unsigned long stars;
};


/**************************************
       iTunesDB Database Layout

  MHBD (Database)
  |
  |-MHSD (Dual Sub-Databases)
  | |
  | |-MHLT (Tunes List)
  | | |
  | | |-MHIT (Tune Item)
  | | | |
  | | | |-MHOD (Description Object)
  | | | |-MHOD
  | | | | ...
  | | |
  | | |-MHIT
  | | | |
  | | | |-MHOD
  | | | |-MHOD
  | | | | ...
  | | |
  | | |-...
  |
  |
  |-MHSD
  | |
  | |-MHLP (Playlists List)
  | | |
  | | |-MHYP (Playlist)
  | | | |
  | | | |-MHOD
  | | | |-MHIP (Playlist Item)
  | | | | ...
  | | |
  | | |-MHYP
  | | | |
  | | | |-MHOD
  | | | |-MHIP
  | | | | ...
  | | |
  | | |-...

**************************************/


// base class, not used directly
class iPodObj
{
public:
	iPodObj();
	virtual ~iPodObj();

	// parse function is required in all subclasses
	// feed it a iTunesDB, it creates an object hierarchy
	virtual long parse(unsigned char * data) = 0;

	// write function is required too
	// feed it a buffer and the size of the buffer, it fills it with an iTunesDB
	// return value is size of the resulting iTunesDB
	// return of -1 means the buffer was too small
	virtual long write(unsigned char * data, const unsigned long datasize) = 0;

	unsigned long size_head;
	unsigned long size_total;
};

// MHBD: The database - parent of all items
class iPod_mhbd : public iPodObj
{
public:
	iPod_mhbd();
	virtual ~iPod_mhbd();

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	unsigned long unk1;
	unsigned long unk2;
	unsigned long children;
	unsigned long unk3;
	unsigned long unk4;
	unsigned long unk5;
	iPod_mhsd * mhsdsongs;
	iPod_mhsd * mhsdplaylists;
};

// MHSD: List container - parent of MHLT or MHLP, child of MHBD
class iPod_mhsd : public iPodObj
{
public:
	iPod_mhsd();
	iPod_mhsd(int newindex);
	virtual ~iPod_mhsd();

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	unsigned long index; // 1 = mhlt, 2 = mhlp
	iPod_mhlt * mhlt;
	iPod_mhlp * mhlp;
};

// MHLT: song list container - parent of MHIT, child of MHSD
class iPod_mhlt : public iPodObj
{
public:
	typedef map<unsigned long, iPod_mhit*> mhit_map_t;		// Map the unique mhit.id to a mhit object
	typedef mhit_map_t::value_type mhit_value_t;

	iPod_mhlt();
	virtual ~iPod_mhlt();

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	const unsigned long GetChildrenCount() const	{ return mhit.size(); }

	// returns a pointer to the new iPod_mhit object in the track list, which you edit directly
	iPod_mhit * AddTrack(const unsigned int requestedID = 0);

	// takes a position index, returns a pointer to the track itself, or NULL if the index isn't found.
	iPod_mhit * GetTrack(const unsigned long index);

	// searches for a track based on the track's id number (mhit.id). returns mhit pointer, or NULL if the id isn't found.
	iPod_mhit * GetTrackByID(const unsigned long id);

	bool DeleteTrack(const unsigned	long index);
	bool DeleteTrackByID(const unsigned long id);

	// clears out the tracklist
	bool ClearTracks(const bool clearMap = true);

	mhit_map_t mhit;

private:
	unsigned long next_mhit_id;
};

// MHIT: song item - parent of MHOD, child of MHLT
class iPod_mhit : public iPodObj
{
public:
	iPod_mhit();
	virtual ~iPod_mhit();

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	const unsigned long GetChildrenCount() const	{ return(mhod.size()); }

	// will add a new mhod string to the mhit
	// optional: pass in a type to get an existing string, if there is one,
	//			 or a new one with the type filled in already, if there is not one
	iPod_mhod * AddString(const int type=0);

	// Find a string by type
	iPod_mhod * FindString(const unsigned long type);

	// deletes a string from the track
	// if more than one string of given type exists, all of that type will be deleted,
	//		to ensure consistency. Pointers to these strings will be invalid after this.
	// return val is how many strings were deleted
	unsigned long DeleteString(const unsigned long type);

	// Creates a copy of the mhit. The operator = is overloaded so you can
	// more easily copy mhit's.
	static void Duplicate(const iPod_mhit *src, iPod_mhit *dst);
	iPod_mhit& operator=(const iPod_mhit& src);

	int GetRating() { return stars/20; }
	void SetRating(int rating) { stars=rating*20; }

	unsigned long id;
	unsigned long unk1;
	unsigned long unk2;
	unsigned long type;
	unsigned char compilation;
	unsigned char stars;
	unsigned long createdtime;
	unsigned long size;
	unsigned long length;
	unsigned long tracknum;
	unsigned long totaltracks;
	unsigned long year;
	unsigned long bitrate;
	unsigned long samplerate;
	unsigned long volume;
	unsigned long starttime;
	unsigned long stoptime;
	unsigned long soundcheck;
	unsigned long playcount;
	unsigned long unk4;
	unsigned long lastplayedtime;
	unsigned long cdnum;
	unsigned long totalcds;
	unsigned long unk5;
	unsigned long lastmodtime;
	unsigned long bookmarktime; // This used to be "unk6". It's the time, in milliseconds, that the iPod will start playing an M4B audiobook file.
	unsigned long unk7;	// this seems to continuously count up, for every song. it may remain counting between syncs. I have no idea why.
	unsigned long unk8; // ditto?
	unsigned long BPM;
	unsigned long app_rating;	// The rating set by the application, as opposed to the rating set on the iPod itself
	unsigned char checked;		// a "checked" song has the value of 0, a non-checked song is 1
	unsigned long unk9;
	unsigned long unk10;
	unsigned long unk11;
	unsigned long unk12;
	unsigned long unk13;
	unsigned long unk14;
	unsigned long unk15;
	unsigned long unk16;

	vector<iPod_mhod*> mhod;
};


// MHLP: playlist container - parent of MHYP, child of MHSD

// Important note: Playlist zero must always be the default playlist, containing every
// track in the DB. To do this, always call "GetDefaultPlaylist()" before you create any
// other playlists, if you start from scratch.
// After you're done adding/deleting tracks in the database, and just before you call
// write(), do the following: GetDefaultPlaylist()->PopulatePlaylist(ptr_to_mhlt);

class iPod_mhlp : public iPodObj
{
public:
	iPod_mhlp();
	virtual ~iPod_mhlp();

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	const unsigned long GetChildrenCount() const	{ return mhyp.size(); }

	// returns a new playlist for you
	iPod_mhyp * AddPlaylist();
	iPod_slst * AddSmartPlaylist();

	// gets a playlist
	iPod_mhyp * GetPlaylist(const unsigned long pos) const			{ return mhyp.at(pos); }

	// finds a playlist by its ID
	iPod_mhyp * FindPlaylist(const unsigned long long playlistID);

	// deletes the playlist at a position
	bool DeletePlaylist(const unsigned long pos);

	// gets the default playlist ( GetPlaylist(0); )
	// if there are no playlists yet (empty db), then it creates the default playlist
	//    and returns a pointer to it
	iPod_mhyp * GetDefaultPlaylist();

	// erases all playlists, including the default one, so be careful here.
	// Set createDefaultPlaylist to create a new, empty default playlist
	bool ClearPlaylists(const bool createDefaultPlaylist = false);

	vector<iPod_mhyp*> mhyp;

private:
	bool beingDeleted;
};

// MHYP: playlist - parent of MHOD or MHIP, child of MHLP
class iPod_mhyp : public iPodObj
{
public:
	iPod_mhyp();
	virtual ~iPod_mhyp();

	// Returns an unique 64 bit value suitable for use as playlistID
	static unsigned long long iPod_mhyp::GeneratePlaylistID();

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	bool IsSmartPlaylist(void) const  { return(isSmartPlaylist); }

	// add an entry to the playlist. Creates a new entry, returns the position in the vector
	// optionally fills in the songindex for you, with the ID from a track you might have
	long AddPlaylistEntry(iPod_mhip * entry, const unsigned long id=0);

	// give it a song id, it'll return a position in the playlist
	// -1, as always, means not found
	// if the same entry is in the playlist multiple times, this only gives back the first one
	long FindPlaylistEntry(const unsigned long id);

	// get an mhip given its position
	iPod_mhip * GetPlaylistEntry(const unsigned long pos) const		{ return mhip.at(pos); }

	// deletes an entry from the playlist. Pointers to that entry become invalid
	bool DeletePlaylistEntry(const unsigned long pos);

	// clears a playlist of all mhip entries
	bool ClearPlaylist();

	// populates a playlist to be the same as a track list you pass into it.
	// Mainly only useful for building the default playlist after you add/delete tracks
	// GetDefaultPlaylist()->PopulatePlaylist(ptr_to_mhlt);
	// for example...
	long PopulatePlaylist(iPod_mhlt * tracks, int hidden_field=1);

	// will add a new string to the playlist
	// optional: pass in a type to get an existing string, if there is one,
	//			 or a new one with the type filled in already, if there is not one
	iPod_mhod * AddString(const int type=0);

	// get an mhod given it's type.. Only really useful with MHOD_TITLE here, until
	// smartlists get worked out better
	iPod_mhod * FindString(const unsigned long type);

	// deletes a string from the playlist
	// if more than one string of given type exists, all of that type will be deleted,
	//		to ensure consistency. Pointers to these strings will be invalid after this.
	// ret val is number of strings removed
	unsigned long DeleteString(const unsigned long type);

	void SetPlaylistTitle(const unsigned short *string);

	const unsigned long GetMhodChildrenCount() const	{ return mhod.size(); }
	const unsigned long GetMhipChildrenCount() const	{ return mhip.size(); }

	unsigned long hidden;
	unsigned long timestamp;
	unsigned long long playlistID;	// ID of the playlist, used in smart playlist rules
	unsigned long unk3;
	unsigned long unk4;
	unsigned long unk5;

	vector<iPod_mhod*> mhod;
	vector<iPod_mhip*> mhip;

protected:
	bool isSmartPlaylist;
	bool isPopulated;
};


// MHIP: playlist item - child of MHYP
class iPod_mhip : public iPodObj
{
public:
	iPod_mhip();
	virtual ~iPod_mhip();

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	unsigned long unk1;
	unsigned long corrid;	// totally unsure what the hell this is for now...
	unsigned long unk2;
	unsigned long songindex;
	unsigned long timestamp;
};

// MHOD: string container item, child of MHIT or MHYP
class iPod_mhod : public iPodObj
{
public:
	iPod_mhod();
	virtual ~iPod_mhod();

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	void SetString(const unsigned short *string);

	static void Duplicate(iPod_mhod *src, iPod_mhod *dst);

	unsigned long type;
	unsigned long unk1;
	unsigned long unk2;

	// renamed this from corrid.. all it is is a position in the playlist
	// for type 100 mhods that come immediately after mhips.
	// for strings, it's always set to "1".
	unsigned long position;

	unsigned long length;
	unsigned long unk3;
	unsigned long unk4;

	// string mhods get the string put here, unaltered, still byte reversed
	// Use unicode functions to work with this string.
	unsigned short *str;

	// mhod types 50 and up get the whole thing put here.
	// until I can figure out all of these, I won't bother to try to recreate them
	// and i'll just copy them back as needed when rewriting the iTunesDB file.
	unsigned char * binary;

	// stuff for type 50 mhod
	unsigned char liveupdate;			// "Live Updating" check box
	unsigned char checkrules;			// "Match X of the following conditions" check box
	unsigned char checklimits;			// "Limit To..." check box.		1 = checked, 0 = not checked
	unsigned char matchcheckedonly;		// "Match only checked songs" check box.
	unsigned char limitsort_opposite;	// Limit Sort rule is reversed (e.g. limitsort == LIMIT_HIGHEST_RATING really means LIMIT_LOWEST_RATING...quite weird...)
	unsigned long limittype;			// See Limit Types defines above
	unsigned long limitsort;			// See Limit Sort defines above
	unsigned long limitvalue;			// Whatever value you type next to "limit type".

	// stuff for type 51 mhod
	unsigned long unk5;					// not sure, probably junk data
	unsigned long rules_operator;		// "All" (logical AND / value = 0) or "Any" (logical OR / value = 1).
	vector<SPLRule *> rule;

	bool parseSmartPlaylists;
};


// Smart Playlist.  A smart playlist doesn't act different from a regular playlist,
// except that it contains a type 50 and type 51 MHOD.  But deriving the iPod_slst
// class makes sense, since there are a lot of functions that are only appropriate
// for smart playlists, and it can guarantee that a type 50 and 51 MHOD will always
// be available.
class iPod_slst  : public iPod_mhyp
{
public:
	enum FieldType
	{
		ftString,
		ftInt,
		ftBoolean,
		ftDate,
		ftPlaylist,
		ftUnknown
	};

	enum ActionType
	{
		atString,
		atInt,
		atBoolean,
		atDate,
		atRange,
		atInTheLast,
		atPlaylist,
		atNone,
		atInvalid,
		atUnknown
	};


	iPod_slst();
	virtual ~iPod_slst();

	iPod_mhod* GetPrefs(void) { UpdateMHODPointers(); return(splPref); }
	void SetPrefs(const bool liveupdate = true, const bool rules_enabled = true, const bool limits_enabled = false,
				  const unsigned long limitvalue = 0, const unsigned long limittype = 0, const unsigned long limitsort = 0);

	static FieldType  GetFieldType(const unsigned long field);
	static ActionType GetActionType(const unsigned long field, const unsigned long action);

	static unsigned long long ConvertDateValueToNum(const unsigned long long val)	{ return(-val); }
	static unsigned long long ConvertNumToDateValue(const unsigned long long val)	{ return(-val); }

	// returns a pointer to the SPLDATA mhod
	iPod_mhod* GetRules() { UpdateMHODPointers(); return(splData); }

	// get the number of rules in the smart playlist
	unsigned long GetRuleCount();

	// Returns rule number (0 == first rule, -1 == error)
	int AddRule(const unsigned long field,
		        const unsigned long action,
				const unsigned short * string = NULL,	// use string for string based rules
				const unsigned long long value  = 0,		// use value for single variable rules
				const unsigned long long from   = 0,		// use from and to for range based rules
				const unsigned long long to     = 0,
				const unsigned long long units  = 0);		// use units for "in the last" based rules

	int AddStringRule(const unsigned long field, const unsigned long action, const unsigned short * string);
	int AddSingleValueRule(const unsigned long field, const unsigned long action, const unsigned long long value);
	int AddRangeRule(const unsigned long field, const unsigned long action, const unsigned long long from, const unsigned long long to);
	int AddInTheLastRule(const unsigned long field, const unsigned long action, const unsigned long long value, const unsigned long long units);

	bool DeleteRule(const unsigned int index);

	SPLRule * GetRule(const unsigned int index);

	// this is to get a rule without resorting to the SPLRule struct (which should only really be
	// used internally anyway).
	// since there's no case in which you need both the value and the date fields,
	// single values just get stuck in "from".
	bool GetRule(const unsigned int index,
				 unsigned long field,
				 unsigned long action,
				 unsigned short string[SPL_MAXSTRINGLENGTH+1],
				 unsigned long length,	// length of string
				 unsigned long long from,
				 unsigned long long to,
				 unsigned long long units);


	void RemoveAllRules(void);

	// populates a smart playlist
	// Pass in the mhlt with all the songs on the iPod, and it populates the playlist
	//	given those songs and the current rules
	// Return value is number of songs in the resulting playlist.
	long PopulateSmartPlaylist(iPod_mhlt * tracks, iPod_mhlp * playlists);

	// used in PopulateSmartPlaylist
	static bool EvalRule(
		SPLRule * r,
		iPod_mhit * track,
		iPod_mhlt * tracks = NULL,		// if you're going to allow playlist type rules
		iPod_mhlp * playlists = NULL	// these are required to be passed in
		);

	// Restore default prefs and remove all rules
	void Reset(void);

protected:
	void UpdateMHODPointers(void);

	iPod_mhod *splPref;
	iPod_mhod *splData;
};




// MHDP: Play Count class
class iPod_mhdp
{
public:
	unsigned long size_head;
	unsigned long entrysize;

	const unsigned long GetChildrenCount() const	{ return entry.size(); }

	// return value is number of songs or -1 if error.
	// you should probably check to make sure the number of songs is the same
	//     as the number of songs you read in from parsing the iTunesDB
	virtual long parse(unsigned char * data);

	// there is no write() function because there is no conceivable need to ever write a
	// play counts file.

	PCEntry GetPlayCount(const unsigned int pos) const		{ return entry.at(pos); }

	// playcounts are stored in the Play Counts file, in the same order as the mhits are
	//  stored in the iTunesDB. So you should apply the changes from these entries to the
	//  mhits in order and then probably delete the Play Counts file entirely to prevent
	//  doing it more than once.
	vector<PCEntry> entry;
};




// MHPO: On-The-Go Playlist class
class iPod_mhpo
{
public:
	iPod_mhpo();
	virtual ~iPod_mhpo();

	unsigned long size_head;
	unsigned long unk1;
	unsigned long unk2;	// this looks like a timestamp, sorta

	const unsigned long GetChildrenCount() const	{ return idList.size(); }

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	// This will create a new playlist from the OTGPlaylist..
	// Give it the DB to create the playlist in and from, and a name for the playlist.
	// Return value is a pointer to the playlist itself, which will be inside the DB you
	// give to it as well.
	// Returns NULL on error (can't create the playlist)
	iPod_mhyp * CreatePlaylistFromOTG(iPod_mhbd * iPodDB, unsigned short * name);

	// OTGPlaylists are stored in the OTGPlaylist file. When iTunes copies them into a
	// new playlist, it deletes the file afterwards. I do not know if creating this file
	// will make the iPod have an OTGPlaylist after you undock it. I added the write function
	// anyway, in case somebody wants to try it. Not much use for it though, IMO.
	vector<unsigned long> idList;
};


// eq presets classes
class iPod_mqed;
class iPod_pqed;

// MQED: EQ Presets holder
class iPod_mqed
{
public:
	iPod_mqed();
	virtual ~iPod_mqed();

	unsigned long size_head;
	unsigned long unk1;
	unsigned long unk2;	// this looks like a timestamp, sorta

	const unsigned long GetChildrenCount() const	{ return eqList.size(); }

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);

	vector<iPod_pqed*> eqList;
};

// PQED: A single EQ Preset
class iPod_pqed
{
public:
	iPod_pqed();
	virtual ~iPod_pqed();

	unsigned long length;		// length of name string
	unsigned short * name;		// name string

/*
  10 band eq is not exactly what iTunes shows it to be.. It really is these:
  32Hz, 64Hz, 128Hz, 256Hz, 512Hz, 1024Hz, 2048Hz, 4096Hz, 8192Hz, 16384Hz

  Also note that although these are longs, The range is only -1200 to +1200. That's dB * 100.
*/

	signed long preamp;			// preamp setting

	signed long eq[10];			// iTunes shows 10 bands for EQ presets
	signed long short_eq[5];	// This is a 5 band version of the same thing (possibly what the iPod actually uses?)

	virtual long parse(unsigned char * data);
	virtual long write(unsigned char * data, const unsigned long datasize);
};


#endif
