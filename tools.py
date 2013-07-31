#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
from numpy.fft import fft, ifft
from pylab import *
import random
import math
import cmath
import heapq

CONST_QAM16 = [
    [(1, 0, 0, 0), (1, 0, 1, 0), (0, 0, 1, 0), (0, 0, 0, 0)],
    [(1, 0, 0, 1), (1, 0, 1, 1), (0, 0, 1, 1), (0, 0, 0, 1)],
    [(1, 1, 0, 1), (1, 1, 1, 1), (0, 1, 1, 1), (0, 1, 0, 1)],
    [(1, 1, 0, 0), (1, 1, 1, 0), (0, 1, 1, 0), (0, 1, 0, 0)]
]
CONST_QAM16_LOOKUP = {}
for i in xrange(4):
    for j in xrange(4):
        CONST_QAM16_LOOKUP[CONST_QAM16[i][j]] = -3 + 2*i + (-3 + 2*j)*1j

CONTINUAL_PILOTS_2K = [
    0,    48,   54,   87,   141,  156,  192,
    201,  255,  279,  282,  333,  432,  450,
    483,  525,  531,  618,  636,  714,  759,
    765,  780,  804,  873,  888,  918,  939,
    942,  969,  984,  1050, 1101, 1107, 1110,
    1137, 1140, 1146, 1206, 1269, 1323, 1377,
    1491, 1683, 1704]

TPS_CARRIERS_2K = [
    34,   50,   209,  346,  413,
    569,  595,  688,  790,  901,
    1073, 1219, 1262, 1286, 1469,
    1594, 1687]

#base_freq = 25.71e6
base_freq = 5760.0 / 224e-6
signal_freq = 76.5e6
packet_time = 224e-6
guard_time = packet_time / 32
carriers_num = 1705
central_carrier = (carriers_num - 1) / 2
#bandwidth = float(carriers_num) / packet_time

packet_len = int(signal_freq * packet_time)
guard_len = int(signal_freq * guard_time)
central_idx = int(base_freq * packet_time)
lower_idx = central_idx - central_carrier
higher_idx = central_idx + central_carrier

#print central_idx, base_freq * packet_time

class PRBS:

    def __init__(self, init, taps):
        self.status = init
        self.taps = taps
        self.length = len(init)

    def generate(self):
        res = self.status[self.length - 1]
        self.status = [sum([self.status[i] for i in self.taps]) % 2] + self.status[:-1]
        return res

def test_PRBS():
    prbs = PRBS([1 for x in xrange(11)], [8, 10])
    res = []
    for x in xrange(200):
        res.append(prbs.generate())
    print res

pilots_prbs = PRBS([1 for x in xrange(11)], [8, 10])
pilots_sequence = []
for x in xrange(carriers_num):
    pilots_sequence.append(pilots_prbs.generate())

class ContinuousFile:

    def __init__(self, buf):
        self.buf = buf
        self.pos = 0

    def read(self, n):
        res = []
        while len(res) < n:
            new_res = self.buf[self.pos:self.pos + n - len(res)]
            res += new_res
            self.pos += len(new_res)
            if self.pos == len(self.buf):
                self.pos = 0
        return res

class SlidingWindow:

    def __init__(self, fin):
        self.fin = fin
        self.buf = []
        self.offset = 0
        self.total_offset = 0

    def __getitem__(self, n):
        real_n = n + self.offset
        if 0 <= real_n and real_n < len(self.buf):
            return self.buf[real_n]
        else:
            raise IndexError("Index out of range")

    def __getslice__(self, n, m):
        real_n = self.offset + n
        real_m = self.offset + m
        if 0 <= real_n and real_n < len(self.buf) and \
                0 <= real_m and real_m < len(self.buf):
            return self.buf[real_n:real_m]
        else:
            raise IndexError("Index out of range")

    def reserve_front(self, n):
        if len(self.buf) - self.offset < n:
            self.buf += self.fin.read(n - (len(self.buf) - self.offset))

    def reserve_back(self, n):
        if self.offset > n:
            self.buf = self.buf[self.offset - n:]
            self.offset = n
        else:
            raise Exception("Cannot reserve %d on back (%d bytes available)" % (n, self.offset))

    def advance(self, n):
        if self.offset + n > len(self.buf):
            self.buf += self.fin.read(self.offset + n - len(self.buf))
        if -n > self.offset:
            raise Exception("Cannot return of %d in the past (%d bytes available)" % (-n, self.offset))
        self.total_offset += n
        self.offset += n

    # TODO - Implement this more efficiently (with only one read call
    # and list concatenation)
    def advance_and_reserve_front(self, n, m):
        self.advance(n)
        self.reserve_front(m)

