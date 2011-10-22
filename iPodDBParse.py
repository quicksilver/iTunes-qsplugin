#!/usr/bin/env python

# Written by Kevin Ballard
# kevin@sb.org

import struct
import sys
import os
import base64
import types
import urllib

from xml.sax.saxutils import escape

if len(sys.argv) > 1:
	path = sys.argv[1]
	# now try various path manipulations
	# first check if the user passed a path to iTunesDB
	if os.path.basename(path) == 'iTunesDB':
		# yes, so path is all we need
		pass
	# now check for an iPod name
	elif os.path.dirname(path) == '' and os.path.exists(os.path.join('/Volumes', path)):
		path = os.path.join('/Volumes', path, 'iPod_Control', 'iTunes', 'iTunesDB')
	# Now check for a path that contains an iPod_Control/iTunes/iTunesDB child
	elif os.path.exists(os.path.join(path, 'iPod_Control', 'iTunes', 'iTunesDB')):
		path = os.path.join(path, 'iPod_Control', 'iTunes', 'iTunesDB')
	# Otherwise, we simply have a path to a database file
	else:
		pass
# Does the current dir have iTunesDB?
elif os.path.exists('iTunesDB'):
	path = 'iTunesDB'
# Ok, hunt for a connected iPod
else:
	import glob
	paths = glob.glob('/Volumes/*/iPod_Control/iTunes/iTunesDB')
	if len(paths) >= 1:
		path = paths[0]
	else:
		sys.stderr.write('Could not locate iPod database file\n')
		sys.exit(1)

try:
	contents = open(path, 'r').read()
except IOError:
	sys.stderr.write('Could not read iPod database file: %s\n' % path)
	sys.exit(1)
file_pos = 0
file_len = len(contents)

code_names = {1:'Name',
			  2:'Location',
			  3:'Album',
			  4:'Artist',
			  5:'Genre',
			  6:'Filetype',
			  7:'Equalizer',
			  8:'Comment',
			  12:'Composer',
			  13:'Grouping',
			  50:'Smart Info',
			  51:'Smart Criteria',
			  52:'Unknown',
			  53:'Unknown',
			  100:'Unknown'} # Seems to be some sort of info property

data_codes = [50, 51, 52, 53]

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
prop_count = 0
is_playlist = False

open_tags = []
def add_tag(level, tag):
	while len(open_tags) <= level: open_tags.append([])
	open_tags[level].append(tag)
def close_tags(level):
	while len(open_tags) > level:
		tag = open_tags.pop()
		if type(tag) is types.ListType:
			tag.reverse()
			for t in tag: close_tag(t, len(open_tags))
		else:
			close_tag(tag, len(open_tags))
def close_tag(tag, level):
	print "%s</%s>" % ("\t" * level, tag)

while file_pos < file_len: # move through the file
	first_16_bytes = contents[file_pos:file_pos+16]
	second_16_bytes = contents[file_pos+16:file_pos+32]
	pad, code, jump, \
	myLen, count = struct.unpack("<2s2sLLL", first_16_bytes)
	
	if code == "bd": # start code
		close_tags(0)
		print '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>'''
		add_tag(0, "plist")
		add_tag(0, "dict")

	elif code == "it": # a song list item
		close_tags(2)
		item_id,x,x,x = struct.unpack("<LLLL", second_16_bytes)
		print "\t\t<key>%i</key>\n\t\t<dict>" % item_id
		print "\t\t\t<key>Track ID</key><integer>%i</integer>" % item_id
		add_tag(2, "dict")
		# myLen is the total size of the item
		# count is the number of properties
		prop_count = count
		list_type = 0
	
	elif code == "sd": # a list starter
		# myLen indicates the size of the directory
		# count is a typecode:
		close_tags(1)
		if count == 1:
			print "\t<key>Tracks</key>\n\t<dict>"
			add_tag(1, "dict")
		elif count == 2:
			print "\t<key>Playlists</key>\n\t<array>"
			add_tag(1, "array")
		else:
			print >> sys.stderr, "%.8x Unknown list starter (%i)" % (file_pos, count)
	
	elif code == "lt": # a list starter (tracks)
		# count is meaningless (usually 0)
		# myLen is the number of upcoming items
		pass
	
	elif code == "lp": # a list starter (playlists)
		# count is meaningless (usually 0)
		# myLen is the number of upcoming items
		pass
	
	elif code == "ip": # a playlist item
		if not is_playlist:
			close_tags(3)
			print "\t\t\t<key>Playlist Items</key>\n\t\t\t<array>"
			add_tag(3, "array")
			is_playlist = True
		# count is the number of properties (generally 1)
		x,x,item_ref,x,x = struct.unpack("<LLLHH",second_16_bytes)
		print "\t\t\t\t<dict>\n\t\t\t\t\t<key>Track ID</key><integer>%i</integer>\n\t\t\t\t</dict>" % item_ref
	
	elif code == "yp": # a playlist
		close_tags(2)
		# myLen indicates the size of the playlist
		# count is the number of properties, where the track list is 1 property
		item_id,x,x,x = struct.unpack("<LLLL", second_16_bytes)
		print "\t\t<dict>"
		print "\t\t\t<key>Playlist ID</key><integer>%i</integer>" % item_id
		add_tag(2, "dict")
		prop_count = count
		is_playlist = False
	
	elif code == "od": # a property - usually a unicode string
		# count is the item type.
		info,strlen,x,x = struct.unpack("<LLLL",contents[file_pos+jump:file_pos+jump+16])
		# info seems to be some sort of index for item types 100,
		# but not for others. For song items it's always 1, but it's random for playlist
		# properties
		if strlen != 0:
			fudge = 40 # jump is /always/ 24, so 24 + 16bytes = 40
			data = contents[file_pos+fudge:file_pos+myLen]
			if count in data_codes:
				data = base64.encodestring(data)
			else:
				data = unicode(data,'utf-16-le',"ignore").encode('utf-8')
			if code_names[count] == "Equalizer":
				try:
					ind = int(data[3:6])
					print "\t\t\t<key>Equalizer</key><string>%s</string" % \
												equalizer_presets[ind]
				except KeyError:
					pass
			elif code_names[count] == "Filetype":
				# we don't want to do anything with this
				pass
			elif code_names[count] != "Unknown":
				if code_names[count] == "Location":
					# We need to turn this into a file:// URL
					filepath = os.path.abspath(path)
					for x in range(3): filepath = os.path.dirname(filepath)
					data = data.replace(":", "/")
					if data.startswith("/"): data = data[1:]
					filepath = os.path.join(filepath, data)
					data = "file://localhost%s" % urllib.pathname2url(filepath)
				if count in data_codes:
					data = "\n\t\t\t<data>\n\t\t\t%s\t\t\t</data>" % data
				else:
					data = "<string>%s</string>" % escape(data)
				print "\t\t\t<key>%s</key>%s" % (code_names[count], data)
			else: # haven't figured this out yet
				if len(data) > 80:
					data = data[:77] + "..."
				print >> sys.stderr, "%.8x: Unknown code: %s (%i) (%s)" % \
									(file_pos, count, info, repr(data))
		elif count == 100:
			if info == 0x1000000:
				#Smart playlist
				pass
			elif info != 0:
				#Revision property for playlist item
				pass
		prop_count = prop_count - 1
		jump = myLen # skip to end of this string.
	
	else: # something mysterious and new
		print >> sys.stderr, "%.8x: %s %6i %6i %6i" % \
						(file_pos , repr(code), jump, myLen, count)
	file_pos = file_pos + jump

close_tags(0)
