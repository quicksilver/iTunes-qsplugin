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


// iPodDB.cpp: implementation of the iPod classes.
//
//////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4786)

#include "iPodDB.h"
#include <assert.h>
#include <time.h>
#include <windows.h>


//#define IPODDB_PROFILER		// Uncomment to enable profiler measurments

#ifdef IPODDB_PROFILER
	/* profiler code from Foobar2000's PFC library:
	 *
	 *  Copyright (c) 2001-2003, Peter Pawlowski
	 *  All rights reserved.
	 *
	 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
	 *
	 *	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	 *	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	 *	Neither the name of the author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
	 *
	 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
     */
	class profiler_static
	{
	private:
		const char * name;
		__int64 total_time,num_called;

	public:
		profiler_static(const char * p_name)
		{
			name = p_name;
			total_time = 0;
			num_called = 0;
		}
		~profiler_static()
		{
			char blah[512];
			char total_time_text[128];
			char num_text[128];
			_i64toa(total_time,total_time_text,10);
			_i64toa(num_called,num_text,10);
			_snprintf(blah, sizeof(blah), "profiler: %s - %s cycles (executed %s times)\n",name,total_time_text,num_text);
			OutputDebugStringA(blah);
		}
		void add_time(__int64 delta) {total_time+=delta;num_called++;}
	};

	class profiler_local
	{
	private:
		static __int64 get_timestamp();
		__int64 start;
		profiler_static * owner;
	public:
		profiler_local(profiler_static * p_owner)
		{
			owner = p_owner;
			start = get_timestamp();
		}
		~profiler_local()
		{
			__int64 end = get_timestamp();
			owner->add_time(end-start);
		}

	};

	__declspec(naked) __int64 profiler_local::get_timestamp()
	{
		__asm
		{
			rdtsc
			ret
		}
	}


	#define profiler(name) \
		static profiler_static profiler_static_##name(#name); \
		profiler_local profiler_local_##name(&profiler_static_##name);

#endif


#ifdef _DEBUG
  #define MYDEBUG_NEW   new( _NORMAL_BLOCK, __FILE__, __LINE__)
   // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
   //allocations to be of _CLIENT_BLOCK type

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new MYDEBUG_NEW
#endif


//
// useful functions
//////////////////////////////////////////////////////////////////////

// convert Macintosh timestamp to windows timestamp
time_t mactime_to_wintime (const unsigned long mactime)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__mactime_to_wintime);
#endif
    if (mactime != 0) return (time_t)(mactime - 2082844800);
    else return (time_t)mactime;
}

// convert windows timestamp to Macintosh timestamp
unsigned long wintime_to_mactime (const time_t time)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__wintime_to_mactime);
#endif

    return (unsigned long)(time + 2082844800);
}

// get 4 bytes from data, reversed
static __forceinline unsigned long rev4(const unsigned char * data)
{
	unsigned long ret;
	ret = ((unsigned long) data[3]) << 24;
	ret += ((unsigned long) data[2]) << 16;
	ret += ((unsigned long) data[1]) << 8;
	ret += ((unsigned long) data[0]);
	return ret;
}

// get 4 bytes from data
static __forceinline unsigned long get4(const unsigned char * data)
{
	unsigned long ret;
	ret = ((unsigned long) data[0]) << 24;
	ret += ((unsigned long) data[1]) << 16;
	ret += ((unsigned long) data[2]) << 8;
	ret += ((unsigned long) data[3]);
	return ret;
}

// get 8 bytes from data
static __forceinline unsigned __int64 get8(const unsigned char * data)
{
	unsigned __int64 ret;
	ret = get4(data);
	ret = ret << 32;
	ret += get4(&data[4]);
	return ret;
}

// reverse 8 bytes in place
static __forceinline unsigned __int64 rev8(unsigned __int64 number)
{
	unsigned __int64 ret;
	ret = (number&0x00000000000000FF) << 56;
	ret+= (number&0x000000000000FF00) << 40;
	ret+= (number&0x0000000000FF0000) << 24;
	ret+= (number&0x00000000FF000000) << 8;
	ret+= (number&0x000000FF00000000) >> 8;
	ret+= (number&0x0000FF0000000000) >> 24;
	ret+= (number&0x00FF000000000000) >> 40;
	ret+= (number&0xFF00000000000000) >> 56;
	return ret;
}

//write 4 bytes reversed
static __forceinline void rev4(const unsigned long number, unsigned char * data)
{
	data[3] = (unsigned char)(number >> 24) & 0xff;
	data[2] = (unsigned char)(number >> 16) & 0xff;
	data[1] = (unsigned char)(number >>  8) & 0xff;
	data[0] = (unsigned char)number & 0xff;
}

//write 4 bytes normal
static __forceinline void put4(const unsigned long number, unsigned char * data)
{
	data[0] = (unsigned char)(number >> 24) & 0xff;
	data[1] = (unsigned char)(number >> 16) & 0xff;
	data[2] = (unsigned char)(number >>  8) & 0xff;
	data[3] = (unsigned char)number & 0xff;
}

// write 8 bytes normal
static __forceinline void put8(const unsigned __int64 number, unsigned char * data)
{
	data[0] = (unsigned char)(number >> 56) & 0xff;
	data[1] = (unsigned char)(number >> 48) & 0xff;
	data[2] = (unsigned char)(number >> 40) & 0xff;
	data[3] = (unsigned char)(number >> 32) & 0xff;
	data[4] = (unsigned char)(number >> 24) & 0xff;
	data[5] = (unsigned char)(number >> 16) & 0xff;
	data[6] = (unsigned char)(number >> 8) & 0xff;
	data[7] = (unsigned char)number & 0xff;
}


// useful function to convert from UTF16 to chars
char * UTF16_to_char(unsigned short * str, int length)
{
	char * tempstr=(char *)malloc(length/2+1);
	int ret=WideCharToMultiByte( CP_MACCP, 0, str, length/2, tempstr, length/2, "x", NULL );
	tempstr[length/2]='\0';

	if (!ret) DWORD bob=GetLastError();

	return tempstr;
}


//////////////////////////////////////////////////////////////////////
// iPodObj - Base for all iPod classes
//////////////////////////////////////////////////////////////////////

iPodObj::iPodObj() :
	size_head(0),
	size_total(0)
{
}

iPodObj::~iPodObj()
{
}

//////////////////////////////////////////////////////////////////////
// iPod_mhbd - iTunes database class
//////////////////////////////////////////////////////////////////////

iPod_mhbd::iPod_mhbd() :
	unk1(1),
	unk2(9),
	children(2),
	unk3(0),
	unk4(0),
	unk5(2)
{
	mhsdsongs = new iPod_mhsd(1);
	mhsdplaylists = new iPod_mhsd(2);
}

iPod_mhbd::~iPod_mhbd()
{
	delete mhsdsongs;
	delete mhsdplaylists;
}

long iPod_mhbd::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhbd_parse);
#endif

	unsigned long ptr=0;

	//check mhbd header
	if (_strnicmp((char *)&data[ptr],"mhbd",4)) return -1;
	ptr+=4;

	// get sizes
	size_head=rev4(&data[ptr]);
	ptr+=4;
	size_total=rev4(&data[ptr]);
	ptr+=4;

	// get unk's and numchildren
	unk1=rev4(&data[ptr]);
	ptr+=4;
	unk2=rev4(&data[ptr]);
	ptr+=4;
	children=rev4(&data[ptr]);
	ptr+=4;
	unk3=rev4(&data[ptr]);
	ptr+=4;
	unk4=rev4(&data[ptr]);
	ptr+=4;
	unk5=rev4(&data[ptr]);
	ptr+=4;


	if (children != 2) return -1;

	//skip over nulls
	ptr=size_head;

	// get the mhsd's
	long ret;
	ret=mhsdsongs->parse(&data[ptr]);
	ASSERT(ret >= 0);
	if (ret<0) return ret;
	else ptr+=ret;

	ret=mhsdplaylists->parse(&data[ptr]);
	ASSERT(ret >= 0);
	if (ret<0) return ret;
	else ptr+=ret;

	if (ptr != size_total) return -1;

	return size_total;
}


long iPod_mhbd::write(unsigned char * data, const unsigned long datasize)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhbd_write);
#endif

	const unsigned int headsize=0x68;
	// check for header size
	if (headsize>datasize) return -1;

	long ptr=0;

	//write mhbd header
	data[0]='m';data[1]='h';data[2]='b';data[3]='d';
	ptr+=4;

	// write sizes
	rev4(headsize,&data[ptr]);	// header size
	ptr+=4;
	rev4(0x00,&data[ptr]);  // placeholder for total size (fill in later)
	ptr+=4;

	//write unks
	rev4(unk1,&data[ptr]);
	ptr+=4;
	rev4(unk2,&data[ptr]);
	ptr+=4;

	//write numchildren
	ASSERT (children = 2); // seen no other case in an iTunesDB yet
	rev4(children,&data[ptr]);
	ptr+=4;
	// more unks
	rev4(unk3,&data[ptr]);
	ptr+=4;
	rev4(unk4,&data[ptr]);
	ptr+=4;
	rev4(unk5,&data[ptr]);
	ptr+=4;

	// fill up the rest of the header with nulls
	for (unsigned int i=ptr;i<headsize;i++)
		data[i]=0;
	ptr=headsize;

	// write the mhsd's
	long ret;
	ret=mhsdsongs->write(&data[ptr], datasize-ptr);
	ASSERT(ret >= 0);
	if (ret<0) return ret;
	else ptr+=ret;

	ret=mhsdplaylists->write(&data[ptr], datasize-ptr);
	ASSERT(ret >= 0);
	if (ret<0) return ret;
	else ptr+=ret;

	// fix the total size
	rev4(ptr,&data[8]);

	return ptr;
}


//////////////////////////////////////////////////////////////////////
// iPod_mhsd - Holds tracklists and playlists
//////////////////////////////////////////////////////////////////////

iPod_mhsd::iPod_mhsd() :
	index(0),
	mhlt(NULL),
	mhlp(NULL)
{
}

iPod_mhsd::iPod_mhsd(int newindex) :
	index(newindex),
	mhlt(NULL),
	mhlp(NULL)
{
		switch(newindex)
		{
		case 1: mhlt=new iPod_mhlt(); break;
		case 2: mhlp=new iPod_mhlp(); break;
		default: index=0;
		}
}

iPod_mhsd::~iPod_mhsd()
{
	if (mhlt) delete mhlt;
	if (mhlp) delete mhlp;
}

long iPod_mhsd::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhsd_parse);
#endif

	unsigned long ptr=0;

	//check mhsd header
	if (_strnicmp((char *)&data[ptr],"mhsd",4)) return -1;
	ptr+=4;

	// get sizes
	size_head=rev4(&data[ptr]);
	ptr+=4;
	size_total=rev4(&data[ptr]);
	ptr+=4;

	// get index number
	index=rev4(&data[ptr]);
	ptr+=4;

	// skip null padding
	ptr=size_head;

	long ret;

	// check to see if this is a holder for an mhlt or an mhlp
	if (!_strnicmp((char *)&data[ptr],"mhlt",4))
	{
		if (mhlt==NULL)
		{
			mhlt=new iPod_mhlt();
			index=1;
		}
		ret=mhlt->parse(&data[ptr]);
		ASSERT(ret >= 0);
		if (ret<0) return ret;
		else ptr+=ret;
	}
	else if (!_strnicmp((char *)&data[ptr],"mhlp",4))
	{
		if (mhlp==NULL)
		{
			mhlp=new iPod_mhlp();
			index=2;
		}
		ret=mhlp->parse(&data[ptr]);
		ASSERT(ret >= 0);
		if (ret<0) return ret;
		else ptr+=ret;
	}
	else return -1;

	if (ptr != size_total) return -1;

	return size_total;
}

long iPod_mhsd::write(unsigned char * data, const unsigned long datasize)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhsd_write);
#endif

	const unsigned int headsize=0x60;
	// check for header size
	if (headsize>datasize) return -1;

	long ptr=0;

	//write mhsd header
	data[0]='m';data[1]='h';data[2]='s';data[3]='d';
	ptr+=4;

	// write sizes
	rev4(headsize,&data[ptr]);	// header size
	ptr+=4;
	rev4(0x00,&data[ptr]);  // placeholder for total size (fill in later)
	ptr+=4;

	// write index number
	rev4(index,&data[ptr]);
	ptr+=4;

	// fill up the rest of the header with nulls
	for (unsigned int i=ptr;i<headsize;i++)
		data[i]=0;
	ptr=headsize;

	// write out the songs or the playlists, depending on index
	long ret;
	if (index==1)	// mhlt
		ret=mhlt->write(&data[ptr],datasize-ptr);
	else if (index==2) // mhlp
		ret=mhlp->write(&data[ptr],datasize-ptr);
	else return -1;

	ASSERT(ret>=0);
	if (ret<0) return ret;
	else ptr+=ret;

	// fix the total size
	rev4(ptr,&data[8]);

	return ptr;
}



//////////////////////////////////////////////////////////////////////
// iPod_mhlt - TrackList class
//////////////////////////////////////////////////////////////////////

iPod_mhlt::iPod_mhlt() :
	mhit(),
	next_mhit_id(2000)
{
}

iPod_mhlt::~iPod_mhlt()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlt_destructor);
#endif

	// It is unnecessary (and slow) to clear the map, since the object is being destroyed anyway
	ClearTracks(false);
}

