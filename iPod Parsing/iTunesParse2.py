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
      7:'Unknown',
      8:'Comment',
      100:'Unknown'}

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
        # myLen indicates the size of the directory
        # count is a typecode:
        if count == 1:
            print "%.8x List of Songs:" % file_pos
        elif count == 2:
            print "%.8x List of Playlists:" % file_pos
        else:
            print "%.8x Unknown (%i)" % (file_pos, count)

    elif code == "lt": # a list starter
        # count is meaningless (usually 0)
        # myLen is the number of upcoming items
        print "%.8x %i-item list:" % (file_pos, myLen)

    elif code == "ip": # a playlist item
        x,x,item_ref,x,x = struct.unpack("<LLLHH",second_16_bytes)
        try:
            item = items[item_ref]
            print "%.8x: itemref (%i): %s-%s" % (file_pos+16, item_ref,
                                             item.get('Artist',None),
                                             repr(item.get('Title',None)))
        except KeyError:
            print "%.8x: itemref (%i) XXX" % (file_pos+16, item_ref)
                                             
    elif code == "od": # a unicode string, usually.
        # count is the item type.
        x,strlen,x,x = struct.unpack("<LLLL",contents[file_pos+jump:file_pos+jump+16])
        if strlen != 0:
            fudge = 40 # jump is /always/ 24, so 24 + 16bytes = 40
            data =  unicode(contents[file_pos+fudge:file_pos+myLen],'utf-16-le',"x")
            if item_id:
                item[code_names[count]] = data
            if code_names[count] != "Unknown":
                try:
                     print "%.8x: %10s: %s" % (file_pos,
                                               code_names[count],data)
                except UnicodeError:
                     print "%.8x: %10s: %s" % (file_pos,
                                               code_names[count],repr(data))
            else: # haven't figured this out yet
                print "%.8x: Unknown code: %s" % (file_pos, count)
            prop_count = prop_count - 1
        jump = myLen # skip to end of this string.

    else: # something mysterious and new
        print "%.8x: %s %6i %6i %6i" % (file_pos , repr(code),
                                        jump, myLen, count)
    file_pos = file_pos + jump