class TPSDecoder:

    SYNC_WORD_ODD = [0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0]
    SYNC_WORD_EVEN = [1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1]

    LENGTH_VALUES = {
        (0, 1, 0, 1, 1, 1): 23,
        (0, 1, 1, 1, 1, 1): 31
    }

    FRAME_NUMBER_VALUES = {
        (0, 0): 1,
        (0, 1): 2,
        (1, 0): 3,
        (1, 1): 4
    }

    CONSTELLATION_VALUES = {
        (0, 0): 'QPSK',
        (0, 1): '16-QAM',
        (1, 0): '64-QAM'
    }

    HIERARCHY_VALUES = {
        (0, 0, 0): 'non hierarchical',
        (0, 0, 1): '1',
        (0, 1, 0): '2',
        (0, 1, 1): '4'
    }

    CODE_RATE_VALUES = {
        (0, 0, 0): '1/2',
        (0, 0, 1): '2/3',
        (0, 1, 0): '3/4',
        (0, 1, 1): '5/6',
        (1, 0, 0): '7/8'
    }

    GUARD_INTERVAL_VALUES = {
        (0, 0): '1/32',
        (0, 1): '1/16',
        (1, 0): '1/8',
        (1, 1): '1/4'
    }

    TRANSMISSION_MODE_VALUES = {
        (0, 0): '2K',
        (0, 1): '8K'
    }

    def __init__(self):
        self.buf = []
        self.synced = False
        self.odd_packet = None

    def push_bit(self, bit):
        self.buf.append(bit)

        # Not synced: check for synchronization word
        if not self.synced:
            self.buf = self.buf[-17:]
            if len(self.buf) == 17:
                if self.buf[1:] == TPSDecoder.SYNC_WORD_ODD:
                    self.synced = True
                    self.odd_packet = True
                if self.buf[1:] == TPSDecoder.SYNC_WORD_EVEN:
                    self.synced = True
                    self.odd_packet = False

        # Synced: check whether packet is complete
        else:
            if len(self.buf) == 68:
                data = self.interpret()
                self.buf = []
                self.odd_packet = not self.odd_packet
                return data

        #print self.buf, self.synced, self.odd_packet

    def interpret(self):
        data = {}

        # Check again synchronization word
        if self.odd_packet:
            assert self.buf[1:17] == TPSDecoder.SYNC_WORD_ODD
            data['parity'] = 'odd'
        else:
            assert self.buf[1:17] == TPSDecoder.SYNC_WORD_EVEN
            data['parity'] = 'even'

        # Decode fields
        data['length'] = TPSDecoder.LENGTH_VALUES[tuple(self.buf[17:23])]
        data['frame_number'] = TPSDecoder.FRAME_NUMBER_VALUES[tuple(self.buf[23:25])]
        data['constellation'] = TPSDecoder.CONSTELLATION_VALUES[tuple(self.buf[25:27])]
        data['hierarchy'] = TPSDecoder.HIERARCHY_VALUES[tuple(self.buf[27:30])]
        data['hp_code_rate'] = TPSDecoder.CODE_RATE_VALUES[tuple(self.buf[30:33])]
        data['lp_code_rate'] = TPSDecoder.CODE_RATE_VALUES[tuple(self.buf[33:36])]
        data['guard_interval'] = TPSDecoder.GUARD_INTERVAL_VALUES[tuple(self.buf[36:38])]
        data['transmission_mode'] = TPSDecoder.TRANSMISSION_MODE_VALUES[tuple(self.buf[38:40])]
        data['cell_id_byte'] = tuple(self.buf[40:48])
        assert self.buf[48:54] == [0] * 6
        data['error_protection'] = tuple(self.buf[54:68])

        return data

def float_xrange(start, stop, step=1.0, first=True):
    while (step > 0.0 and start < stop) or (step < 0.0 and start > stop):
        if first:
            yield start
        else:
            first = True
        start += step

def last_before(start, step, ref):
    return start + step * math.floor((ref - start) / step)

def complex_in_square(z, x1, x2):
    return x1.real <= z.real and z.real <= x2.real \
        and x1.imag <= z.imag and z.imag <= x2.imag

def read_from_file():
    data = []
    with open("dvbt2.pgm") as fin:
        for i in xrange(4):
            fin.readline()
        line = fin.readline()
        while line != '':
            data.append(float(line.strip()) / 255.0)
            line = fin.readline()
    return data

def decode_symbol(data, offset):
    sample = data[offset:offset + packet_len]
    freqs = fft(sample)
    return freqs[lower_idx:higher_idx + 1]

def cofdm_encoder(symbols=100):
    data = []
    for i in xrange(symbols):
        samples = [0.0+0.0j for i in xrange(packet_len)]
        samples[lower_idx:higher_idx] = [random.randrange(-3, 4, 2) + random.randrange(-3, 4, 2) * 1j for i in xrange(higher_idx - lower_idx)]
        tmp = list(ifft(samples))
        #tmp = [0.0 for i in xrange(guard_len)] + data
        tmp = tmp[-guard_len:] + tmp
        data += tmp
    return data

def cofdm_decoder(data, frame=0, animation=None):
    start = frame
    if animation is None:
        animation = 1
    stop = start + animation

    for i in xrange(start, stop):
        print i
        #offset = 0 * guard_len + 100 * (packet_len + guard_len) + i
        offset = 0 * guard_len + 0 * (packet_len + guard_len) + i
        sample = data[offset:offset+packet_len]
        freqs = fft(sample)
        interesting = freqs[lower_idx:higher_idx]
        clf()
        #plot(map(lambda x: abs(x)**2, interesting))
        scatter(map(lambda x: x.real, interesting), map(lambda x: x.imag, interesting))
        savefig("frames/prova_%04d.png" % (i))
        #show()

def subsample_animation(data, start=0, step=0.1, number=200):
    freqs = decode_symbol(data, start)
    for i in xrange(number):
        clf()
        ylim(-60.0, 60.0)
        axis('equal')
        scatter(map(lambda x: x.real, freqs), map(lambda x: x.imag, freqs))
        savefig("frames/prova_%04d.png" % (i))
        freqs = map(lambda (i, x): x * exp(2j * cmath.pi * (lower_idx+i) * step / packet_len), enumerate(freqs))

def deteriorate_signal(data):
    for i in xrange(len(data) / 3672):
        data[3672*(i+1)-16:3672*(i+1)] = [0.0] * 16

def subsample_animation_from_file():
    data = read_from_file()
    subsample_animation(data, start=525)

def decode_from_file():
    data = read_from_file()
    cofdm_decoder(data, frame=500+2*(packet_len+guard_len), animation=600)

def encode_and_decode():
    data = cofdm_encoder()
    deteriorate_signal(data)
    cofdm_decoder(data, frame=0, animation=None)

def follow_subcarrier(data, sc, symbols):
    offset = 0
    for symbol in xrange(symbols):
        full_offset = offset + (packet_len + guard_len) * symbol
        carriers = decode_symbol(data, full_offset)
        value = carriers[sc]
        print abs(value)**2, value

def follow_subcarrier_from_file():
    data = read_from_file()
    follow_subcarrier(data, 0, 10)
    print
    follow_subcarrier(data, 1, 10)
    print
    follow_subcarrier(data, 3, 25)

def encode_and_follow_subcarrier():
    data = cofdm_encoder()
    deteriorate_signal(data)
    follow_subcarrier(data, 0, 10)

def detect_pilots(data):
    #offset = 577 + 100 * (packet_len + guard_len)
    offset = 527
    carriers = decode_symbol(data, offset)
    carriers = map(lambda x: x * cmath.exp(-(cmath.pi/4+0.15+cmath.pi)*1j), carriers)
    #scatter(map(lambda x: x.real, carriers), map(lambda x: x.imag, carriers))
    #show()
    #plot(map(lambda x: abs(x)**2, carriers))
    #show()
    positive_pilots = filter(lambda (x, y): complex_in_square(y, 24-5j, 55+5j), enumerate(carriers))
    negative_pilots = filter(lambda (x, y): complex_in_square(y, -55-5j, -24+5j), enumerate(carriers))
    print len(negative_pilots), len(positive_pilots)
    #print map(lambda (x, y): x, positive_pilots)
    #print map(lambda (x, y): x, negative_pilots)
    #pilots = sorted(positive_pilots + negative_pilots)
    #print map(lambda (x, y): x, pilots)
    #diffs = [pilots[i+1][0] - pilots[i][0] for i in xrange(len(pilots)-1)]
    #print diffs
    #nulls = filter(lambda (x, y): complex_in_square(y, -4-4j, 4+4j), enumerate(carriers))
    #print map(lambda (x, y): x, nulls)
    for carrier, _ in positive_pilots:
        if pilots_sequence[carrier] != 1:
            print "Wrong positive pilot carrier: %d" % (carrier)
    for carrier, _ in negative_pilots:
        if pilots_sequence[carrier] != 0:
            print "Wrong negative pilot carrier: %d" % (carrier)

def detect_pilots_from_file():
    data = read_from_file()
    detect_pilots(data)