long iPod_mhlt::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlt_parse);
#endif

	long ptr=0;

	//check mhlt header
	if (_strnicmp((char *)&data[ptr],"mhlt",4)) return -1;
	ptr+=4;

	// get size
	size_head=rev4(&data[ptr]);
	ptr+=4;

	// get num children (num songs on iPod)
	const unsigned long children=rev4(&data[ptr]);		// Only used locally - child count is obtained from the mhit list
	ptr+=4;

	//skip nulls
	ptr=size_head;

	long ret;

	// get children one by one
	for (unsigned long i=0;i<children;i++)
	{
		iPod_mhit *m = new iPod_mhit;
		ret=m->parse(&data[ptr]);
		ASSERT(ret >= 0);
		if (ret<0)
		{
			delete m;
			return ret;
		}

		ptr+=ret;

		mhit.insert(mhit_value_t(m->id, m));
	}
	return ptr;
}

long iPod_mhlt::write(unsigned char * data, const unsigned long datasize)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlt_write);
#endif

	const unsigned int headsize=0x5c;
	// check for header size
	if (headsize>datasize) return -1;

	long ptr=0;

	//write mhlt header
	data[0]='m';data[1]='h';data[2]='l';data[3]='t';
	ptr+=4;

	// write size
	rev4(headsize,&data[ptr]);	// header size
	ptr+=4;

	// write numchildren (numsongs)
	const unsigned long children = GetChildrenCount();
	rev4(children,&data[ptr]);
	ptr+=4;

	// fill up the rest of the header with nulls
	for (unsigned long i=ptr;i<headsize;i++)
		data[i]=0;
	ptr=headsize;

	long ret;

	// write children one by one
	mhit_map_t::const_iterator begin = mhit.begin();
	mhit_map_t::const_iterator end   = mhit.end();
	for(mhit_map_t::const_iterator it = begin; it != end; it++)
	{
		iPod_mhit *m = static_cast<iPod_mhit*>((*it).second);

#ifdef _DEBUG
		const unsigned int mapID = (*it).first;
		ASSERT(mapID == m->id);
#endif

		ret=m->write(&data[ptr],datasize-ptr);
		ASSERT(ret >= 0);
		if (ret<0) return ret;
		else ptr+=ret;
	}

	return ptr;
}

iPod_mhit * iPod_mhlt::AddTrack(const unsigned int requestedID)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlt_AddTrack);
#endif

	unsigned int id = requestedID;
	if(id == 0)
	{
		// Find the next available ID
		mhit_map_t::iterator end = mhit.end();
		id = next_mhit_id;
		for(;;id++)
		{
			if(mhit.find(id) == end)		// Found an ID that isn't in the list
			{
				next_mhit_id = id + 1;
				break;
			}
		}
	}
	else
	{
		// Allow duplicate IDs, but warn if they exist
		ASSERT(mhit.find(id) == mhit.end());
	}

	iPod_mhit *track = new iPod_mhit;
	ASSERT(track != NULL);
	if (track != NULL)
	{
		track->id = id;
		mhit.insert(mhit_value_t(track->id, track));
	}

	return track;
}

bool iPod_mhlt::DeleteTrack(const unsigned long index)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlt_DeleteTrack);
#endif

	unsigned int i=0;
	for(mhit_map_t::const_iterator it = mhit.begin(); it != mhit.end(); it++, i++)
	{
		if(i == index)
		{
			iPod_mhit *m = static_cast<iPod_mhit*>((*it).second);
			return(DeleteTrackByID(m->id));
		}
	}
	return false;
}

bool iPod_mhlt::DeleteTrackByID(const unsigned long id)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlt_DeleteTrackByID);
#endif

	mhit_map_t::const_iterator it = mhit.find(id);
	if(it != mhit.end())
	{
		iPod_mhit *m = static_cast<iPod_mhit*>((*it).second);
		mhit.erase(id);
		delete m;
		return true;
	}
	return false;
}

iPod_mhit * iPod_mhlt::GetTrack(const unsigned long index)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlt_GetTrack);
#endif

	unsigned long i = 0;
	mhit_map_t::const_iterator begin = mhit.begin();
	mhit_map_t::const_iterator end   = mhit.end();
	for(mhit_map_t::const_iterator it = begin; it != end; it++, i++)
	{
		if(i == index)
			return static_cast<iPod_mhit*>((*it).second);
	}

	return NULL;
}

iPod_mhit * iPod_mhlt::GetTrackByID(const unsigned long id)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlt_GetTrackByID);
#endif

	mhit_map_t::const_iterator it = mhit.find(id);
	if(it == mhit.end())
		return NULL;

	return static_cast<iPod_mhit*>((*it).second);
}

bool iPod_mhlt::ClearTracks(const bool clearMap)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlt_ClearTracks);
#endif

	mhit_map_t::const_iterator begin = mhit.begin();
	mhit_map_t::const_iterator end   = mhit.end();
	for(mhit_map_t::const_iterator it = begin; it != end; it++)
	{
		delete static_cast<iPod_mhit*>((*it).second);
	}

	if(clearMap)
		mhit.clear();

	return true;
}


//////////////////////////////////////////////////////////////////////
// iPod_mhit - Holds info about a song
//////////////////////////////////////////////////////////////////////

iPod_mhit::iPod_mhit() :
	 id(0),
	 unk1(1),
	 unk2(0),
	 type(0),
	 compilation(0),
	 stars(0),
	 createdtime(0),
	 size(0),
	 length(0),
	 tracknum(0),
	 totaltracks(0),
	 year(0),
	 bitrate(0),
	 samplerate(0),
	 volume(0),
	 starttime(0),
	 stoptime(0),
	 soundcheck(0),
	 playcount(0),
	 unk4(0),
	 lastplayedtime(0),
	 cdnum(0),
	 totalcds(0),
	 unk5(0),
	 lastmodtime(0),
	 bookmarktime(0),
	 unk7(0),
	 unk8(0),
	 BPM(0),
	 app_rating(0),
	 checked(0),
	 unk9(0),
	 unk10(0),
	 unk11(0),
	 unk12(0),
	 unk13(0),
	 unk14(0),
	 unk15(0),
	 unk16(0),
	 mhod()
{
	mhod.reserve(8);
}

iPod_mhit::~iPod_mhit()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhit_destructor);
#endif

	const unsigned long count = GetChildrenCount();
	for (unsigned long i=0;i<count;i++)
		delete mhod[i];
}

long iPod_mhit::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhit_parse);
#endif

	long ptr=0;
	unsigned long temp=0;

	//check mhit header
	if (_strnicmp((char *)&data[ptr],"mhit",4)) return -1;
	ptr+=4;

	// get sizes
	size_head=rev4(&data[ptr]);
	ptr+=4;
	size_total=rev4(&data[ptr]);
	ptr+=4;

	// get rest of data
	unsigned long mhodnum=rev4(&data[ptr]);	// Only used locally
	ptr+=4;
	id=rev4(&data[ptr]);
	ptr+=4;
	unk1=rev4(&data[ptr]);
	ptr+=4;
	unk2=rev4(&data[ptr]);
	ptr+=4;

	temp=rev4(&data[ptr]);
	ptr+=4;
	stars = temp >> 24;			// stars = rating * 20 (divide stars by 20 to get star rating)
	compilation = (temp & 0xff0000) >> 16;
	type = temp & 0x0000ffff;

	createdtime=rev4(&data[ptr]);
	ptr+=4;
	size=rev4(&data[ptr]);
	ptr+=4;
	length=rev4(&data[ptr]);
	ptr+=4;
	tracknum=rev4(&data[ptr]);
	ptr+=4;
	totaltracks=rev4(&data[ptr]);
	ptr+=4;
	year=rev4(&data[ptr]);
	ptr+=4;
	bitrate=rev4(&data[ptr]);
	ptr+=4;

	samplerate=rev4(&data[ptr]) >> 16;
	ptr+=4;

	volume=rev4(&data[ptr]);
	ptr+=4;
	starttime=rev4(&data[ptr]);
	ptr+=4;
	stoptime=rev4(&data[ptr]);
	ptr+=4;
	soundcheck=rev4(&data[ptr]);
	ptr+=4;
	playcount=rev4(&data[ptr]);
	ptr+=4;
	unk4=rev4(&data[ptr]);
	ptr+=4;
	lastplayedtime=rev4(&data[ptr]);
	ptr+=4;
	cdnum=rev4(&data[ptr]);
	ptr+=4;
	totalcds=rev4(&data[ptr]);
	ptr+=4;
	unk5=rev4(&data[ptr]);
	ptr+=4;
	lastmodtime=rev4(&data[ptr]);
	ptr+=4;
	bookmarktime=rev4(&data[ptr]);
	ptr+=4;
	unk7=rev4(&data[ptr]);
	ptr+=4;
	unk8=rev4(&data[ptr]);
	ptr+=4;

	temp=rev4(&data[ptr]);
	BPM=temp>>16;
	app_rating=(temp&0xff00) >> 8;
	checked = temp&0xff;

	ptr+=4;
	unk9=rev4(&data[ptr]);
	ptr+=4;
	unk10=rev4(&data[ptr]);
	ptr+=4;
	unk11=rev4(&data[ptr]);
	ptr+=4;
	unk12=rev4(&data[ptr]);
	ptr+=4;
	unk13=rev4(&data[ptr]);
	ptr+=4;
	unk14=rev4(&data[ptr]);
	ptr+=4;
	unk15=rev4(&data[ptr]);
	ptr+=4;
	unk16=rev4(&data[ptr]);

	// skip nulls
	ptr=size_head;

	long ret;
	for (unsigned long i=0;i<mhodnum;i++)
	{
		iPod_mhod *m = new iPod_mhod;
		ret=m->parse(&data[ptr]);
		ASSERT(ret >= 0);
		if (ret<0)
		{
			delete m;
			return ret;
		}

		ptr+=ret;
		mhod.push_back(m);
	}

	return size_total;
}


long iPod_mhit::write(unsigned char * data, const unsigned long datasize)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhit_write);
#endif

	const unsigned int headsize=0x9c;
	// check for header size
	if (headsize>datasize) return -1;

	long ptr=0;

	//write mhlt header
	data[0]='m';data[1]='h';data[2]='i';data[3]='t';
	ptr+=4;

	// write sizes
	rev4(headsize,&data[ptr]);	// header size
	ptr+=4;
	rev4(0x00,&data[ptr]);  // placeholder for total size (fill in later)
	ptr+=4;

	unsigned long temp;
	// write stuff out
	unsigned long mhodnum = GetChildrenCount();
	rev4(mhodnum,&data[ptr]);
	ptr+=4;
	rev4(id,&data[ptr]);
	ptr+=4;
	rev4(unk1,&data[ptr]);
	ptr+=4;
	rev4(unk2,&data[ptr]);
	ptr+=4;

	temp = stars << 24 | compilation << 16 | type & 0x0000ffff;
	rev4(temp,&data[ptr]);
	ptr+=4;

	rev4(createdtime,&data[ptr]);
	ptr+=4;
	rev4(size,&data[ptr]);
	ptr+=4;
	rev4(length,&data[ptr]);
	ptr+=4;
	rev4(tracknum,&data[ptr]);
	ptr+=4;
	rev4(totaltracks,&data[ptr]);
	ptr+=4;
	rev4(year,&data[ptr]);
	ptr+=4;
	rev4(bitrate,&data[ptr]);
	ptr+=4;

	temp=samplerate<<16;
	rev4(temp,&data[ptr]);
	ptr+=4;
	rev4(volume,&data[ptr]);
	ptr+=4;
	rev4(starttime,&data[ptr]);
	ptr+=4;
	rev4(stoptime,&data[ptr]);
	ptr+=4;
	rev4(soundcheck,&data[ptr]);
	ptr+=4;
	rev4(playcount,&data[ptr]);
	ptr+=4;
	rev4(unk4,&data[ptr]);
	ptr+=4;
	rev4(lastplayedtime,&data[ptr]);
	ptr+=4;
	rev4(cdnum,&data[ptr]);
	ptr+=4;
	rev4(totalcds,&data[ptr]);
	ptr+=4;
	rev4(unk5,&data[ptr]);
	ptr+=4;
	rev4(lastmodtime,&data[ptr]);
	ptr+=4;
	rev4(bookmarktime,&data[ptr]);
	ptr+=4;
	rev4(unk7,&data[ptr]);
	ptr+=4;
	rev4(unk8,&data[ptr]);
	ptr+=4;

	temp = BPM << 16 | (app_rating & 0xff) << 8 | (checked & 0xff);
	rev4(temp, &data[ptr]);
	ptr+=4;

	rev4(unk9,&data[ptr]);
	ptr+=4;
	rev4(unk10,&data[ptr]);
	ptr+=4;
	rev4(unk11,&data[ptr]);
	ptr+=4;
	rev4(unk12,&data[ptr]);
	ptr+=4;
	rev4(unk13,&data[ptr]);
	ptr+=4;
	rev4(unk14,&data[ptr]);
	ptr+=4;
	rev4(unk15,&data[ptr]);
	ptr+=4;
	rev4(unk16,&data[ptr]);
	ptr+=4;

	ASSERT(ptr==headsize); // if this ain't true, I screwed up badly somewhere above

	long ret;
	for (unsigned long i=0;i<mhodnum;i++)
	{
		ret=mhod[i]->write(&data[ptr],datasize-ptr);
		ASSERT(ret >= 0);
		if (ret<0) return ret;
		else ptr+=ret;
	}

	// fix the total size
	rev4(ptr,&data[8]);

	return ptr;
}

