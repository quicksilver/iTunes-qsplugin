# itunesdb.tcl
#
# Tcl library for reading iTunesDB files.
#
# Copyright (C) 2004 Andy Goth <unununium@openverse.com>
# For more information visit <URL:http://ioioio.net/devel/itunesdb/>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# See the file README for information on how to get the full text of the GNU
# General Public License.

package require snit     0.97
package provide itunesdb 0.04

snit::type ::itunesdb_impl {
    pragma -hastypemethods no
    pragma -simpledispatch yes

    variable path           ;# Name of iTunesDB file.
    variable chan           ;# Tcl channel for reading the file.

    variable track_pos      ;# File position at which first track resides.
    variable track_num      ;# Total number of tracks in the file.

    variable plist_pos      ;# File position at which first playlist resides.
    variable plist_num      ;# Total number of playlists in the file.

    variable constructed 0  ;# If true, the constructor has completed.

    # itunesdb $filename
    #
    # Opens an iTunesDB database file.
    #
    # $filename - Path to the iTunesDB file.
    #
    # return - Command through which further access is conducted ($db).
    constructor {filename} {
        # Open the iTunesDB file.
        set path $filename
        set chan [open $path r]

        # Treat it as a raw binary.  Do no translations whatsoever.
        fconfigure $chan -translation binary

        # Load file summary information.
        $self read summary

        # Job's done.
        set constructed 1
    }

    # $db destroy
    #
    # Closes the database file.
    destructor {
        if {[info exists chan]} {
            close $chan
        }
    }

    # $db read $which [[$array] $script]
    #
    # Reads records from the database file.  All records of the given type
    # ($which) are processed.  If $script is specified, it is executed for each
    # record.  If $array is also specified, then it is used within the $script
    # as the name of an array variable containing all fields of the record;
    # otherwise, each field is stored in a variable of the same name.
    #
    # $which - One of "summary", "tracks", and "playlists".
    #     summary   - Gathers summary information.  Only one record.
    #     tracks    - Gets all track information, one track per record.
    #     playlists - Gets all playlists, one playlist per record.
    #
    # return - If $script is not specified, all requested data.  If $script is
    #          specified, nothing is returned.
    #
    # error - 
    #     "file is not an iTunes DB" - Incorrect format on first read.
    #
    # TODO: Add a mode for returning a single record at a time.
    method read {which args} {
        # Grab the optional arguments.
        switch -- [llength $args] {
        0 {
            set have_script 0
        } 1 {
            set have_script 1
            set have_array  0
            set array  ""
            set script [lindex $args 0]
        } 2 {
            set have_script 1
            set have_array  1
            set array  [lindex $args 0]
            set script [lindex $args 1]
        } default {
            error "wrong # args: should be \$db read \$which ?\$array?\
                    ?\$script?"
        }}
        
        # If a script is given, turn it into a proc.
        if {$have_script} {
            set body [list upvar 1 record $array]
            if {!$have_array} {
                append body {
                    # Create variables from contents of array.
                    foreach {key val} [array get ""] {
                        set $key $val
                    }
                }
            }
            append body \n$script
            proc ${selfns}::Script {} $body
        }

        # Eventually, $result will be returned.
        set result [list]

        # Set up initial state.
        switch -- $which {
        summary {
            set pos       0
            set state     header
            set track_num 0
            set plist_num 0
            unset -nocomplain track_pos plist_pos
        } tracks {
            if {$track_num == 0} {
                return [list]
            } else {
                set pos   $track_pos
                set state track_data
                set index 0
            }
        } playlists {
            if {$plist_num == 0} {
                return [list]
            } else {
                set pos   $plist_pos
                set state plist_data
                set index 0
            }
        } default {
            error "invalid \$which \"$which\": should be \"summary\",\
                    \"tracks\", or \"playlists\""
        }}

        # Main loop; runs once per block.
        while {1} {
            # Read all bytes for this block.
            if {[catch {
                if {$pos eq "here"} {
                    set offset [tell $chan]
                } else {
                    seek $chan $pos
                    set offset $pos
                }
                set buffer [GetBlock $chan]
                if {$buffer eq ""} {
                    if {[eof $chan] && $which eq "summary"} {
                        set header "end-of-file"
                    } else {
                        error "end of file"
                    }
                } else {
                    set header [GetHdr $buffer 0]
                }
            } err]} {
                if {$constructed} {
                    error $err $::errorInfo
                } else {
                    error "file is not an iTunes DB"
                }
            }

            # Default next position is wherever the file pointer landed.
            set pos here

            # Process this block.
            switch -- [catch {switch -glob -- "$state $header" {
            "header mhbd" {
                # Found initial header.  Skip ahead to mhsd.
                set state aggregate
            } "header *" {
                # Bad bits at the beginning of the file.
                error "file is not an iTunes DB"
            } "aggregate mhsd" {
                # Top-level aggregate block.
                set next_mhsd [expr {$offset + [GetInt $buffer 8]}]
                switch -- [GetInt $buffer 12] {
                1 {
                    # Track list.
                    if {[info exists track_pos]} {
                        error "extra track list section"
                    }
                    set state track_header
                } 2 {
                    # Playlists.
                    if {[info exists plist_pos]} {
                        error "extra playlist section"
                    }
                    set state plist_header
                } default {
                    # Neither track list nor playlist.  Skip.
                    set pos $next_mhsd
                }}
            } "aggregate end-of-file" {
                # End of file.
                break
            } "track_header mhlt" {
                # Beginning of track list.
                set track_pos [expr {$offset + [GetInt $buffer 4]}]
                set track_num [GetInt $buffer 8]
                set pos       $next_mhsd
                set state     aggregate
            } "plist_header mhlp" {
                # Beginning of playlists.
                set plist_pos [expr {$offset + [GetInt $buffer 4]}]
                set plist_num [GetInt $buffer 8]
                set pos       $next_mhsd
                set state     aggregate
            }

            "track_data mhit" {
                # Track entry, numeric portion.
                array unset record *

                set fld_num [GetInt $buffer 12]
                set record(id) [GetInt $buffer 16]

                # Extract all numeric fields.
                foreach {field offset} {
                    rating   28     last_mod   32     file_size   36
                    duration 40     track_num  44     track_count 48
                    year     52     bit_rate   56     samp_rate   60
                    volume   64     play_count 80     last_play   88
                    cd_num   92     cd_count   96     add_time   104
                    bpm     120
                } {
                    set val [GetInt $buffer $offset]

                    switch -- $field {
                    rating {
                        # TODO: This needs more research.
                        set val [expr {$val >> 24 / 20}]
                    } last_mod - last_play - add_time {
                        # Fix epoch.
                        if {$val == 0} {
                            set val never
                        } else {
                            incr val -2082844800
                        }
                    } samp_rate - bpm {
                        # TODO: How are samp_rate values above 65535 handled?
                        set val [expr {($val >> 16) & 0xffff}]
                    } volume {
                        # Change to a scale of -100.0 through +100.0.
                        set val [expr {$val / 2.55}]
                        if {$val < -100 || $val > 100} {
                            set val 0
                        }
                    }}

                    set record($field) $val
                }

                if {$fld_num == 0} {
                    # No text fields to read, so quit now.
                    break
                } else {
                    # Expect text fields (mhod).
                    set fld_idx 0
                    set state   track_data_text
                }
            } "track_data_text mhod" {
                # Track entry, textual portion.

                # Determine field type.
                switch -- [GetInt $buffer 12] {
                 1 {set field title    }  2 {set field ipod_path}
                 3 {set field album    }  4 {set field artist   }
                 5 {set field genre    }  6 {set field type     }
                 7 {set field equalizer}  8 {set field comment  }
                12 {set field composer } 13 {set field group    }
                                    default {set field unknown  }
                }

                if {$field ne "unknown"} {
                    # Read textual data.
                    set record($field) [GetUtf16 $buffer 40 [GetInt $buffer 28]]
                }

                incr fld_idx
                if {$fld_idx == $fld_num} {
                    # No more text fields to read.  Move on to the next record.
                    break
                }
            }
                
            "plist_data mhyp" {
                # Playlist, numeric portion.
                array unset record *

                set next_mhyp [expr {$offset + [GetInt $buffer 8]}]
                set trk_num   [GetInt $buffer 16]

                # Calculate default playlist name.
                if {([GetInt $buffer 20] & 255) == 1} {
                    set record(id)   Master-PL
                    set record(type) master
                } else {
                    set record(id)   Playlist
                    set record(type) custom
                }
                set record(id) [format "%s (%08x)" $record(id) $offset]

                # Expect text fields (mhod) or playlist entries (mhip).
                set trk_idx 0
                set state   plist_data_entry
            } "plist_data_entry mhod" {
                # Playlist, textual portion.

                # I assume the title mhod appears somewhere before the final
                # mhip entry--- if not, oh well, the defaults are fine. :^)
                if {[GetInt $buffer 12] == 1} {
                    # Playlist title.
                    set record(id) [GetUtf16 $buffer 40 [GetInt $buffer 28]]
                } else {
                    # Unknown mhod type (most likely 100).  Skip.
                }
            } "plist_data_entry mhip" {
                # Playlist, track entry.
                lappend record(tracks) [GetInt $buffer 24]

                incr trk_idx
                if {$trk_idx == $trk_num} {
                    # This was the last track in the playlist.
                    break
                }
            }

            default {
                error "unexpected section \"$header\""
            }}} err] {
            0 {
                # TCL_OK: Success.  Go to next block for more of the record.
            } 1 {
                # TCL_ERROR: Failure.  We lose.
                error $err $::errorInfo
            } 3 {
                # TCL_BREAK: Success.  The record is complete.
                if {$which eq "summary"} {
                    array set record [list          \
                        filename       $path        \
                        track_count    $track_num   \
                        playlist_count $plist_num   \
                    ]
                }

                # Process the record.
                if {$have_script} {
                    ${selfns}::Script
                } else {
                    lappend result [array get record]
                }

                # Is there more to do, or have we processed the last record?
                switch -- $which {
                summary {
                    break
                } tracks {
                    incr index
                    if {$index == $track_num} {
                        break
                    } else {
                        set state track_data
                    }
                } playlists {
                    incr index
                    if {$index == $plist_num} {
                        break
                    } else {
                        # Skip any extra mhod's in this section.
                        set pos $next_mhyp
                        set state plist_data
                    }
                }}
            }}
        }

        return $result
    }

    # GetBlock $chan
    #
    # TODO: Finish documenting this.
    proc GetBlock {chan} {
        if {[catch {
            set buffer [read $chan 12]
            set header [GetHdr $buffer 0]

            if {$header eq "mhod" || $header eq "mhip"} {
                set size [GetInt $buffer 8]
            } else {
                set size [GetInt $buffer 4]
            }

            if {$size <= 12} {
                error "bad block size"
            }

            append buffer [read $chan [expr {$size - 12}]]
        } err]} {
            if {[eof $chan]} {
                set buffer ""
            } else {
                error $err $::errorInfo
            }
        }

        return $buffer
    }

    # Get$type $buffer $offset [$length]
    #
    # Gets numeric or textual data from a buffer read from the database file.
    #
    # (Internal)
    #
    # $type - Type of data to get from the buffer.  Can be one of the following:
    #     Byte  -  8-bit signed value         (numeric) *UNIMPLEMENTED*
    #     Short - 16-bit signed value         (numeric) *UNIMPLEMENTED*
    #     Int   - 32-bit signed value         (numeric) (default)
    #     Long  - 64-bit signed value         (numeric) *UNIMPLEMENTED*
    #     Hdr   - four-byte header identifier (numeric)
    #     Bin   - binary data                 (textual) *UNIMPLEMENTED*
    #     Utf16 - UTF-16 data                 (textual)
    # $buffer - The byte buffer from which to extract data.
    # $offset - Offset relative to the start of $buffer at which to get data.
    # $length - Number of bytes to process.  Only valid with textual $type.
    #
    # return - The data extracted from the buffer.
    # error - "end of file" - Attempt to read past the end of the buffer.
    #
    # Full list of formats; use this to add support for the other types.  Just
    # use it as the second paramter to the foreach below.
    #
    #   byte  c       num     short s       num
    #   int   i       num     long  w       num
    #   hdr   a4      num     bin   binary  txt
    #   utf16 unicode txt
    foreach {type fmt mode} {
        int i num       hdr a4 num       utf16 unicode txt
    } {
        set name    Get[string totitle $type]
        set formals [list buffer offset]

        if {$mode eq "txt"} {
            lappend formals length

            set enc $fmt
            set fmt a\$length
        }

        # Extract the formatted data from the buffer.
        set body [string map [list %FMT% $fmt] {
            if {[binary scan $buffer @${offset}%FMT% data] == 0} {
                error "end of file"
            }
        }]

        # Fix silly byte-ordering problem.
        if {$mode eq "txt" && $enc eq "unicode" &&
        $tcl_platform(byteOrder) eq "bigEndian"} {
            append body {
                binary scan $data s* data
                set data [binary format S* $data]
            }
        }

        # Return the data.
        if {$mode eq "txt" && $enc ne "binary"} {
            # Decode the byte array into a string.
            append body [string map [list %ENC% $enc] {
                return [encoding convertfrom %ENC% $data]
            }]
        } else {
            append body {
                return $data
            }
        }

        proc $name $formals $body
    }
}

interp alias "" itunesdb "" itunesdb_impl %AUTO%

# vim: set ts=4 sts=4 sw=4 tw=80 et:
