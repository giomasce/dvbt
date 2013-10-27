
all: dvbt streamer

GCC = gcc
CFLAGS = -g -O0 -std=gnu99 -Wall -pedantic
LDFLAGS = -lfftw3 -lfftw3_threads -lm
OBJECTS = main.o prbs.o util.o ofdm.o data.o tps.o

main.o: ofdm.h tps.h

ofdm.h: util.h data.h

ofdm.o: prbs.h

tps.h: data.h

%.h:
	touch $@

%.o: %.c %.h
	$(GCC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(GCC) $(CFLAGS) -c -o $@ $<

dvbt: ${OBJECTS}
	$(GCC) $(LDFLAGS) $(CFLAGS) -o $@ ${OBJECTS}

streamer: streamer.c
	$(GCC) $(LDFLAGS) $(CFLAGS) -o $@ $<

clean:
	rm -f dvbt streamer ${OBJECTS}