def finely_adjust_freqs(freqs, window_width=30.0):
    value = 0.0
    derivative = 0.0
    position = -window_width
    queue = []
    for pilot in CONTINUAL_PILOTS_2K:
        phase = cmath.polar(freqs[pilot])[1]
        target_phase = 0.0 if pilots_sequence[pilot] == 1 else math.pi
        freq = (lower_idx + pilot) / packet_time
        points_start = (target_phase - phase) / (2 * cmath.pi * freq) * signal_freq
        step = signal_freq / freq
        peaks_start = points_start - step / 2
        last_point = last_before(points_start, step, position)
        last_peak = last_before(peaks_start, step, position)
        if last_point > last_peak:
            derivative += 1.0
            value += position - last_point
            heapq.heappush(queue, (last_point + step / 2, step, -1))
        else:
            derivative -= 1.0
            value += step / 2 - (position - last_peak)
            heapq.heappush(queue, (last_peak + step / 2, step, 1))

    min_value = value
    min_pos = position

    while position < window_width:
        next_position, step, change = heapq.heappop(queue)
        next_position = min(next_position, window_width)
        value += (next_position - position) * derivative
        derivative += 2 * change
        #print position, value
        assert value >= 0.0
        if value < min_value:
            min_value = value
            min_pos = position
        position = next_position
        heapq.heappush(queue, (next_position + step / 2, step, -change))

    return min_pos, min_value

def shift_freqs(freqs, time):
    res = []
    for i, value in enumerate(freqs):
        freq = (lower_idx + i) / packet_time
        res.append(value * cmath.exp(2j * cmath.pi * (time / signal_freq) * freq))
    return res

def finely_adjust_from_file():
    data = read_from_file()
    offset = 527
    freqs = decode_symbol(data, offset)
    shift_time, value = finely_adjust_freqs(freqs)
    print shift_time, value
    freqs = shift_freqs(freqs, shift_time)
    clf()
    scatter(map(lambda x: x.real, freqs), map(lambda x: x.imag, freqs))
    show()

def decode_tps_bit(freqs):
    vote_1 = 0
    vote_0 = 0
    for carrier in TPS_CARRIERS_2K:
        bit = ((1 if freqs[carrier].real > 0.0 else 0) + pilots_sequence[carrier]) % 2
        if bit == 1:
            vote_1 += 1
        else:
            vote_0 += 1
    #print vote_0, vote_1
    return 1 if vote_1 > vote_0 else 0

def process_symbol(data, offset):
    prev_tps_bit = 0
    tps = []
    for i in xrange(2 * 68):
        freqs = decode_symbol(data, offset)
        shift_time, value = finely_adjust_freqs(freqs)
        #print shift_time
        freqs = shift_freqs(freqs, shift_time)
        tps_bit = decode_tps_bit(freqs)
        decoded_tps_bit = 0 if tps_bit == prev_tps_bit else 1
        #print tps_bit
        tps.append(decoded_tps_bit)
        prev_tps_bit = tps_bit
        offset += packet_len + guard_len + int(round(shift_time))

    print tps

def process_symbol_from_file():
    data = read_from_file()
    offset = 527
    process_symbol(data, offset)

def decode_stream():
    data = read_from_file()
    fin = ContinuousFile(data)
    window = SlidingWindow(fin)

    # Just to have some free space
    window.advance(5000)
    window.reserve_front(3 * (packet_len + guard_len) + 5000)
    #votes = []
    #for i in xrange(0, 2 * (packet_len + guard_len), 5000):
    #    print i
    #    freqs = decode_symbol(window, i)
    #    shift, value = finely_adjust_freqs(freqs, window_width=5000.0)
    #    votes.append((value, int(round(i + shift))))

    # Align the window
    #votes.sort()
    #window.advance(votes[0][1])
    #print votes[0][1]
    window.advance(30871)

    # Start really decoding symbols
    prev_tps_bit = 0
    tps_decoder = TPSDecoder()
    while True:

        # Compute the FFT and optimal shifting
        window.reserve_front(2 * (packet_len + guard_len))
        window.reserve_back(2 * (packet_len + guard_len))
        freqs = decode_symbol(window, 0)
        shift, _ = finely_adjust_freqs(freqs)
        window.advance(int(round(shift)))
        window.advance(packet_len + guard_len)
        freqs = shift_freqs(freqs, shift)

        # Some debug information
        #print shift, window.total_offset
        #clf()
        #scatter(map(lambda x: x.real, freqs), map(lambda x: x.imag, freqs))
        #show()

        # Here we can work on the data
        tps_bit = decode_tps_bit(freqs)
        decoded_tps_bit = 0 if tps_bit == prev_tps_bit else 1
        prev_tps_bit = tps_bit
        #print decoded_tps_bit
        tps_data = tps_decoder.push_bit(decoded_tps_bit)
        #if tps_data is not None:
        #    print tps_data

if __name__ == '__main__':
    #decode_from_file()
    #encode_and_decode()
    #follow_subcarrier_from_file()
    #encode_and_follow_subcarrier()
    #detect_pilots_from_file()
    #subsample_animation_from_file()
    #finely_adjust_from_file()
    #process_symbol_from_file()
    decode_stream()

    #test_PRBS()