iPod_mhod * iPod_mhit::AddString(const int type)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhit_AddString);
#endif

	iPod_mhod * m;
	if (type)
	{
		m = FindString(type);
		if (m != NULL)
		{
			return m;
		}
	}

	m=new iPod_mhod;
	if (m!=NULL && type) m->type=type;
	mhod.push_back(m);
	return m;
}

iPod_mhod * iPod_mhit::FindString(const unsigned long type)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhit_FindString);
#endif

	const unsigned long children = GetChildrenCount();
	for (unsigned long i=0;i<children;i++)
	{
		if (mhod[i]->type == type) return mhod[i];
	}

	return NULL;
}

unsigned long iPod_mhit::DeleteString(const unsigned long type)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhit_DeleteString);
#endif

	unsigned long count=0;
	const unsigned long children = GetChildrenCount();
	for (unsigned long i=0;i<children;i++)
	{
		if (mhod[i]->type == type)
		{
			iPod_mhod * m = mhod.at(i);
			mhod.erase(mhod.begin()+i);
			delete m;
			i = i > 0 ? i - 1 : 0;  // do this to ensure that it checks the new entry in position i next
			count++;
		}
	}
	return count;
}

iPod_mhit& iPod_mhit::operator=(const iPod_mhit& src)
{
  Duplicate(&src,this);
  return *this;
}

void iPod_mhit::Duplicate(const iPod_mhit *src, iPod_mhit *dst)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhit_Duplicate);
#endif

	if(src == NULL || dst == NULL)
		return;

	dst->id = src->id;
	dst->unk1 = src->unk1;
	dst->unk2 = src->unk2;
	dst->type = src->type;
	dst->compilation = src->compilation;
	dst->stars = src->stars;
	dst->createdtime = src->createdtime;
	dst->size = src->size;
	dst->length = src->length;
	dst->tracknum = src->tracknum;
	dst->totaltracks = src->totaltracks;
	dst->year = src->year;
	dst->bitrate = src->bitrate;
	dst->samplerate = src->samplerate;
	dst->volume = src->volume;
	dst->starttime = src->starttime;
	dst->stoptime = src->stoptime;
	dst->soundcheck = src->soundcheck;
	dst->playcount = src->playcount;
	dst->unk4 = src->unk4;
	dst->lastplayedtime = src->lastplayedtime;
	dst->cdnum = src->cdnum;
	dst->totalcds = src->totalcds;
	dst->unk5 = src->unk5;
	dst->lastmodtime = src->lastmodtime;
	dst->bookmarktime = src->bookmarktime;
	dst->unk7 = src->unk7;
	dst->unk8 = src->unk8;
	dst->BPM = src->BPM;
	dst->app_rating = src->app_rating;
	dst->checked = src->checked;
	dst->unk9 = src->unk9;
	dst->unk10 = src->unk10;
	dst->unk11 = src->unk11;
	dst->unk12 = src->unk12;
	dst->unk13 = src->unk13;
	dst->unk14 = src->unk14;
	dst->unk15 = src->unk15;
	dst->unk16 = src->unk16;

	const unsigned int mhodSize = src->mhod.size();
	for(unsigned int i=0; i<mhodSize; i++)
	{
		iPod_mhod *src_mhod = src->mhod[i];
		if(src_mhod == NULL)
			continue;

		iPod_mhod *dst_mhod = dst->AddString(src_mhod->type);
		if(dst_mhod)
			dst_mhod->Duplicate(src_mhod, dst_mhod);
	}
}

//////////////////////////////////////////////////////////////////////
// iPod_mhod - Holds strings for a song or playlist, among other things
//////////////////////////////////////////////////////////////////////

iPod_mhod::iPod_mhod() :
	type(0),
	unk1(0),
	unk2(0),
	position(1),
	length(0),
	unk3(0),
	unk4(0),
	str(NULL),
	binary(NULL),
	liveupdate(1),
	checkrules(0),
	matchcheckedonly(0),
	limitsort_opposite(0),
	limittype(0),
	limitsort(0),
	limitvalue(0),
	unk5(0),
	rules_operator(SPLMATCH_AND),
	parseSmartPlaylists(true)
{
}


iPod_mhod::~iPod_mhod()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhod_destructor);
#endif

	if (str) delete [] str;
	if (binary) delete [] binary;

	const unsigned int size = rule.size();
	for (unsigned int i=0;i<size;i++)
		delete rule[i];
}

long iPod_mhod::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhod_parse);
#endif

   	long ptr=0;
	unsigned long i = 0;

	//check mhod header
	if (_strnicmp((char *)&data[ptr],"mhod",4)) return -1;
	ptr+=4;

	// get sizes
	size_head=rev4(&data[ptr]); ptr+=4;
	size_total=rev4(&data[ptr]); ptr+=4;

	// get type
	type=rev4(&data[ptr]); ptr+=4;

	// dunno what these are, but they are definitely common among all the types of mhods
	// smartlist types prove that.
	// they're usually zero though.. null padding for the MHOD header?
	unk1=rev4(&data[ptr]); ptr+=4;
	unk2=rev4(&data[ptr]); ptr+=4;

	if (type<50)
	{
		// string types get parsed
		position=rev4(&data[ptr]); ptr+=4;
		length=rev4(&data[ptr]); ptr+=4;
		unk3=rev4(&data[ptr]); ptr+=4;
		unk4=rev4(&data[ptr]); ptr+=4;

		str=new unsigned short[length + 1];
		memcpy(str,&data[ptr],length);
		str[length / 2] = '\0';
		ptr+=length;
	}
	else if (type==MHOD_SPLPREF)
	{
		if(parseSmartPlaylists)
		{
			liveupdate=data[ptr]; ptr++;
			checkrules=data[ptr]; ptr++;
			checklimits=data[ptr]; ptr++;
			limittype=data[ptr]; ptr++;
			limitsort=data[ptr]; ptr++;
			ptr+=3;
			limitvalue=rev4(&data[ptr]); ptr+=4;
			matchcheckedonly=data[ptr]; ptr++;

			// if the opposite flag is on, set limitsort's high bit
			limitsort_opposite=data[ptr]; ptr++;
			if(limitsort_opposite)
				limitsort += 0x80000000;
		}
	}
	else if (type==MHOD_SPLDATA)
	{
		if(parseSmartPlaylists)
		{
			// strangely, SPL Data is the only thing in the file that *isn't* byte reversed.
			// check for SLst header
			if (_strnicmp((char *)&data[ptr],"SLst",4)) return -1;
			ptr+=4;
			unk5=get4(&data[ptr]); ptr+=4;
			const int numrules=get4(&data[ptr]); ptr+=4;
			rules_operator=get4(&data[ptr]); ptr+=4;
			ptr+=120;

			rule.reserve(numrules);

			for (i=0;i<numrules;i++)
			{
				SPLRule * r = new SPLRule;
				ASSERT(r);
				if(r == NULL)
					continue;

				r->field=get4(&data[ptr]); ptr+=4;
				r->action=get4(&data[ptr]); ptr+=4;
				ptr+=44;
				r->length=get4(&data[ptr]); ptr+=4;

	#ifdef _DEBUG
				switch(r->action)
				{
					case SPLACTION_IS_INT:
					case SPLACTION_IS_GREATER_THAN:
					case SPLACTION_IS_NOT_GREATER_THAN:
					case SPLACTION_IS_LESS_THAN:
					case SPLACTION_IS_NOT_LESS_THAN:
					case SPLACTION_IS_IN_THE_RANGE:
					case SPLACTION_IS_NOT_IN_THE_RANGE:
					case SPLACTION_IS_IN_THE_LAST:
					case SPLACTION_IS_STRING:
					case SPLACTION_CONTAINS:
					case SPLACTION_STARTS_WITH:
					case SPLACTION_DOES_NOT_START_WITH:
					case SPLACTION_ENDS_WITH:
					case SPLACTION_DOES_NOT_END_WITH:
					case SPLACTION_IS_NOT_INT:
					case SPLACTION_IS_NOT_IN_THE_LAST:
					case SPLACTION_IS_NOT:
					case SPLACTION_DOES_NOT_CONTAIN:
						break;

					default:
						// New action!
						//printf("New Action Discovered = %x\n",r->action);
						ASSERT(0);
						break;
				}
	#endif

				const bool hasString = iPod_slst::GetFieldType(r->field) == iPod_slst::ftString;

				if(hasString)
				{
					// For some unknown reason, smart playlist strings have UTF-16 characters that are byte swapped
					unsigned char *c = (unsigned char*)r->string;
					const unsigned len = min(r->length, SPL_MAXSTRINGLENGTH);
					for(unsigned int i=0; i<len; i+=2)
					{
						*(c + i) = data[ptr + i + 1];
						*(c + i + 1) = data[ptr + i];
					}

					ptr += r->length;
				}
				else
				{
					// from/to combos always seem to be 0x44 in length in all cases...
					// fix this to be smarter if it turns out not to be the case
					ASSERT(r->length == 0x44);

					r->fromvalue=get8(&data[ptr]); ptr+=8;
					r->fromdate=get8(&data[ptr]); ptr+=8;
					r->fromunits=get8(&data[ptr]); ptr+=8;

					r->tovalue=get8(&data[ptr]); ptr+=8;
					r->todate=get8(&data[ptr]); ptr+=8;
					r->tounits=get8(&data[ptr]); ptr+=8;

					// SPLFIELD_PLAYLIST seems to use the unks here...
					r->unk1=get4(&data[ptr]); ptr+=4;
					r->unk2=get4(&data[ptr]); ptr+=4;
					r->unk3=get4(&data[ptr]); ptr+=4;
					r->unk4=get4(&data[ptr]); ptr+=4;
					r->unk5=get4(&data[ptr]); ptr+=4;
				}

				rule.push_back(r);
			}
		}
	}
	else
	{
		// non string/smart playlist types get copied in.. with the header and such being ignored
		binary=new unsigned char[size_total-size_head];
		memcpy(binary,&data[ptr],size_total-size_head);
		// in this case, we'll use the length field to store the length of the binary stuffs,
		//		since it's not being used for anything else in these entries.
		// this helps in the writing phase of the process.
		// note that if, for some reason, you decide to create a mhod for type 50+ from scratch,
		//		you need to set the length = the size of your binary space
		length=size_total-size_head;
	}

	return size_total;
}

long iPod_mhod::write(unsigned char * data, const unsigned long datasize)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhod_write);
#endif

	const unsigned long headsize=0x18;
	// check for header size
	if (headsize>datasize) return -1;

	long ptr=0;

	//write mhod header
	data[0]='m';data[1]='h';data[2]='o';data[3]='d';
	ptr+=4;

	// write sizes
	rev4(headsize,&data[ptr]);	// header size
	ptr+=4;
	rev4(0x00,&data[ptr]);  // placeholder for total size (fill in later)
	ptr+=4;

	// write stuff out
	rev4(type,&data[ptr]);
	ptr+=4;
	rev4(unk1,&data[ptr]);
	ptr+=4;
	rev4(unk2,&data[ptr]);
	ptr+=4;

	if (type<50)
	{
		// check for string size
		if (16+length+headsize>datasize) return -1;

		rev4(position,&data[ptr]);
		ptr+=4;
		rev4(length,&data[ptr]);
		ptr+=4;
		rev4(unk3,&data[ptr]);
		ptr+=4;
		rev4(unk4,&data[ptr]);
		ptr+=4;

		const unsigned int len = length / 2;
		for (unsigned int i=0;i<len;i++)
		{
			data[ptr]=str[i] & 0xff;
			ptr++;
			data[ptr]=(str[i] >> 8) & 0xff;
			ptr++;
		}
	}
	else if (type==MHOD_SPLPREF)
	{
		// write the type 50 mhod
		data[ptr]=liveupdate; ptr++;
		data[ptr]=checkrules; ptr++;
		data[ptr]=checklimits; ptr++;
		data[ptr]=limittype; ptr++;
		data[ptr]=(limitsort & 0x000000ff); ptr++;
		data[ptr]=0; ptr++;
		data[ptr]=0; ptr++;
		data[ptr]=0; ptr++;
		rev4(limitvalue,&data[ptr]); ptr+=4;
		data[ptr]=matchcheckedonly; ptr++;
		// set the limitsort_opposite flag by checking the high bit of limitsort
		data[ptr] = limitsort & 0x80000000 ? 1 : 0;	 ptr++;

		// insert 58 nulls
		memset(data + ptr, 0, 58);	ptr += 58;
	}
	else if (type==MHOD_SPLDATA)
	{
		int i;
		const unsigned int ruleCount = rule.size();

		// put "SLst" header
		data[ptr]='S';data[ptr+1]='L';data[ptr+2]='s';data[ptr+3]='t';
		ptr+=4;
		put4(unk5,&data[ptr]); ptr+=4;
		put4(ruleCount,&data[ptr]); ptr+=4;
		put4(rules_operator,&data[ptr]); ptr+=4;
		memset(data + ptr, 0, 120); ptr+=120;

		for (i=0;i<ruleCount;i++)
		{
			SPLRule *r = rule[i];
			ASSERT(r);
			if(r == NULL)
				continue;

			put4(r->field,&data[ptr]); ptr+=4;
			put4(r->action,&data[ptr]); ptr+=4;
			memset(data + ptr, 0, 44); ptr+=44;
			put4(r->length,&data[ptr]); ptr+=4;

			const bool hasString = iPod_slst::GetFieldType(r->field) == iPod_slst::ftString;
			if(hasString)
			{
				// Byte swap the characters
				unsigned char *c = (unsigned char*)r->string;
				for(unsigned int i=0; i<r->length; i+=2)
				{
					data[ptr + i] = *(c + i + 1);
					data[ptr + i + 1] = *(c + i);
				}

				ptr += r->length;
			}
			else
			{
				put8(r->fromvalue,&data[ptr]); ptr+=8;
				put8(r->fromdate,&data[ptr]); ptr+=8;
				put8(r->fromunits,&data[ptr]); ptr+=8;
				put8(r->tovalue,&data[ptr]); ptr+=8;
				put8(r->todate,&data[ptr]); ptr+=8;
				put8(r->tounits,&data[ptr]); ptr+=8;

				put4(r->unk1,&data[ptr]); ptr+=4;
				put4(r->unk2,&data[ptr]); ptr+=4;
				put4(r->unk3,&data[ptr]); ptr+=4;
				put4(r->unk4,&data[ptr]); ptr+=4;
				put4(r->unk5,&data[ptr]); ptr+=4;

			}
		} // end for
	}
	else	// not a known type, use the binary
	{
		// check for binary size
		if (length+headsize>datasize) return -1;
		for (unsigned int i=0;i<length;i++)
		{
			data[ptr]=binary[i];
			ptr++;
		}
	}

	// fix the total size
	rev4(ptr,&data[8]);

	return ptr;
}

