
all: vga vga_simple modeline upconverter generator

vga: vga.c
	gcc -O3 -Wall -pedantic -std=gnu99 -o vga -lm -lGLU -lGL -lglut -lGLEW -lX11 -lXxf86vm -lfftw3 -lfftw3_threads vga.c

vga_simple: vga_simple.c
	gcc -Wall -o vga_simple -lm -lGLU -lGL -lglut -lGLEW vga_simple.c

modeline: modeline.c
	gcc -Wall -o modeline -lX11 -lXxf86vm modeline.c

upconverter: upconverter.c
	gcc -O3 -Wall -pedantic -std=gnu99 -o upconverter -lm upconverter.c

generator: generator.c
	gcc -O3 -Wall -pedantic -std=gnu99 -o generator -lm generator.c

clean:
	rm -f vga vga_simple modeline upconverter generator
