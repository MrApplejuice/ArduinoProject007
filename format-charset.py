#!/usr/bin/python

import sys
import struct

import numpy as np
import scipy.ndimage as ndi

if len(sys.argv) < 3:
  print "1 argument required: source-image charmap"
  sys.exit(1)

img = ndi.imread(sys.argv[1], flatten=True) # Loads images as 2 dimensional, grayscale image

# Check image format
if img.shape[1] == 8:
  raise ValueError("Image has wrong height")
 
# Make a binary image out of the grayscale image  
img = img < 128 # Note that black pixels must be set to become black on the LCD!

# Display was mounted upside down, rotate images by 180 degreees
img = np.fliplr(np.flipud(img))

# Start output
encodedImage = []
offsets = [0]
for x in xrange(img.shape[1]):
  # Construct bytes
  b = 0
  for i in xrange(8):
    b |= img[i, x] << i
  
  if b == 0:
    if offsets[-1] != len(encodedImage):
      offsets.append(len(encodedImage))
  else:
    encodedImage.append("0x" + format(b & 0xFE, 'x'))
  
if offsets[-1] != len(encodedImage):
  offsets.append(len(encodedImage))

if len(sys.argv[2]) != len(offsets) - 1:
  print "Warning: number of offsets does not correspond to the number of translated images - check results"

rCharmap = list(reversed(sys.argv[2]))
def findInCharmap(c):
  global rCharmap
  if c in rCharmap:
    return rCharmap.index(c)
  else:
    return -1

print "{", ", ".join(encodedImage), "}"
print "{", ", ".join(map(str, offsets)), "}"
print "{", ", ".join([str(findInCharmap(chr(i))) for i in xrange(0x100)]), "}"