void iPod_mhod::SetString(const unsigned short *string)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhod_SetString);
#endif

	if(!string)
		return;

	delete [] str;
	length = 0;

	const unsigned int stringLen = wcslen(string);
	str = new unsigned short[stringLen + 1];
	if(str)
	{
		wcscpy(str, string);
		length = stringLen * 2;
	}
}

void iPod_mhod::Duplicate(iPod_mhod *src, iPod_mhod *dst)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhod_Duplicate);
#endif

	if(src == NULL || dst == NULL)
		return;

	dst->type = src->type;
	dst->unk1 = src->unk1;
	dst->unk2 = src->unk2;
	dst->position = src->position;
	dst->length = src->length;
	dst->unk3 = src->unk3;
	dst->unk4 = src->unk4;
	dst->liveupdate = src->liveupdate;
	dst->checkrules = src->checkrules;
	dst->checklimits = src->checklimits;
	dst->limittype = src->limittype;
	dst->limitsort = src->limitsort;
	dst->limitvalue = src->limitvalue;
	dst->matchcheckedonly = src->matchcheckedonly;
	dst->limitsort_opposite = src->limitsort_opposite;
	dst->unk5 = src->unk5;
	dst->rules_operator = src->rules_operator;

	if(src->str)
	{
		dst->SetString(src->str);
	}
	else if(src->binary)
	{
		dst->binary = new unsigned char[src->length];
		if(dst->binary)
			memcpy(dst->binary, src->binary, src->length);
	}


	const unsigned int ruleLen = src->rule.size();
	for(unsigned int i=0; i<ruleLen; i++)
	{
		SPLRule *srcRule = src->rule[i];
		if(srcRule)
		{
			SPLRule *dstRule = new SPLRule;
			if(dstRule)
				memcpy(dstRule, srcRule, sizeof(SPLRule));

			dst->rule.push_back(dstRule);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// iPod_mhlp - Holds playlists
//////////////////////////////////////////////////////////////////////

iPod_mhlp::iPod_mhlp() :
	mhyp(),
	beingDeleted(false)
{
	// Always start off with an empty, hidden, default playlist
	ASSERT(GetChildrenCount() == 0);
	GetDefaultPlaylist();
}

iPod_mhlp::~iPod_mhlp()
{
	// This is unnecessary (and slow) to clear the vector list,
	// since the object is being destroyed anyway...
	beingDeleted = true;
	ClearPlaylists();
}

long iPod_mhlp::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlp_parse);
#endif

	long ptr=0;

	//check mhlp header
	if (_strnicmp((char *)&data[ptr],"mhlp",4)) return -1;
	ptr+=4;

	// get sizes
	size_head=rev4(&data[ptr]);
	ptr+=4;
	const unsigned long children=rev4(&data[ptr]);		// Only used locally - child count is obtained from the mhyp vector list
	ptr+=4;

	// skip nulls
	ptr=size_head;

	mhyp.reserve(children); // pre allocate the space, for speed

	ClearPlaylists();

	long ret;
	for (unsigned long i=0;i<children; i++)
	{
		iPod_mhyp *m = new iPod_mhyp;
		ret=m->parse(&data[ptr]);
		ASSERT(ret >= 0);
		if (ret<0)
		{
			delete m;
			return ret;
		}

		// If this is really a smart playlist, we need to parse it again as a smart playlist
		if(m->FindString(MHOD_SPLPREF) != NULL)
		{
			delete m;

			m = new iPod_slst;
			ASSERT(m);
			ret = m->parse(&data[ptr]);
			ASSERT(ret >= 0);
			if(ret < 0)
			{
				delete m;
				return ret;
			}

		}

		ptr+=ret;
		mhyp.push_back(m);
	}

	return ptr;
}

long iPod_mhlp::write(unsigned char * data, const unsigned long datasize)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlp_write);
#endif

	const unsigned int headsize=0x5c;
	// check for header size
	if (headsize>datasize) return -1;

	long ptr=0;

	//write mhlp header
	data[0]='m';data[1]='h';data[2]='l';data[3]='p';
	ptr+=4;

	// write sizes
	rev4(headsize,&data[ptr]);	// header size
	ptr+=4;

	// write num of children
	const unsigned long children = GetChildrenCount();
	rev4(children,&data[ptr]);
	ptr+=4;

	// fill up the rest of the header with nulls
	for (unsigned int i=ptr;i<headsize;i++)
		data[i]=0;
	ptr=headsize;

	long ret;
	for (i=0;i<children; i++)
	{
		ret=mhyp[i]->write(&data[ptr],datasize-ptr);
		ASSERT(ret >= 0);
		if (ret<0) return ret;
		ptr+=ret;
	}

	return ptr;
}

iPod_mhyp * iPod_mhlp::AddPlaylist()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlp_AddPlaylist);
#endif

	iPod_mhyp * m = new iPod_mhyp;
	ASSERT(m);
	if (m != NULL)
	{
		mhyp.push_back(m);
		return m;
	}
	else return NULL;
}


iPod_slst* iPod_mhlp::AddSmartPlaylist()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlp_AddSmartPlaylist);
#endif

	iPod_slst *s = new iPod_slst;
	ASSERT(s);
	if (s != NULL)
	{
		mhyp.push_back(s);
		return s;
	}
	else return NULL;
}

iPod_mhyp * iPod_mhlp::FindPlaylist(const unsigned __int64 playlistID)
{
	const unsigned long count = GetChildrenCount();
	for (unsigned long i=0; i<count; i++)
	{
		iPod_mhyp * m = GetPlaylist(i);
		if (m->playlistID == playlistID)
			return m;
	}
	return NULL;
}


// deletes the playlist at a position
bool iPod_mhlp::DeletePlaylist(const unsigned long pos)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlp_DeletePlaylist);
#endif

	if (GetChildrenCount() >= pos)
	{
		iPod_mhyp * m = GetPlaylist(pos);
		mhyp.erase(mhyp.begin()+pos);
		delete m;
		return true;
	}
	else return false;
}

iPod_mhyp * iPod_mhlp::GetDefaultPlaylist()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlp_GetDefaultPlaylist);
#endif

	if (!mhyp.empty())
		return GetPlaylist(0);
	else
	{
		// Create a new hidden playlist, and set a default title
		iPod_mhyp * playlist = AddPlaylist();
		ASSERT(playlist);
		if(playlist)
		{
			playlist->hidden = 1;

			iPod_mhod *mhod = playlist->AddString(MHOD_TITLE);
			if(mhod)
				mhod->SetString(L"iPod");
		}

		return playlist;
	}
}

bool iPod_mhlp::ClearPlaylists(const bool createDefaultPlaylist)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhlp_ClearPlaylists);
#endif

	const unsigned long count = GetChildrenCount();
	for (unsigned long i=0;i<count;i++)
	{
		iPod_mhyp *m=GetPlaylist(i);
		delete m;
	}

	if(!beingDeleted)
		mhyp.clear();

	// create the default playlist again, if it's gone
	// XXX - Normally this is the right thing to do, but if we
	// are about to parse the playlists from the iPod, we can't create
	// the default playlist.  Doing so will create two default/hidden
	// playlists - this one and the one that will be created when
	// the playlists are parsed!
	if(createDefaultPlaylist)
		GetDefaultPlaylist();

	return true;
}


//////////////////////////////////////////////////////////////////////
// iPod_mhyp - A Playlist
//////////////////////////////////////////////////////////////////////

iPod_mhyp::iPod_mhyp() :
	hidden(0),
	timestamp(0),
	unk3(0),
	unk4(0),
	unk5(0),
	mhod(),
	mhip(),
	isSmartPlaylist(false),
	isPopulated(true)			// consider normal playlists to be populated always
{
	// Create a highly randomized 64 bit value for the playlistID
	playlistID = GeneratePlaylistID();
}

iPod_mhyp::~iPod_mhyp()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_destructor);
#endif

	const unsigned long mhodCount = GetMhodChildrenCount();
	const unsigned long mhipCount = GetMhipChildrenCount();
	unsigned long i;
	for (i=0;i<mhodCount;i++)
		delete mhod[i];
	for (i=0;i<mhipCount;i++)
		delete mhip[i];
}

long iPod_mhyp::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_parse);
#endif

	long ptr=0;

	//check mhyp header
	if (_strnicmp((char *)&data[ptr],"mhyp",4)) return -1;
	ptr+=4;

	// get sizes
	size_head=rev4(&data[ptr]);
	ptr+=4;
	size_total=rev4(&data[ptr]);
	ptr+=4;

	const unsigned long mhodnum=rev4(&data[ptr]);	 // Only used locally
	ptr+=4;
	const unsigned long numsongs=rev4(&data[ptr]);	// Only used locally
	ptr+=4;
	hidden=rev4(&data[ptr]);
	ptr+=4;
	timestamp=rev4(&data[ptr]);
	ptr+=4;
	playlistID = rev8(get8(&data[ptr]));
	if(playlistID == 0)
	{
		// Force the playlistID to be a valid value.
		// This may not always be the right thing to do, but I can't think of any reason why it wouldn't be ok...
		playlistID = GeneratePlaylistID();
	}
	ptr+=8;
	unk3=rev4(&data[ptr]);
	ptr+=4;
	unk4=rev4(&data[ptr]);
	ptr+=4;
	unk5=rev4(&data[ptr]);
	ptr+=4;

	ptr=size_head;

	long ret;
	unsigned long i;

	mhod.reserve(mhodnum);	// pre allocate the space, for speed
	mhip.reserve(numsongs);

	for (i=0;i<mhodnum;i++)
	{
		iPod_mhod *m = new iPod_mhod;

		// parseSmartPlaylists is an optimization for when dealing with smart playlists.
		// Since the playlist has to be parsed in order to determine if it is a smart playlist,
		// and if it is, parsed again as a smart playlist, this optimization prevents some duplicate parsing.
		m->parseSmartPlaylists = isSmartPlaylist;
		ret=m->parse(&data[ptr]);
		ASSERT(ret >= 0);
		if (ret<0)
		{
			delete m;
			return ret;
		}

		ptr+=ret;
		mhod.push_back(m);

	}

	for (i=0;i<numsongs;i++)
	{
		iPod_mhip *m = new iPod_mhip;
		ret=m->parse(&data[ptr]);
		ASSERT(ret >= 0);
		if (ret<0)
		{
			delete m;
			return ret;
		}

		else ptr+=ret;
		mhip.push_back(m);
	}

	return ptr;
}

