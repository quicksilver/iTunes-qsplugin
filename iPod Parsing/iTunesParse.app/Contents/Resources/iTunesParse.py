#!/usr/bin/env python
import struct

contents = open('iTunesDB','r').read()
file_pos = 0
file_len = len(contents)

code_names = {1:'Title',
			  2:'Filename',
			  3:'Album',
			  4:'Artist',
			  5:'Genre',
			  6:'Filetype',
			  7:'Equalizer',
			  8:'Comment',
			  12:'Composer',
			  13:'Grouping',
			  50:'Unknown',
			  51:'Unknown',
			  52:'Unknown',
			  53:'Unknown',
			  100:'Unknown'} # Seems to be some sort of info property

equalizer_presets = {100:'Acoustic',
					 101:'Bass Booster',
					 102:'Bass Reducer',
					 103:'Classical',
					 104:'Dance',
					 105:'Deep',
					 106:'Electronic',
					 107:'Flat',
					 108:'Hip-Hop',
					 109:'Jazz',
					 110:'Latin',
					 111:'Loudness',
					 112:'Lounge',
					 113:'Piano',
					 114:'Pop',
					 115:'R&B',
					 116:'Rock',
					 117:'Small Speakers',
					 118:'Spoken Word',
					 119:'Treble Booster',
					 120:'Treble Reducer',
					 121:'Vocal Booster'}

item_id = 0
item = {}
items = {}
prop_count = 0

while file_pos < file_len: # move through the file
	first_16_bytes = contents[file_pos:file_pos+16]
	second_16_bytes = contents[file_pos+16:file_pos+32]
	pad, code, jump, \
	myLen, count = struct.unpack("<2s2sLLL", first_16_bytes)
	
	if prop_count == 0 and item:
		items[item_id] = item
		item = {}
		item_id = 0
	if code == "bd": # start code
		print "%.8x Beginning of database" % file_pos
	
	elif code == "it": # a song list item
		item_id,x,x,x = struct.unpack("<LLLL", second_16_bytes)
		print "%.8x %i-property item (%i):" % (file_pos, count, item_id)
		# myLen is the total size of the item
		# count is the number of properties
		prop_count = count
	
	elif code == "sd": # a list starter
		# myLen indicates the size of the director2y
		# count is a typecode:
		if count == 1:
			print "%.8x List of Songs:" % file_pos
		elif count == 2:
			print "%.8x List of Playlists:" % file_pos
		else:
			print "%.8x Unknown (%i)" % (file_pos, count)
	
	elif code == "lt": # a list starter (tracks)
		# count is meaningless (usually 0)
		# myLen is the number of upcoming items
		print "%.8x %i-item list:" % (file_pos, myLen)
	
	elif code == "lp": # a list starter (playlists)
		# count is meaningless (usually 0)
		# myLen is the number of upcoming items
		print "%.8x %i-item list:" % (file_pos, myLen)
	
	elif code == "ip": # a playlist item
		# count is the number of properties (generally 1)
		x,x,item_ref,x,x = struct.unpack("<LLLHH",second_16_bytes)
		try:
			item = items[item_ref]
			print "%.8x: itemref (%i): %s-%s" % (file_pos+16, item_ref,
											 item.get('Artist',None),
											 repr(item.get('Title',None)))
		except KeyError:
			print "%.8x: itemref (%i) XXX" % (file_pos+16, item_ref)
	
	elif code == "yp": # a playlist
		# myLen indicates the size of the playlist
		# count is the number of properties, where the track list is 1 property
		item_id,x,x,x = struct.unpack("<LLLL", second_16_bytes)
		print "%.8x %i-property item (%i):" % (file_pos, count, item_id)
		prop_count = count
	
	elif code == "od": # a property - usually a unicode string
		# count is the item type.
		info,strlen,x,x = struct.unpack("<LLLL",contents[file_pos+jump:file_pos+jump+16])
		# info seems to be some sort of index for item types 100,
		# but not for others. For song items it's always 1, but it's random for playlist
		# properties
		if strlen != 0:
			fudge = 40 # jump is /always/ 24, so 24 + 16bytes = 40
			data =  unicode(contents[file_pos+fudge:file_pos+myLen],'utf-16-le',"x")
			if item_id:
				item[code_names[count]] = data
			if code_names[count] == "Equalizer":
				try:
					ind = int(data[3:6])
					print "%.8x: %10s: %s" % (file_pos,
											   code_names[count],
											   equalizer_presets[ind])
				except KeyError:
					print "%.8x: %10s: %s" % (file_pos,
											   code_names[count],data)
			elif code_names[count] != "Unknown":
				try:
					print "%.8x: %10s: %s" % (file_pos,
											   code_names[count],data)
				except UnicodeError:
					print "%.8x: %10s: %s" % (file_pos,
											   code_names[count],repr(data))
			else: # haven't figured this out yet
				if len(data) > 80:
					data = data[:77] + "..."
				print "%.8x: Unknown code: %s (%i) (%s)" % (file_pos, count, info,
															 repr(data))
		elif count == 100:
			if info == 0x1000000:
				print "%.8x:       Type: Smart" % file_pos
			elif info != 0:
				print "%.8x:   Revision: %i" % (file_pos, info)
		prop_count = prop_count - 1
		jump = myLen # skip to end of this string.
	
	else: # something mysterious and new
		print "%.8x: %s %6i %6i %6i" % (file_pos , repr(code),
										jump, myLen, count)
	file_pos = file_pos + jump
