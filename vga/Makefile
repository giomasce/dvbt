
all: vga vga_simple

vga: vga.c
	gcc -O3 -Wall -pedantic -std=gnu99 -o vga -lm -lGLU -lGL -lglut -lGLEW -lX11 -lXxf86vm vga.c

vga_simple: vga_simple.c
	gcc -Wall -o vga_simple -lm -lGLU -lGL -lglut -lGLEW vga_simple.c

clean:
	rm -f vga vga_simple