long iPod_mhyp::write(unsigned char * data, const unsigned long datasize)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_write);
#endif

	const unsigned int headsize=0x6c;
	// check for header size
	ASSERT(headsize <= datasize);
	if (headsize>datasize) return -1;

	long ptr=0;

	//write mhyp header
	data[0]='m';data[1]='h';data[2]='y';data[3]='p';
	ptr+=4;

	// write sizes
	rev4(headsize,&data[ptr]);	// header size
	ptr+=4;
	rev4(0x00,&data[ptr]);  // placeholder for total size (fill in later)
	ptr+=4;

	// fill in stuff
	const unsigned long mhodnum = GetMhodChildrenCount();
	rev4(mhodnum,&data[ptr]);
	ptr+=4;
	const unsigned long numsongs = GetMhipChildrenCount();
	rev4(numsongs,&data[ptr]);
	ptr+=4;
	rev4(hidden,&data[ptr]);
	ptr+=4;
	rev4(timestamp,&data[ptr]);
	ptr+=4;
	put8(rev8(playlistID),&data[ptr]);
	ptr+=8;
	rev4(unk3,&data[ptr]);
	ptr+=4;
	rev4(unk4,&data[ptr]);
	ptr+=4;
	rev4(unk5,&data[ptr]);
	ptr+=4;

	unsigned long i;
	// fill up the rest of the header with nulls
	for (i=ptr;i<headsize;i++)
		data[i]=0;
	ptr=headsize;

	long ret;
	for (i=0;i<mhodnum; i++)
	{
		ret=mhod[i]->write(&data[ptr],datasize-ptr);
		ASSERT(ret >= 0);
		if (ret<0) return ret;
		else ptr+=ret;
	}

	for (i=0;i<numsongs;i++)
	{
		ret=mhip[i]->write(&data[ptr],datasize-ptr);
		ASSERT(ret >= 0);
		if (ret<0) return ret;
		else ptr+=ret;

		// create an faked up mhod type 100 for position info
		//   (required at this point, albeit seemingly useless.. type 100 mhods are weird)
		data[ptr]='m';data[ptr+1]='h';data[ptr+2]='o';data[ptr+3]='d';
		ptr+=4;
		rev4(0x18,&data[ptr]);	// header size
		ptr+=4;
		rev4(0x2c,&data[ptr]);  // total size
		ptr+=4;
		rev4(100,&data[ptr]);  // type
		ptr+=4;
		rev4(0,&data[ptr]);  // two nulls
		ptr+=4;
		rev4(0,&data[ptr]);
		ptr+=4;
		rev4(i+1,&data[ptr]);  // position in playlist
		ptr+=4;
		rev4(0,&data[ptr]);  // four nulls
		ptr+=4;
		rev4(0,&data[ptr]);
		ptr+=4;
		rev4(0,&data[ptr]);
		ptr+=4;
		rev4(0,&data[ptr]);
		ptr+=4;
		// the above code could be more optimized, but this is simpler to read, methinks
	}

	// fix the total size
	rev4(ptr,&data[8]);

	return ptr;
}

long iPod_mhyp::AddPlaylistEntry(iPod_mhip * entry, const unsigned long id)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_AddPlaylistEntry);
#endif

	entry = new iPod_mhip;
	ASSERT(entry != NULL);
	if (id && entry !=NULL)
		entry->songindex=id;
	if (entry !=NULL)
	{
		mhip.push_back(entry);
		return GetMhipChildrenCount()-1;
	}
	else return -1;
}

long iPod_mhyp::FindPlaylistEntry(const unsigned long id)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_FindPlaylistEntry);
#endif

	const unsigned long children = GetMhipChildrenCount();
	for (unsigned long i=0;i<children;i++)
	{
		if (mhip.at(i)->songindex==id)
			return i;
	}
	return -1;
}

bool iPod_mhyp::DeletePlaylistEntry(unsigned long pos)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_DeletePlaylistEntry);
#endif

	if (GetMhipChildrenCount() >= pos)
	{
		iPod_mhip * m = GetPlaylistEntry(pos);
		mhip.erase(mhip.begin()+pos);
		delete m;
		return true;
	}
	return false;
}

bool iPod_mhyp::ClearPlaylist()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_ClearPlaylist);
#endif

	const unsigned long count = GetMhipChildrenCount();
	for (unsigned long i=0;i<count;i++)
	{
		iPod_mhip * m = GetPlaylistEntry(i);
		delete m;
	}
	mhip.clear();
	return true;
}

long iPod_mhyp::PopulatePlaylist(iPod_mhlt * tracks, int hidden_field)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_PopulatePlaylist);
#endif

	ASSERT(tracks != NULL);
	if(tracks == NULL)
		return(-1);

	const unsigned long trackCount = tracks->GetChildrenCount();
	if(trackCount == 0)
		return(0);

	ClearPlaylist();

	// Speed up getting the id as follows:
	// Iterate the whole tracks->mhit map, storing the ids in a local vector
	vector<unsigned long> ids;
	ids.reserve(trackCount);

	iPod_mhlt::mhit_map_t::const_iterator begin = tracks->mhit.begin();
	iPod_mhlt::mhit_map_t::const_iterator end   = tracks->mhit.end();
	for(iPod_mhlt::mhit_map_t::const_iterator it = begin; it != end; it++)
		ids.push_back(static_cast<unsigned long>((*it).first));

	for (unsigned long i=0;i<trackCount;i++)
	{
		// Add the playlist entry locally rather than using
		// AddPlaylistEntry() for speed optimization reasons
		iPod_mhip *entry = new iPod_mhip;
		ASSERT(entry != NULL);
		if(entry)
		{
			entry->songindex = ids[i];
			mhip.push_back(entry);
		}
	}

	hidden=hidden_field;
	return GetMhipChildrenCount();
}

iPod_mhod * iPod_mhyp::AddString(const int type)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_AddString);
#endif

	iPod_mhod * m;
	if (type)
	{
		m = FindString(type);
		if (m!=NULL) return m;
	}

	m=new iPod_mhod;
	if (m!=NULL && type) m->type=type;

	mhod.push_back(m);
	return m;
}


iPod_mhod * iPod_mhyp::FindString(const unsigned long type)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_FindString);
#endif

	const unsigned long children = GetMhodChildrenCount();
	for (unsigned long i=0;i<children;i++)
	{
		if (mhod[i]->type == type) return mhod[i];
	}

	return NULL;
}

unsigned long iPod_mhyp::DeleteString(const unsigned long type)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_DeleteString);
#endif

	unsigned long count=0;
	const unsigned long children = GetMhodChildrenCount();
	for (unsigned long i=0;i<children;i++)
	{
		if (mhod[i]->type == type)
		{
			iPod_mhod * m = mhod.at(i);
			mhod.erase(mhod.begin()+i);
			delete m;
			i = i > 0 ? i - 1 : 0;  // do this to ensure that it checks the new entry in position i next
			count++;
		}
	}
	return count;
}

void iPod_mhyp::SetPlaylistTitle(const unsigned short *string)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhyp_SetPlaylistTitle);
#endif

	if(string == NULL)
		return;

	iPod_mhod *mhod = AddString(MHOD_TITLE);
	ASSERT(mhod);
	if(mhod)
		mhod->SetString(string);
}


unsigned __int64 iPod_mhyp::GeneratePlaylistID()
{
	GUID tmp;
	CoCreateGuid(&tmp);
	unsigned __int64 one   = tmp.Data1;
	unsigned __int64 two   = tmp.Data2;
	unsigned __int64 three = tmp.Data3;
	unsigned __int64 four  = rand();
	return(one << 32 | two << 16 | three | four);
}

//////////////////////////////////////////////////////////////////////
// iPod_mhip - Playlist Entry
//////////////////////////////////////////////////////////////////////
iPod_mhip::iPod_mhip() :
	unk1(1),
	corrid(0),
	unk2(0),
	songindex(0),
	timestamp(0)
{
}

iPod_mhip::~iPod_mhip()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhip_destructor);
#endif
}

long iPod_mhip::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhip_parse);
#endif

	long ptr=0;

	//check mhyp header
	if (_strnicmp((char *)&data[ptr],"mhip",4)) return -1;
	ptr+=4;

	// get sizes
	size_head=rev4(&data[ptr]);
	ptr+=4;
	size_total=rev4(&data[ptr]);
	ptr+=4;

	// read in the useful info
	unk1=rev4(&data[ptr]);
	ptr+=4;
	corrid=rev4(&data[ptr]);
	ptr+=4;
	unk2=rev4(&data[ptr]);
	ptr+=4;
	songindex=rev4(&data[ptr]);
	ptr+=4;
	timestamp=rev4(&data[ptr]);
	ptr+=4;

	ptr=size_total;

	// dump the mhod after the mhip, as it's just a position number in the playlist
	//   and useless to read in since we get them in order anyway
	iPod_mhod temp;
	long ret=temp.parse(&data[ptr]);
	ASSERT(ret >= 0);
	if (ret<0) return ret;
	else ptr+=ret;

	return ptr;
}


long iPod_mhip::write(unsigned char * data, const unsigned long datasize)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhip_write);
#endif

	long ptr=0;

	const unsigned int headsize=0x4c;
	// check for header size and mhod size
	if (headsize+0x2c>datasize)
	{
		ASSERT(0);
		return -1;
	}

	//write mhip header
	data[0]='m';data[1]='h';data[2]='i';data[3]='p';
	ptr+=4;

	// write sizes
	rev4(headsize,&data[ptr]);	// header size
	ptr+=4;
	rev4(headsize,&data[ptr]);  // mhips have no children or subdata
	ptr+=4;

	// fill in stuff
	rev4(unk1,&data[ptr]);
	ptr+=4;
	rev4(corrid,&data[ptr]);
	ptr+=4;
	rev4(unk2,&data[ptr]);
	ptr+=4;
	rev4(songindex,&data[ptr]);
	ptr+=4;
	rev4(timestamp,&data[ptr]);
	ptr+=4;

	// fill up the rest of the header with nulls
	for (unsigned int i=ptr;i<headsize;i++)
		data[i]=0;
	ptr=headsize;

	return ptr;
}


//////////////////////////////////////////////////////////////////////
// iPod_slst - Smart Playlist
//////////////////////////////////////////////////////////////////////

iPod_slst::iPod_slst() :
	splPref(NULL),
	splData(NULL)
{
	isSmartPlaylist = true;
	isPopulated = false;	// smart playlists are not considered populated by default
	Reset();
}

iPod_slst::~iPod_slst()
{
	RemoveAllRules();
}


iPod_slst::FieldType iPod_slst::GetFieldType(const unsigned long field)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_slst_GetFieldType);
#endif

	switch(field)
	{
		case SPLFIELD_SONG_NAME:
		case SPLFIELD_ALBUM:
		case SPLFIELD_ARTIST:
		case SPLFIELD_GENRE:
		case SPLFIELD_KIND:
		case SPLFIELD_COMMENT:
		case SPLFIELD_COMPOSER:
		case SPLFIELD_GROUPING:
			return(ftString);

		case SPLFIELD_BITRATE:
		case SPLFIELD_SAMPLE_RATE:
		case SPLFIELD_YEAR:
		case SPLFIELD_TRACKNUMBER:
		case SPLFIELD_SIZE:
		case SPLFIELD_PLAYCOUNT:
		case SPLFIELD_DISC_NUMBER:
		case SPLFIELD_BPM:
		case SPLFIELD_RATING:
		case SPLFIELD_TIME:				// time is the length of the track in milliseconds
			return(ftInt);

		case SPLFIELD_COMPILATION:
			return(ftBoolean);

		case SPLFIELD_DATE_MODIFIED:
		case SPLFIELD_DATE_ADDED:
		case SPLFIELD_LAST_PLAYED:
			return(ftDate);

		case SPLFIELD_PLAYLIST:
			return(ftPlaylist);

		default:
			// Unknown field type
			ASSERT(0);
	}

	return(ftUnknown);
}

iPod_slst::ActionType iPod_slst::GetActionType(const unsigned long field, const unsigned long action)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_slst_GetActionType);
#endif

	const FieldType fieldType = GetFieldType(field);
	switch(fieldType)
	{
		case ftString:
			switch(action)
			{
				case SPLACTION_IS_STRING:
				case SPLACTION_IS_NOT:
				case SPLACTION_CONTAINS:
				case SPLACTION_DOES_NOT_CONTAIN:
				case SPLACTION_STARTS_WITH:
				case SPLACTION_DOES_NOT_START_WITH:
				case SPLACTION_ENDS_WITH:
				case SPLACTION_DOES_NOT_END_WITH:
					return(atString);

				case SPLACTION_IS_NOT_IN_THE_RANGE:
				case SPLACTION_IS_INT:
				case SPLACTION_IS_NOT_INT:
				case SPLACTION_IS_GREATER_THAN:
				case SPLACTION_IS_NOT_GREATER_THAN:
				case SPLACTION_IS_LESS_THAN:
				case SPLACTION_IS_NOT_LESS_THAN:
				case SPLACTION_IS_IN_THE_RANGE:
				case SPLACTION_IS_IN_THE_LAST:
				case SPLACTION_IS_NOT_IN_THE_LAST:
					return(atInvalid);

				default:
					// Unknown action type
					ASSERT(0);
					return(atUnknown);
			}
			break;

		case ftInt:
			switch(action)
			{
				case SPLACTION_IS_INT:
				case SPLACTION_IS_NOT_INT:
				case SPLACTION_IS_GREATER_THAN:
				case SPLACTION_IS_NOT_GREATER_THAN:
				case SPLACTION_IS_LESS_THAN:
				case SPLACTION_IS_NOT_LESS_THAN:
					return(atInt);

				case SPLACTION_IS_NOT_IN_THE_RANGE:
				case SPLACTION_IS_IN_THE_RANGE:
					return(atRange);

				case SPLACTION_IS_STRING:
				case SPLACTION_CONTAINS:
				case SPLACTION_STARTS_WITH:
				case SPLACTION_DOES_NOT_START_WITH:
				case SPLACTION_ENDS_WITH:
				case SPLACTION_DOES_NOT_END_WITH:
				case SPLACTION_IS_IN_THE_LAST:
				case SPLACTION_IS_NOT_IN_THE_LAST:
				case SPLACTION_IS_NOT:
				case SPLACTION_DOES_NOT_CONTAIN:
					return(atInvalid);

				default:
					// Unknown action type
					ASSERT(0);
					return(atUnknown);
			}
			break;

		case ftBoolean:
			return(atNone);

		case ftDate:
			switch(action)
			{
				case SPLACTION_IS_INT:
				case SPLACTION_IS_NOT_INT:
				case SPLACTION_IS_GREATER_THAN:
				case SPLACTION_IS_NOT_GREATER_THAN:
				case SPLACTION_IS_LESS_THAN:
				case SPLACTION_IS_NOT_LESS_THAN:
					return(atDate);

				case SPLACTION_IS_IN_THE_LAST:
				case SPLACTION_IS_NOT_IN_THE_LAST:
					return(atInTheLast);

				case SPLACTION_IS_IN_THE_RANGE:
				case SPLACTION_IS_NOT_IN_THE_RANGE:
					return(atRange);

				case SPLACTION_IS_STRING:
				case SPLACTION_CONTAINS:
				case SPLACTION_STARTS_WITH:
				case SPLACTION_DOES_NOT_START_WITH:
				case SPLACTION_ENDS_WITH:
				case SPLACTION_DOES_NOT_END_WITH:
				case SPLACTION_IS_NOT:
				case SPLACTION_DOES_NOT_CONTAIN:
					return(atInvalid);

				default:
					// Unknown action type
					ASSERT(0);
					return(atUnknown);
			}
			break;

		case ftPlaylist:
			switch(action)
			{
				case SPLACTION_IS_INT:
				case SPLACTION_IS_NOT_INT:
					return (atPlaylist);

				case SPLACTION_IS_GREATER_THAN:
				case SPLACTION_IS_NOT_GREATER_THAN:
				case SPLACTION_IS_LESS_THAN:
				case SPLACTION_IS_NOT_LESS_THAN:
				case SPLACTION_IS_IN_THE_LAST:
				case SPLACTION_IS_NOT_IN_THE_LAST:
				case SPLACTION_IS_IN_THE_RANGE:
				case SPLACTION_IS_NOT_IN_THE_RANGE:
				case SPLACTION_IS_STRING:
				case SPLACTION_CONTAINS:
				case SPLACTION_STARTS_WITH:
				case SPLACTION_DOES_NOT_START_WITH:
				case SPLACTION_ENDS_WITH:
				case SPLACTION_DOES_NOT_END_WITH:
				case SPLACTION_IS_NOT:
				case SPLACTION_DOES_NOT_CONTAIN:
					return (atInvalid);

				default:
					ASSERT(0);
					return(atUnknown);
			}

		case ftUnknown:
			// Unknown action type
			ASSERT(0);
			break;
	}

	return(atUnknown);
}


