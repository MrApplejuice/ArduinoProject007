#!/usr/bin/python

"""
  Reveses the formatting of an image for debuggin purposes
"""

import sys
import struct

import numpy as np
import scipy.misc as sm

if len(sys.argv) < 3:
  print "2 arguments required: binary-arduino-file target-png-name"
  sys.exit(1)

imgData = ""
with open(sys.argv[1], "rb") as f:
  while True:
    newData = f.read(1024)
    imgData += newData
    if len(newData) < 1024:
      break

if len(imgData) != 128 * 64 / 8:
  raise ValueError("Input image has wrong size (is not 128 * 64 / 8)")

imgBytes = struct.unpack("B" * (128 * 64 / 8), imgData)

img = np.ndarray([64, 128], dtype=np.uint8)

byteIterator = iter(imgBytes)
for row in xrange(img.shape[0] / 8):
  for x in xrange(img.shape[1]):
    b = byteIterator.next()
    for i in xrange(8):
      #print hex(b), (b and (1 << i)) != 0
      img[row * 8 + i, x] = 0 if (b & (1 << i)) != 0 else 0xFF
      
sm.imsave(sys.argv[2], img)
