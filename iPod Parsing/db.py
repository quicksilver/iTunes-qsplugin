import struct, sys, os, time, codecs
(utf_encode, utf_decode, sr, sw) = codecs.lookup('utf-16-le')

global mhit
mhit = {}

class mh_record:
    def __init__(self, type, hdr_len, hdr, offset, db, level):
        self.type = type
        self.hdr_len = hdr_len
        self.hdr = hdr
        self.data_start = offset
        self.db = db
        self.level = level
        
    def read_record(self):
        f = {'mhbd' : mhbd_record,
             'mhsd' : mhsd_record,
             'mhit' : mhit_record,
             'mhod' : mhod_record,
             'mhlt' : mhlt_record,
             'mhlp' : mhlp_record,
             'mhyp' : mhyp_record,
             'mhip' : mhip_record}

        format = '<4sL'
        size = struct.calcsize(format)
        hdr = db.read(size)
        (type, hdr_len) = struct.unpack(format, hdr)
        hdr = db.read(hdr_len-size)
        data_start = db.tell()

        # print '%sinstancing %s' % ('\t'*self.level, type)
        new_rec = f[type](type, hdr_len, hdr, data_start, db, self.level+1)
        return new_rec


    def fill_in(self, nb_rec):
        self.record = []
        self.db.seek(self.data_start)
        for i in range(nb_rec):
            # print '%s %s- rec %d/%d' % ('\t'*self.level, self.type, i, nb_rec)
            self.record.append(self.read_record())
        
        
class mhbd_record(mh_record):
    def __init__(self, type, hdr_len, hdr, offset, db, level):
        mh_record.__init__(self, type, hdr_len, hdr, offset, db, level)
        self.format = '<LLLL80s'
        (tmp, u1, u2, u3, u4) = struct.unpack(self.format, self.hdr)

        if (u1 != 1) or (u2 != 1) or (u3 != 2) or (u4 != '\x00' * 80):
            raise 'Strange mhbd_record'
        
        self.data_size = tmp - hdr_len
        self.fill_in(2)
            
        
class mhsd_record(mh_record):
    def __init__(self, type, hdr_len, hdr, offset, db, level):
        mh_record.__init__(self, type, hdr_len, hdr, offset, db, level)
        self.format = '<LL80s'
        (tmp, idx, u1) = struct.unpack(self.format, self.hdr)

        if (u1 != '\x00' * 80):
            raise 'Strange mhsd_record'
        
        self.data_size = tmp - hdr_len
        self.fill_in(1)

class mhlt_record(mh_record):
    def __init__(self, type, hdr_len, hdr, offset, db, level):
        mh_record.__init__(self, type, hdr_len, hdr, offset, db, level)
        self.format = '<L80s'
        (nb, u1) = struct.unpack(self.format, self.hdr)

        if (u1 != '\x00' * 80):
            raise 'Strange mhlt_record'
        
        self.data_size = 0
        self.fill_in(nb)

class mhit_record(mh_record):
    def __init__(self, type, hdr_len, hdr, offset, db, level):
        mh_record.__init__(self, type, hdr_len, hdr, offset, db, level)
        self.format = '<LLLLLLLLLLLLLLLLL80s'
        (tmp, nb, self.key, u2, u3, self.u4, self.file_date, self.file_size, self.file_duration, self.file_order,
         self.u9, self.u10, self.u11, self.u12, u13, u14, u15, u16) = struct.unpack(self.format, self.hdr)

        if (u16 != '\x00' * 80) or u13!=0 or u14!=0 or u15!=0 or u2!=1 or u3!=0 or (self.u4!=0x0101 and self.u4!=0x0100):
            hexdump(hdr)
            raise 'Strange mhit_record u(13,14,15,2,3,4) %08X %08X %08X %08X %08X %08X ' % (u13, u14, u15, u2, u3, u4)

        if mhit.has_key(self.key):
            raise 'Duplicate mhit key'
        mhit[self.key] = self

        self.data_size = tmp - hdr_len
        self.fill_in(nb)

    def display(self):
        sec = self.file_duration/1000.0
        m = int (sec/60)
        s = int (sec%60)
        print 'mhit record %08X : (%02d:%02d) %s (type : %08X)' % (self.key, m, s, self.record[0].name.encode('latin-1','replace'), self.u4)
        
    def display_full(self):
        sec = self.file_duration/1000.0
        m = int (sec/60)
        s = int (sec%60)
        print 'mhit record %08X : (%02d:%02d) %s (type : %08X)' % (self.key, m, s, self.record[0].name.encode('latin-1','replace'), self.u4)
        print '\tFile size : %3.3f MB' % (self.file_size / (1024.0*1024.0))
        date_offset = long(time.mktime((1904, 01, 01, 00, 00, 00, 0, 0, 0)))+time.timezone
        print '\tFile date : %s' % (time.strftime('%d/%m/%Y %H:%M:%S', time.localtime(self.file_date+date_offset)))
        for r in self.record:
            print '\t\t%s' % (r.name.encode('latin-1','replace'))
            