void iPod_slst::UpdateMHODPointers()
{
	splPref = AddString(MHOD_SPLPREF);
	splData = AddString(MHOD_SPLDATA);
}

void iPod_slst::SetPrefs(const bool liveupdate, const bool rules_enabled, const bool limits_enabled,
			             const unsigned long limitvalue, const unsigned long limittype, const unsigned long limitsort)
{
	UpdateMHODPointers();

	ASSERT(splPref);
	if(splPref)
	{
		splPref->liveupdate = liveupdate ? 1 : 0;
		splPref->checkrules = rules_enabled ? 1 : 0;
		splPref->checklimits = limits_enabled ? 1 : 0;
		splPref->matchcheckedonly = 0;
		splPref->limitsort_opposite = limitsort & 0x80000000 ? 1 : 0;
		splPref->limittype = limittype;
		splPref->limitsort = limitsort & 0x000000ff;
		splPref->limitvalue = limitvalue;
	}
}

unsigned long iPod_slst::GetRuleCount()
{
	UpdateMHODPointers();

	ASSERT(splData);
	if(splData == NULL)
		return(0);

	return(splData->rule.size());
}

int iPod_slst::AddStringRule(const unsigned long field, const unsigned long action, const unsigned short * string)
{
	if(GetFieldType(field) != ftString)
	{
		ASSERT(0);
		return(-1);
	}

	return(AddRule(field, action, string));
}

int iPod_slst::AddSingleValueRule(const unsigned long field, const unsigned long action, const unsigned __int64 value)
{
	const FieldType ft = GetFieldType(field);
	if(!(ft == ftInt || ft == ftBoolean || ft == ftDate || ft == ftPlaylist))
	{
		ASSERT(0);
		return(-1);
	}

	return(AddRule(field, action, NULL, value));
}

int iPod_slst::AddRangeRule(const unsigned long field, const unsigned long action, const unsigned __int64 from, const unsigned __int64 to)
{
	if(GetActionType(field, action) != atRange)
	{
		ASSERT(0);
		return(-1);
	}

	return(AddRule(field, action, NULL, 0, from, to));
}

int iPod_slst::AddInTheLastRule(const unsigned long field, const unsigned long action, const unsigned __int64 value, const unsigned __int64 units)
{
	if(GetActionType(field, action) != atInTheLast)
	{
		ASSERT(0);
		return(-1);
	}

	return(AddRule(field, action, NULL, value, 0, 0, units));
}

int iPod_slst::AddRule(const unsigned long field,
					   const unsigned long action,
					   const unsigned short * string,	// use string for string based rules
					   const unsigned __int64 value,	// use value for single variable rules
					   const unsigned __int64 from,		// use from and to for range based rules
					   const unsigned __int64 to,
					   const unsigned __int64 units)	// use units for "In The Last" based rules
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_slst_AddRule);
#endif

	UpdateMHODPointers();

	ASSERT(splPref != NULL && splData != NULL);
	if (splPref == NULL || splData == NULL) return -1;

	// create a new rule
	SPLRule *r = new SPLRule;
	ASSERT(r);
	if(r == NULL)
		return(-1);

	const FieldType  ft = GetFieldType(field);
	const ActionType at = GetActionType(field, action);

	if(ft == ftUnknown || at == atUnknown)
	{
		ASSERT(0);
		return(-1);
	}


	r->field = field;
	r->action = action;
	r->unk1 = 0;
	r->unk2 = 0;
	r->unk3 = 0;
	r->unk4 = 0;
	r->unk5 = 0;

	if(ft == ftString)
	{
		// it's a string type (SetString() sets the length)
		r->SetString(string);
	}
	else
	{
		// All non-string rules currently have a length of 68 bytes
		r->length = 0x44;

		/* Values based on ActionType:
		 *   int:         fromvalue = value, fromdate = 0, fromunits = 1, tovalue = value, todate = 0, tounits = 1
		 *   playlist:    same as int
		 *   boolean:     same as int, except fromvalue/tovalue are 1 if set, 0 if not set
		 *   date:        same as int
		 *   range:       same as int, except use from and to, instead of value
		 *   in the last: fromvalue = 0x2dae2dae2dae2dae (SPLDATE_IDENTIFIER), fromdate = 0xffffffffffffffff - value, fromunits = seconds in period,
		 *			      tovalue = 0x2dae2dae2dae2dae (SPLDATE_IDENTIFIER), todate = 0xffffffffffffffff - value, tounits = seconds in period
		 */
		switch(at)
		{
			case atInt:
			case atPlaylist:
			case atDate:
				r->fromvalue = value;
				r->fromdate = 0;
				r->fromunits = 1;
				r->tovalue = value;
				r->todate = 0;
				r->tounits = 1;
				break;
			case atBoolean:
				r->fromvalue = value > 0 ? 1 : 0;
				r->fromdate = 0;
				r->fromunits = 1;
				r->tovalue = r->fromvalue;
				r->todate = 0;
				r->tounits = 1;
				break;
			case atRange:
				r->fromvalue = from;
				r->fromdate = 0;
				r->fromunits = 1;
				r->tovalue = to;
				r->todate = 0;
				r->tounits = 1;
				break;
			case atInTheLast:
				r->fromvalue = SPLDATE_IDENTIFIER;
				r->fromdate = ConvertNumToDateValue(value);
				r->fromunits = units;
				r->tovalue = SPLDATE_IDENTIFIER;
				r->todate = ConvertNumToDateValue(value);
				r->tounits = units;
				break;
			case atNone:
				break;
			default:
				ASSERT(0);
				break;
		}
	}

	// set isPopulated to false, since we're modifying the rules
	isPopulated = false;

	// push the rule into the mhod
	splData->rule.push_back(r);
	return(splData->rule.size() - 1);
}


bool iPod_slst::GetRule(const unsigned int index,
						unsigned long field,
						unsigned long action,
						unsigned short string[SPL_MAXSTRINGLENGTH+1],
						unsigned long length,
						unsigned __int64 from,
						unsigned __int64 to,
						unsigned __int64 units)
{
	UpdateMHODPointers();

	ASSERT(splPref != NULL && splData != NULL);
	if(splPref == NULL || splData == NULL)
		return false;

	SPLRule * r = splData->rule.at(index);
	ASSERT(r);
	if (!r) return false;

	field = r->field;
	action = r->action;

	const FieldType  ft = GetFieldType(field);
	const ActionType at = GetActionType(field, action);

	if(ft == ftString)
	{
		// it's a string type
		memset(string,0,sizeof(string));
		wcsncpy(string, r->string, SPL_MAXSTRINGLENGTH);
		length = r->length;
		// rest of values are not needed for string types
		// TODO? May want to set them to zero here?
	}
	else
	{
		switch(at)
		{
			case atInt:
			case atDate:
			case atBoolean:
			case atRange:
			case atPlaylist:
			case atNone:
				from = r->fromvalue;
				units = r->fromunits;
				to = r->tovalue;
			case atInTheLast:
				from = - (r->fromdate);
				units = r->fromunits;
				to = - (r->todate);
				break;
			default:
				ASSERT(0);
				break;
		}
	}

	return true;
}

bool iPod_slst::DeleteRule(const unsigned int index)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_slst_DeleteRule);
#endif

	UpdateMHODPointers();

	ASSERT(splData != NULL);
	if(splData == NULL)
		return(false);

	if(index > splData->rule.size())
		return(false);

	SPLRule *r = splData->rule.at(index);
	ASSERT(r);
	if(r)
	{
		// set isPopulated to false, since we're modifying the rules
		isPopulated = false;

		splData->rule.erase(splData->rule.begin() + index);
		delete r;
		return(true);
	}

	return(false);
}

SPLRule* iPod_slst::GetRule(const unsigned int index)
{
	UpdateMHODPointers();

	ASSERT(splData);
	if(splData == NULL)
		return(NULL);

	const unsigned int ruleCount = splData->rule.size();
	if(index + 1 > ruleCount || ruleCount == 0)
		return(NULL);

	return(splData->rule[index]);
}

void iPod_slst::RemoveAllRules()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_slst_RemoveAllRules);
#endif

	// Note: Don't update MHOD pointers here...if they don't already exist, there is no point in creating them
	if(splData == NULL)
		return;

	while(splData->rule.size() > 0)
	{
		// set isPopulated to false, since we're modifying the rules
		isPopulated = false;

		SPLRule *r = splData->rule[0];
		splData->rule.erase(splData->rule.begin());
		delete r;
	}

	splData->rule.clear();
}


void iPod_slst::Reset()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_slst_Reset);
#endif

	// Note: Don't update MHOD pointers here...if they don't already exist, there is no point in creating them
	RemoveAllRules();

	isPopulated = false;

	if(splPref)
	{
		splPref->liveupdate = 1;
		splPref->checkrules = 1;
		splPref->checklimits = 0;
		splPref->matchcheckedonly = 0;
		splPref->limittype = LIMITTYPE_SONGS;
		splPref->limitsort = LIMITSORT_RANDOM;
		splPref->limitsort_opposite = 0;
		splPref->limitvalue = 25;
	}

	if(splData)
	{
		splData->rules_operator = SPLMATCH_AND;
		splData->unk5 = 0;
	}
}

