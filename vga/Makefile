
all: vga

vga: vga.c
	gcc -O3 -Wall -pedantic -std=gnu99 -o vga -lm -lGLU -lGL -lglut -lGLEW -lX11 -lXxf86vm vga.c

clean:
	rm vga