class mhod_record(mh_record):
    def __init__(self, type, hdr_len, hdr, offset, db, level):
        mh_record.__init__(self, type, hdr_len, hdr, offset, db, level)

        self.format = '<LLLL'
        (tmp, type,  u1, u2) = struct.unpack(self.format, self.hdr)

        self.data_size = tmp - hdr_len
        self.data = self.db.read(self.data_size)

        if type == 0x64:
            pass
        #    print "MHOD Empty Record type %d, len %d : u(1-2) %08X %08X" % (type, self.data_size, u1, u2)
        else:
            (u1, self.utf_name_len, u2, u3) = struct.unpack('<LLLL', self.data[:16])
            (self.name, self.name_len) = utf_decode(self.data[16:])
                    
class mhlp_record(mh_record):
    def __init__(self, type, hdr_len, hdr, offset, db, level):
        mh_record.__init__(self, type, hdr_len, hdr, offset, db, level)
        self.format = '<L80s'
        (self.nb, u1) = struct.unpack(self.format, self.hdr)

        if (u1 != '\x00' * 80):
            raise 'Strange mhlp_record'
#

        
        self.data_size = 0
        self.fill_in(self.nb)

    def display(self):
        print 'mhlp with %d lists:' % (self.nb)
        for i in range(self.nb):
            print "\t%04d" % (i), self.record[i].name()
        

class mhyp_record(mh_record):
    def __init__(self, type, hdr_len, hdr, offset, db, level):
        mh_record.__init__(self, type, hdr_len, hdr, offset, db, level)
        self.format = '<LLLLLLLLL64s'
        (tmp, self.subtype, self.nb, u1, u2, u3, u4, u5, u6, u7) = struct.unpack(self.format, self.hdr)

        if (u7 != '\x00' * 64):
            raise 'Strange mhyp_record'

        self.data_size = tmp - hdr_len
        self.fill_in(2+self.nb*2)
        
        if db.tell() != (offset-hdr_len+tmp):
            print 'Warning : mhyp end not reached'
        db.seek(offset-hdr_len+tmp)

    def name(self):
        return self.record[1].name    

    def display(self):
        # rec 0 is mhod with play list's name
        # rec 1 is empty mhod ???
        # after, it's 
        print 'mhyp sub %d : %s' % (self.subtype, self.record[1].name)
        for i in range(self.nb):
            print "\t%04d" % (i),
            mhit[self.record[2+2*i].ptr].display()

class mhip_record(mh_record):
    def __init__(self, type, hdr_len, hdr, offset, db, level):
        mh_record.__init__(self, type, hdr_len, hdr, offset, db, level)
        self.format = '<LLLLLL44s'
        (tmp, self.u1, self.u2, self.key, self.ptr, self.u3, self.u4) = struct.unpack(self.format, self.hdr)

        self.data_size = tmp - hdr_len
        self.data = self.db.read(self.data_size)



db = open ('iTunesDB','rb')
hd = mh_record('', 0, '', 0, db, 0)
root = hd.read_record()
db.close()

# first song
root.record[0].record[0].record[0].display_full()

# master playlist
#root.record[1].record[0].record[0].display()

# First user playlist
root.record[1].record[0].record[1].display()

        

    

