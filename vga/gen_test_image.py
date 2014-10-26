#!/usr/bin/python

import sys
import numpy
import scipy
import scipy.misc

# blue_base = []
# for i in xrange(8):
#     for j in xrange(2 ** i):
#         blue_base.append(0)
#     for j in xrange(2 ** i):
#         blue_base.append(255)

def main():
    # Create image
    width, height, step = map(int, sys.argv[1:])
    image = numpy.zeros((height, width, 3), dtype='uint8')
    row_bits = 14

    # Red
    red_row = numpy.linspace(0, 255, width)
    for i in xrange(height):
        image[i,:,0] = red_row

    # Blue
    for i in xrange(height):
        blue_row = [128] * step
        for j in xrange(row_bits):
            digit = (i >> (row_bits - j - 1)) & 1
            blue_row += [255 * digit] * step
        blue_row += [128] * step
        image[i,:len(blue_row),2] = blue_row

    # Save image
    scipy.misc.imsave('test.png', image)

if __name__ == '__main__':
    main()