// function to evaluate a rule's truth against a track
//bool iPod_slst::EvalRule(SPLRule * r, iPod_mhit * track)
bool iPod_slst::EvalRule(SPLRule * r,
						 iPod_mhit * track,
						 iPod_mhlt * tracks,
						 iPod_mhlp * playlists)
{
	FieldType ft = iPod_slst::GetFieldType(r->field);
	ActionType at = iPod_slst::GetActionType(r->field,r->action);

	if (at==iPod_slst::atInvalid) return false;	// if the rule is invalid, call it false

	wchar_t * strcomp = NULL;
	unsigned long intcomp = 0;
	unsigned char boolcomp = 0;
	unsigned long datecomp = 0;
	iPod_mhyp * playcomp = NULL;	// assume it's a smart playlist.. we can treat it as such anyway
	iPod_mhod * mhod = NULL;

	// find what we need to compare in the mhit
	switch (r->field)
	{
		case SPLFIELD_SONG_NAME:
			ASSERT(track);
			if(track)
			{
				mhod = track->FindString(MHOD_TITLE);
				if(mhod)
					strcomp = mhod->str;
			}
			break;
		case SPLFIELD_ALBUM:
			ASSERT(track);
			if(track)
			{
				mhod = track->FindString(MHOD_ALBUM);
				if(mhod)
					strcomp = mhod->str;
			}
			break;
		case SPLFIELD_ARTIST:
			ASSERT(track);
			if(track)
			{
				mhod = track->FindString(MHOD_ARTIST);
				if(mhod)
					strcomp = mhod->str;
			}
			break;
		case SPLFIELD_GENRE:
			ASSERT(track);
			if(track)
			{
				mhod = track->FindString(MHOD_GENRE);
				if(mhod)
					strcomp = mhod->str;
			}
			break;
		case SPLFIELD_KIND:
			ASSERT(track);
			if(track)
			{
				mhod = track->FindString(MHOD_FILETYPE);
				if(mhod)
					strcomp = mhod->str;
			}
			break;
		case SPLFIELD_COMMENT:
			ASSERT(track);
			if(track)
			{
				mhod = track->FindString(MHOD_COMMENT);
				if(mhod)
					strcomp = mhod->str;
			}
			break;
		case SPLFIELD_COMPOSER:
			ASSERT(track);
			if(track)
			{
				mhod = track->FindString(MHOD_COMPOSER);
				if(mhod)
					strcomp = mhod->str;
			}
			break;
		case SPLFIELD_GROUPING:
			ASSERT(track);
			if(track)
			{
				mhod = track->FindString(MHOD_GROUPING);
				if(mhod)
					strcomp = mhod->str;
			}
			break;
		case SPLFIELD_BITRATE:
			ASSERT(track);
			if(track)
				intcomp = track->bitrate; break;
		case SPLFIELD_SAMPLE_RATE:
			ASSERT(track);
			if(track)
				intcomp = track->samplerate; break;
		case SPLFIELD_YEAR:
			ASSERT(track);
			if(track)
				intcomp = track->year; break;
		case SPLFIELD_TRACKNUMBER:
			ASSERT(track);
			if(track)
				intcomp = track->tracknum; break;
		case SPLFIELD_SIZE:
			ASSERT(track);
			if(track)
				intcomp = track->size; break;
		case SPLFIELD_PLAYCOUNT:
			ASSERT(track);
			if(track)
				intcomp = track->playcount; break;
		case SPLFIELD_DISC_NUMBER:
			ASSERT(track);
			if(track)
				intcomp = track->cdnum; break;
		case SPLFIELD_BPM:
			ASSERT(track);
			if(track)
				intcomp = track->BPM; break;
		case SPLFIELD_RATING:
			ASSERT(track);
			if(track)
				intcomp = track->stars; break;
		case SPLFIELD_TIME:
			ASSERT(track);
			if(track)
				intcomp = track->length; break;
		case SPLFIELD_COMPILATION:
			ASSERT(track);
			if(track)
				boolcomp = track->compilation; break;
		case SPLFIELD_DATE_MODIFIED:
			ASSERT(track);
			if(track && track->lastmodtime > 0)
				datecomp = mactime_to_wintime(track->lastmodtime);
			break;
		case SPLFIELD_DATE_ADDED:
			ASSERT(track);
			if(track && track->createdtime > 0)
				datecomp = mactime_to_wintime(track->createdtime);
			break;
		case SPLFIELD_LAST_PLAYED:
			ASSERT(track);
			if(track && track->lastplayedtime > 0)
				datecomp = mactime_to_wintime(track->lastplayedtime);
			break;
		case SPLFIELD_PLAYLIST:
			ASSERT(playlists);
			if(playlists)
				playcomp = playlists->FindPlaylist(r->fromvalue);
			break;
		default:					// unknown field type
			ASSERT (0); break;
	}

	// some temp variables we'll need to compare things
	wchar_t tempstr1[SPL_MAXSTRINGLENGTH+1],tempstr2[SPL_MAXSTRINGLENGTH+1];
	time_t timer;

	// actually do the comparison to our rule
	switch (ft)
	{
	case ftString:
		if(strcomp == NULL || r->string == NULL)
			return(false);

		switch (r->action)
		{
		case SPLACTION_IS_STRING:
			return (wcscmp(strcomp,r->string)==0);
		case SPLACTION_IS_NOT:
			return (wcscmp(strcomp,r->string)!=0);
		case SPLACTION_CONTAINS:
			return (wcsstr(strcomp,r->string)!=NULL);
		case SPLACTION_DOES_NOT_CONTAIN:
			return (wcsstr(strcomp,r->string)==NULL);
		case SPLACTION_STARTS_WITH:
			return (wcsncmp(strcomp,r->string,wcslen(r->string))==0);
		case SPLACTION_ENDS_WITH:
			// tricky.. copy the strings, reverse them, do the same check at starts_with
			// look for a faster way to do this
			wcscpy(tempstr1,strcomp);
			wcscpy(tempstr2,r->string);
			wcsrev(tempstr1);
			wcsrev(tempstr2);
			return (wcsncmp(tempstr1, tempstr2,wcslen(tempstr2))==0);
		case SPLACTION_DOES_NOT_START_WITH:
			return (wcsncmp(strcomp,r->string,wcslen(r->string))!=0);
		case SPLACTION_DOES_NOT_END_WITH:
			wcscpy(tempstr1,strcomp);
			wcscpy(tempstr2,r->string);
			wcsrev(tempstr1);
			wcsrev(tempstr2);
			return (wcsncmp(tempstr1, tempstr2,wcslen(tempstr2))!=0);
		};
		return false;
	case ftInt:
		switch(r->action)
		{
		case SPLACTION_IS_INT:
			return (intcomp == r->fromvalue);
		case SPLACTION_IS_NOT_INT:
			return (intcomp != r->fromvalue);
		case SPLACTION_IS_GREATER_THAN:
			return (intcomp > r->fromvalue);
		case SPLACTION_IS_LESS_THAN:
			return (intcomp < r->fromvalue);
		case SPLACTION_IS_IN_THE_RANGE:
			return ((intcomp < r->fromvalue && intcomp > r->tovalue) ||
					(intcomp > r->fromvalue && intcomp < r->tovalue));
		case SPLACTION_IS_NOT_IN_THE_RANGE:
			return ((intcomp < r->fromvalue && intcomp < r->tovalue) ||
					(intcomp > r->fromvalue && intcomp > r->tovalue));
		}
		return false;
	case ftBoolean:
		switch (r->action)
		{
		case SPLACTION_IS_INT:	// aka "is set"
			return (boolcomp != 0);
		case SPLACTION_IS_NOT_INT:	// aka "is not set"
			return (boolcomp == 0);
		}
		return false;
	case ftDate:
		switch (r->action)
		{
		case SPLACTION_IS_INT:
			return (datecomp == r->fromvalue);
		case SPLACTION_IS_NOT_INT:
			return (datecomp != r->fromvalue);
		case SPLACTION_IS_GREATER_THAN:
			return (datecomp > r->fromvalue);
		case SPLACTION_IS_LESS_THAN:
			return (datecomp < r->fromvalue);
		case SPLACTION_IS_NOT_GREATER_THAN:
			return (datecomp <= r->fromvalue);
		case SPLACTION_IS_NOT_LESS_THAN:
			return (datecomp >= r->fromvalue);
		case SPLACTION_IS_IN_THE_LAST:
			time(&timer);
			timer += (r->fromdate * r->fromunits);
			return (datecomp > timer);
		case SPLACTION_IS_NOT_IN_THE_LAST:
			time(&timer);
			timer += (r->fromdate * r->fromunits);
			return (datecomp <= timer);
		case SPLACTION_IS_IN_THE_RANGE:
			return ((datecomp < r->fromvalue && datecomp > r->tovalue) ||
					(datecomp > r->fromvalue && datecomp < r->tovalue));
		case SPLACTION_IS_NOT_IN_THE_RANGE:
			return ((datecomp < r->fromvalue && datecomp < r->tovalue) ||
					(datecomp > r->fromvalue && datecomp > r->tovalue));
		}
		return false;
	case ftPlaylist:
		// if we didn't find the playlist, just exit instead of dealing with it
		if (playcomp == NULL) return false;

		// first, deal with smartplaylists
		if (playcomp->IsSmartPlaylist())
		{
			ASSERT(tracks);
			if(tracks)
			{
				iPod_slst * smartplaycomp = (iPod_slst*)playcomp;
				smartplaycomp->PopulateSmartPlaylist(tracks, playlists);
			}
		}
		// now deal with the action
		switch(r->action)
		{
		case SPLACTION_IS_INT:	// is this track in this playlist?
			return (playcomp->FindPlaylistEntry(track->id) != -1);
		case SPLACTION_IS_NOT_INT:	// is this track NOT in this playlist?
			return (playcomp->FindPlaylistEntry(track->id) == -1);
		}
		return false;

	case ftUnknown:	// don't know how to deal with Unknown's
		ASSERT(0);
		return false;

	default:		// new type needs to assert to warn us to change this code
		ASSERT(0);
		return false;
	}

	ASSERT(0);		// we should never make it out of the above switches alive
	return false;
}


// local functions to help with the sorting of the list of tracks so that we can do limits

bool compSongName(iPod_mhit* x, iPod_mhit* y)
{	return (wcscmp(x->FindString(MHOD_TITLE)->str,y->FindString(MHOD_TITLE)->str)<0);	}
bool compAlbum(iPod_mhit* x, iPod_mhit* y)
{	return (wcscmp(x->FindString(MHOD_ALBUM)->str,y->FindString(MHOD_ALBUM)->str)<0);	}
bool compArtist(iPod_mhit* x, iPod_mhit* y)
{	return (wcscmp(x->FindString(MHOD_ARTIST)->str,y->FindString(MHOD_ARTIST)->str)<0);	}
bool compGenre(iPod_mhit* x, iPod_mhit* y)
{	return (wcscmp(x->FindString(MHOD_GENRE)->str,y->FindString(MHOD_GENRE)->str)<0);	}

// Note: I may have gotten my >'s and <'s reversed in the following comparisons. If you notice
// the lists coming out in the wrong order, try reversing the comparisons in these functions.
bool compMostRecentlyAdded(iPod_mhit* x, iPod_mhit* y)
{	return (x->createdtime > y->createdtime); }
bool compLeastRecentlyAdded(iPod_mhit* x, iPod_mhit* y)
{	return (x->createdtime < y->createdtime); }
bool compMostOftenPlayed(iPod_mhit* x, iPod_mhit* y)
{	return (x->playcount > y->playcount); }
bool compLeastOftenPlayed(iPod_mhit* x, iPod_mhit* y)
{	return (x->playcount < y->playcount); }
bool compMostRecentlyPlayed(iPod_mhit* x, iPod_mhit* y)
{	return (x->lastplayedtime > y->lastplayedtime); }
bool compLeastRecentlyPlayed(iPod_mhit* x, iPod_mhit* y)
{	return (x->lastplayedtime < y->lastplayedtime); }
bool compHighestRating(iPod_mhit* x, iPod_mhit* y)
{	return (x->stars > y->stars); }
bool compLowestRating(iPod_mhit* x, iPod_mhit* y)
{	return (x->stars < y->stars); }

