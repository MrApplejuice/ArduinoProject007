#!/usr/bin/python
"""midi to arduino format converter

Convert a monophonic midi file in to a format which Paul and me can use to make an arduino speaker beep.

= Formats =
midi: a .mid file with only a single melody line. No polyphonics!
arduino: a list of (frequency, duration) tuples

= requirements =
python midi lib (https://github.com/vishnubob/python-midi)
"""
import midi
import sys
import math
import os
import json

import pdb


class midiToArduinoConverter:
    
    def __init__(self, midifile):
        self.midifile = midifile        
        
    def convert(self):
        pattern = midi.read_midifile(self.midifile)
        track = pattern[0]
        mylog("found {0} midi events in '{1}'".format(len(track),self.midifile))
        mylog("converting..")
        notes = []
        
        while len(track) > 1:
            note_and_silence = self.get_next_silence_and_note(track)
            # mylog("next note was {}".format(note))
            notes.extend(note_and_silence)
        
        mylog("conversion complete")
        return notes

    def calc_freq(self, note):
        """given an 88 key piano note number, return frequency.
        note 49 is the concert a (440 hz)
        """
        return math.pow(2,((note-49)/12.0)) * 440.0
        
    def get_next_event(self, track,event_type):
        event = None
        while len(track) > 0 and not event:
            candidate = track.pop(0)
            if isinstance(candidate,event_type):
                event = candidate
                    
        return event
        
    def get_next_silence_and_note(self, track):
        """find NoteOn and NoteOff events to create a single tone with freq and duration.
        Also return silence before this note a 0 freq item
        """
        #find note on
        on_event = self.get_next_event(track,midi.NoteOnEvent)
        
        #find note off
        off_event = self.get_next_event(track,midi.NoteOffEvent)
        
        #check 
        if on_event.data[0] != off_event.data[0]:
            raise midiToArduinoException("polyphonics detected. Polyphonics is Satan. Begon ye! NoteOn was {0}, NoteOff was {1}. track: {2}".format(on_event.data[0],off_event.data[0],track[0:30]))
        
        
        arduinolist = []
        #calculate silence before note
        silence = on_event.tick
        if silence > 0:
            arduinolist.append((0,silence))
        
        #calc note
        freq = self.calc_freq(on_event.data[0])
        duration = off_event.tick
        arduinolist.append((freq,duration))
        return arduinolist
    
    def get_silence(self,track):
        """return the silence before the current note. Silence is encoded as a 0 Hz signal.
        """
        event = track[0]
        if isinstance(event,midi.NoteOnEvent):
            silence = event.tick
        else:
            raise midiToArduinoException("Expected to find midi.NoteOnEvent but instead found {}. Midi files should have strictly consecutive notes, no overlap!".format(event.__class__))
        
        if silence > 0:
            return (0,silence)
        else:
            return None
        
    
def mylog(msg):
    print("* " + msg)
    
def write_to_file(content,path):
    f = open(path,"w")
    f.write(content)
    f.close()
    
    
   
class midiToArduinoException(Exception):
    pass
    
if len(sys.argv) != 2:
    print "Usage: {0} <midifile>".format(sys.argv[0])
    sys.exit(2)
    
midifile = sys.argv[1]

conv = midiToArduinoConverter(midifile)
notes = conv.convert()
outputfile = os.path.splitext(midifile)[0] + ".json"
write_to_file(json.dumps(notes),outputfile)
mylog("Wrote notes to {}".format(outputfile))





