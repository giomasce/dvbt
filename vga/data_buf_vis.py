#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
from numpy.fft import fft, ifft
from pylab import *
import matplotlib.pyplot  as pyplot
import math
import cmath

def main():
    # Read input
    fin = open(sys.argv[1])
    length, sample_type = fin.readline().strip().split(" ")
    samples = map(float, fin.readline().strip().split(" "))
    if sample_type == 'signed_byte':
        samples = map(lambda x: x / 127.0, samples)
    freqs = fft(samples)
    freqs = freqs[:len(freqs)/2]
    yscale('log')
    plot(map(lambda x: abs(x)**2, freqs[:]))
    show()

if __name__ == '__main__':
    main()
