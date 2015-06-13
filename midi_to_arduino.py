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
import pdb


class midiToArduinoConverter:
    
    def __init__(self, midifile):
        self.midifile = midifile        
        
    def get_arduino_notes(self):
        pattern = midi.read_midifile(self.midifile)
        track = pattern[0]
        mylog("found {0} midi events in '{1}'".format(len(track),self.midifile))

        notes = []
        while len(track) > 1:
            note = self.get_next_note(track)
            mylog("next note was {}".format(note))
            notes.append(note)
        
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
        
    def get_next_note(self, track):
        """find NoteOn and NoteOff events to create a single tone with freq and duration
        """
        #find note on
        on_event = self.get_next_event(track,midi.NoteOnEvent)
        
        #find note off
        off_event = self.get_next_event(track,midi.NoteOffEvent)
        
        #check 
        if on_event.data[0] != off_event.data[0]:
            raise midiToArduinoException("polyphonics detected. Polyphonics is Satan. Begon ye! NoteOn was {0}, NoteOff was {1}".format(on_event.data[0],off_event.data[0]))
        
        #calc note
        freq = self.calc_freq(on_event.data[0])
        duration = off_event.tick
        return (freq,duration)
    
def mylog(msg):
    print("* " + msg)
   
class midiToArduinoException(Exception):
    pass
    
if len(sys.argv) != 2:
    print "Usage: {0} <midifile>".format(sys.argv[0])
    sys.exit(2)
    
midifile = sys.argv[1]

conv = midiToArduinoConverter(midifile)
conv.get_arduino_notes()    




