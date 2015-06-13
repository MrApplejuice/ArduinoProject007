#!/usr/bin/python

import sys
import struct

import numpy as np
import scipy.ndimage as ndi

if len(sys.argv) < 3:
  print "2 arguments required: source-image target-filename"
  sys.exit(1)

img = ndi.imread(sys.argv[1], flatten=True) # Loads images as 2 dimensional, grayscale image

# Check image format
if img.shape[0] == 128:
  raise ValueError("Image has wrong width")
if img.shape[1] == 64:
  raise ValueError("Image has wrong height")
 
# Make a binary image out of the grayscale image  
img = img < 128 # Note that black pixels must be set to become black on the LCD!

# Start output
with open(sys.argv[2], "wb") as f:
  # Segment image into rows of 8 pixels
  for row in xrange(img.shape[0] / 8):
    for x in xrange(img.shape[1]):
    
      # Construct bytes
      b = 0
      for i in xrange(8):
        b |= img[row * 8 + i, x] << i
        
      f.write(struct.pack("B", b)) # Encode byte in binary format