long iPod_slst::PopulateSmartPlaylist(iPod_mhlt * tracks, iPod_mhlp * playlists)
{

	ASSERT(tracks != NULL);
	if(tracks == NULL)
		return -1;

	if(tracks->GetChildrenCount() == 0)
		return 0;

	// check to see if this playlist is already populated and up to date
	// gives us a big speedup on evaluating playlist type rules
	if (isPopulated)
		return GetMhipChildrenCount();

	// clear this playlist, because we're rebuilding using the rules
	ClearPlaylist();

	// set isPopulated to true, since we're now populating it
	// this should solve any infinite loop problem, since we set it while it's still
	// being populated
	isPopulated = true;

	const bool rulesEnabled = (GetPrefs()->checkrules!=0);
	const bool limitsEnabled = (GetPrefs()->checklimits!=0);

	// Speed up getting the id as follows:
	// Iterate the whole tracks->mhit map, storing the ids in a local vector
	vector<iPod_mhit *> ids;

	// Note: the rule_operator is part of the SPLDATA, not SPLPREFS (very easy to mix up!)
	const unsigned long rules_operator = GetRules()->rules_operator;
	const bool checked_only = (GetPrefs()->matchcheckedonly != 0);

	iPod_mhlt::mhit_map_t::const_iterator begin = tracks->mhit.begin();
	iPod_mhlt::mhit_map_t::const_iterator end   = tracks->mhit.end();
	for(iPod_mhlt::mhit_map_t::const_iterator it = begin; it != end; it++)
	{
		// skip non-checked songs if we have to do so (this takes care of *all* the match_checked functionality)
		if (checked_only && ((*it).second->checked == 0))
			continue;

		// first, match the rules
		if (rulesEnabled) // if we are set to check the rules
		{
			// start with true for "match all", start with false for "match any"
			bool matchrules=(rules_operator == SPLMATCH_AND);
			const unsigned long rulecount=GetRuleCount();
			if (rulecount == 0) matchrules=true; // assume everything matches with no rules
			for (unsigned long i=0;i<rulecount;i++)
			{
				SPLRule* r = GetRule(i);
				bool ruletruth=EvalRule(r,(*it).second, tracks, playlists);
				if (rules_operator == SPLMATCH_AND) // match all
					matchrules = matchrules && ruletruth;
				else if (rules_operator == SPLMATCH_OR)
					matchrules = matchrules || ruletruth;
			}
			if (matchrules)
			{
				// we have a track that matches the ruleset, so save a pointer to it for now
				ids.push_back(static_cast<iPod_mhit*>((*it).second));
			}
		}
		else	// we aren't checking the rules, so stick all the mhits onto the list
			ids.push_back(static_cast<iPod_mhit*>((*it).second));
	}

	// no reason to go on if nothing matches so far
	if (ids.size() == 0) return 0;

	// do the limits
	if (limitsEnabled)
	{
		// limit to (number) (type) selected by (sort)
		// first, we sort the list
		switch(GetPrefs()->limitsort)
		{
		case LIMITSORT_RANDOM:
			random_shuffle(ids.begin(),ids.end()); break;
		case LIMITSORT_SONG_NAME:
			sort(ids.begin(),ids.end(),compSongName); break;
		case LIMITSORT_ALBUM:
			sort(ids.begin(),ids.end(),compAlbum); break;
		case LIMITSORT_ARTIST:
			sort(ids.begin(),ids.end(),compArtist); break;
		case LIMITSORT_GENRE:
			sort(ids.begin(),ids.end(),compGenre); break;
		case LIMITSORT_MOST_RECENTLY_ADDED:
			sort(ids.begin(),ids.end(),compMostRecentlyAdded); break;
		case LIMITSORT_LEAST_RECENTLY_ADDED:
			sort(ids.begin(),ids.end(),compLeastRecentlyAdded); break;
		case LIMITSORT_MOST_OFTEN_PLAYED:
			sort(ids.begin(),ids.end(),compMostOftenPlayed); break;
		case LIMITSORT_LEAST_OFTEN_PLAYED:
			sort(ids.begin(),ids.end(),compLeastOftenPlayed); break;
		case LIMITSORT_MOST_RECENTLY_PLAYED:
			sort(ids.begin(),ids.end(),compMostRecentlyPlayed); break;
		case LIMITSORT_LEAST_RECENTLY_PLAYED:
			sort(ids.begin(),ids.end(),compLeastRecentlyPlayed); break;
		case LIMITSORT_HIGHEST_RATING:
			sort(ids.begin(),ids.end(),compHighestRating); break;
		case LIMITSORT_LOWEST_RATING:
			sort(ids.begin(),ids.end(),compLowestRating); break;
		default:
			ASSERT(0);
			break;
		}
		// now that the list is sorted in the order we want, we take the top X
		// tracks off the list and insert them into our playlist
		double runningtotal = 0;	// use a double because we may need to deal with fractions here
		unsigned long trackcounter = 0;
		const unsigned long limitval = GetPrefs()->limitvalue;
		while (runningtotal < limitval && trackcounter < ids.size())
		{
			double currentvalue=0;
			// get the next song's value to add to running total
			switch (GetPrefs()->limittype)
			{
			case LIMITTYPE_MINUTES:
				currentvalue = (double)(ids[trackcounter]->length)/60; break;
			case LIMITTYPE_HOURS:
				currentvalue = (double)(ids[trackcounter]->length)/(60*60); break;
			case LIMITTYPE_MB:
				currentvalue = (double)(ids[trackcounter]->size)/(1024*1024); break;
			case LIMITTYPE_GB:
				currentvalue = (double)(ids[trackcounter]->size)/(1024*1024*1024); break;
			case LIMITTYPE_SONGS:
				currentvalue = 1; break;
			default:
				ASSERT(0); break;
			}

			// check to see that we won't actually exceed the limitvalue
			if (runningtotal + currentvalue <= limitval)
			{
				runningtotal += currentvalue;
				// Add the playlist entry locally rather than using
				// AddPlaylistEntry() for speed optimization reasons
				iPod_mhip *entry = new iPod_mhip;
				ASSERT(entry != NULL);
				if(entry)
				{
					entry->songindex = ids[trackcounter]->id;
					mhip.push_back(entry);
				}
			}	// end if it'll fit

			//increment the track counter so we can look at the next track
			trackcounter++;
		}	// end while
	} // end if limits enabled
	else		// no limits, so stick everything that matched the rules into the playlist
	{
		for (unsigned long i=0;i<ids.size();i++)
		{
			// Add the playlist entry locally rather than using
			// AddPlaylistEntry() for speed optimization reasons
			iPod_mhip *entry = new iPod_mhip;
			ASSERT(entry != NULL);
			if(entry)
			{
				entry->songindex = ids[i]->id;
				mhip.push_back(entry);
			}
		}
	}

	return GetMhipChildrenCount();
}



//////////////////////////////////////////////////////////////////////
// iPod_mhdp - Class for parsing the Play Counts file
//////////////////////////////////////////////////////////////////////

long iPod_mhdp::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhdp_parse);
#endif

	long ptr=0;

	//check mhdp header
	if (_strnicmp((char *)&data[ptr],"mhdp",4)) return -1;
	ptr+=4;

	// get sizes
	size_head=rev4(&data[ptr]);
	ptr+=4;
	entrysize=rev4(&data[ptr]);
	ptr+=4;
	if (entrysize != 0x10 && entrysize != 0x0c)
	{
		ASSERT(0);
		return -1;	// can't understand new versions of this file
	}

	const unsigned long children=rev4(&data[ptr]);	 // Only used locally
	ptr+=4;

	// skip dummy space
	ptr=size_head;

	unsigned long i;

	entry.reserve(children);	// pre allocate the space, for speed

	for (i=0;i<children;i++)
	{
		PCEntry e;

		e.playcount=rev4(&data[ptr]);
		ptr+=4;
		e.lastplayedtime=rev4(&data[ptr]);
		ptr+=4;
		e.bookmarktime=rev4(&data[ptr]);
		ptr+=4;
		if (entrysize >= 0x10)
		{
			e.stars=rev4(&data[ptr]);
			ptr+=4;
		}

		entry.push_back(e);
	}

	return children;
}


//////////////////////////////////////////////////////////////////////
// iPod_mhpo - Class for parsing the OTGPlaylist file
//////////////////////////////////////////////////////////////////////

iPod_mhpo::iPod_mhpo() :
	size_head(0),
	unk1(0),
	unk2(0),
	idList()
{
}

iPod_mhpo::~iPod_mhpo()
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhpo_destructor);
#endif
}

long iPod_mhpo::parse(unsigned char * data)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhpo_parse);
#endif

	long ptr=0;

	//check mhdp header
	if (_strnicmp((char *)&data[ptr],"mhpo",4)) return -1;
	ptr+=4;

	// get sizes
	size_head=rev4(&data[ptr]);
	ptr+=4;

	unk1=rev4(&data[ptr]);
	ptr+=4;

	const unsigned long children=rev4(&data[ptr]);	 // Only used locally
	ptr+=4;

	unk2=rev4(&data[ptr]);
	ptr+=4;

	if (ptr!=size_head) return -1; // if this isn't true, I screwed up somewhere

	unsigned long i;

	idList.reserve(children);	// pre allocate the space, for speed

	for (i=0;i<children;i++)
	{
		unsigned long temp;
		temp=rev4(&data[ptr]);
		ptr+=4;
		idList.push_back(temp);
	}

	return ptr;
}

long iPod_mhpo::write(unsigned char * data, const unsigned long datasize)
{
#ifdef IPODDB_PROFILER
	profiler(iPodDB__iPod_mhpo_write);
#endif

	long ptr=0;

	const unsigned int headsize=0x14;
	// check for header size + child size
	if (headsize+(GetChildrenCount()*4)>datasize) return -1;

	//write mhpo header
	data[0]='m';data[1]='h';data[2]='p';data[3]='o';
	ptr+=4;

	// write size
	rev4(headsize,&data[ptr]);	// header size
	ptr+=4;

	// fill in stuff
	rev4(unk1,&data[ptr]);
	ptr+=4;
	const unsigned long children = GetChildrenCount();
	rev4(children,&data[ptr]);
	ptr+=4;
	rev4(unk2,&data[ptr]);
	ptr+=4;

	for (unsigned long i=0;i<children;i++)
	{
		rev4(idList[i],&data[ptr]);
		ptr+=4;

	}
	return ptr;
}


iPod_mhyp * iPod_mhpo::CreatePlaylistFromOTG(iPod_mhbd * iPodDB, unsigned short * name)
{
	// create playlist
	iPod_mhyp * myplaylist = iPodDB->mhsdplaylists->mhlp->AddPlaylist();
	if (myplaylist == NULL)
		return NULL;

	// set name
	iPod_mhod * playlistName = myplaylist->AddString(MHOD_TITLE);
	playlistName->SetString(name);

	unsigned long count = GetChildrenCount();
	// for every track
	for (unsigned long i=0;i<count;i++)
	{
		// find the track
		iPod_mhit * track = iPodDB->mhsdsongs->mhlt->GetTrack(idList[i]);

		// myplaylist->AddPlaylistEntry(iPod_mhip * entry, track->id);

		// Add the playlist entry locally rather than using
		// AddPlaylistEntry() for speed optimization reasons
		iPod_mhip *entry = new iPod_mhip;
		ASSERT(entry != NULL);
		if(entry)
		{
			entry->songindex = track->id;
			myplaylist->mhip.push_back(entry);
		}
	}

	return myplaylist;
}


//////////////////////////////////////////////////////////////////////
// iPod_mqed - Class for parsing the EQ Presets file
//////////////////////////////////////////////////////////////////////
iPod_mqed::iPod_mqed() :
	unk1(1),
	unk2(1),
	eqList(),
	size_head(0x68)
{
}

iPod_mqed::~iPod_mqed()
{
	const unsigned long count = GetChildrenCount();
	for (unsigned long i=0;i<count;i++)
	{
		iPod_pqed * m=eqList[i];
		delete m;
	}
	eqList.clear();
}

long iPod_mqed::parse(unsigned char * data)
{
	unsigned long ptr=0;
	int i;

	//check mqed header
	if (_strnicmp((char *)&data[ptr],"mqed",4)) return -1;
	ptr+=4;

	size_head=rev4(&data[ptr]);
	ptr+=4;
	unk1=rev4(&data[ptr]);
	ptr+=4;
	unk2=rev4(&data[ptr]);
	ptr+=4;
	unsigned long numchildren=rev4(&data[ptr]);
	ptr+=4;
	unsigned long childsize=rev4(&data[ptr]);
	ptr+=4;

	ASSERT(childsize == 588);
	if (childsize != 588) return -1;

	// skip the nulls
	ptr=size_head;

	for (i=0;i<numchildren;i++)
	{
		iPod_pqed * e = new iPod_pqed;
		long ret = e->parse(&data[ptr]);
		if (ret < 0)
		{
			delete e;
			return ret;
		}

		eqList.push_back(e);
		ptr+=ret;
	}

	return ptr;
}


long iPod_mqed::write(unsigned char * data, const unsigned long datasize)
{
	unsigned long ptr=0;
	int i;

	//write mqed header
	data[0]='m';data[1]='q';data[2]='e';data[3]='d';
	ptr+=4;

	rev4(size_head,&data[ptr]);
	ptr+=4;
	rev4(unk1,&data[ptr]);
	ptr+=4;
	rev4(unk2,&data[ptr]);
	ptr+=4;

	rev4(GetChildrenCount(),&data[ptr]);
	ptr+=4;

	rev4(588,&data[ptr]);
	ptr+=4;

	// fill with nulls
	while (ptr<size_head)
	{
		data[ptr]=0; ptr++;
	}

	// write the eq settings
	for (i=0;i<GetChildrenCount();i++)
	{
		long ret=eqList[i]->write(&data[ptr],datasize-ptr);
		if (ret <0) return -1;
		ptr+=ret;
	}

	return ptr;
}



//////////////////////////////////////////////////////////////////////
// iPod_pqed - Class for parsing the EQ Entries
//////////////////////////////////////////////////////////////////////
iPod_pqed::iPod_pqed() :
	name(NULL),
	preamp(0)
{
	int i;
	for (i=0;i<10;i++)
		eq[i]=0;
	for (i=0;i<5;i++)
		short_eq[i]=0;
}

iPod_pqed::~iPod_pqed()
{
	if (name) delete [] name;
}

long iPod_pqed::parse(unsigned char * data)
{
	unsigned long ptr=0;
	int i;

	//check pqed header
	if (_strnicmp((char *)&data[ptr],"pqed",4)) return -1;
	ptr+=4;

	// get string length
	length=data[ptr]; ptr++;
	length+=data[ptr]*256; ptr++;

	name=new unsigned short[length + 1];
	memcpy(name,&data[ptr],length);
	name[length / 2] = '\0';

	// skip the nulls
	ptr=516;

	preamp = rev4(&data[ptr]);
	ptr+=4;

	unsigned long numbands;
	numbands = rev4(&data[ptr]);
	ptr+=4;

	ASSERT (numbands == 10);
	if (numbands != 10) return -1;

	for (i=0;i<numbands;i++)
	{
		eq[i]=rev4(&data[ptr]);
		ptr+=4;
	}

	numbands = rev4(&data[ptr]);
	ptr+=4;
	ASSERT (numbands == 5);
	if (numbands != 5) return -1;

	for (i=0;i<numbands;i++)
	{
		short_eq[i]=rev4(&data[ptr]);
		ptr+=4;
	}

	return ptr;
}

long iPod_pqed::write(unsigned char * data, const unsigned long datasize)
{
	long ptr=0;
	int i;

	//write pqed header
	data[0]='p';data[1]='q';data[2]='e';data[3]='d';
	ptr+=4;

	// write 2 byte string length
	data[ptr]=(length & 0xff); ptr++;
	data[ptr]=((length >> 8) & 0xff); ptr++;

	for (i=0;i<length;i++)
	{
		data[ptr]=name[i] & 0xff; ptr++;
		data[ptr]=(name[i] >> 8) & 0xff; ptr++;
	}

	// fill rest with nulls
	while (ptr<516)
	{
		data[ptr]=0;ptr++;
	}

	rev4(preamp,&data[ptr]);
	ptr+=4;

	rev4(10,&data[ptr]);
	ptr+=4;
	for (i=0;i<10;i++)
	{
		rev4(eq[i],&data[ptr]);
		ptr+=4;
	}

	rev4(5,&data[ptr]);
	ptr+=4;
	for (i=0;i<5;i++)
	{
		rev4(short_eq[i],&data[ptr]);
		ptr+=4;
	}

	return ptr;
}
