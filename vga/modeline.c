
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

Display *display;
int screen;
int dotclock;
XF86VidModeModeLine modeline, new_modeline;

int main(int argc, char **argv) {

  if (argc != 1 && argc != 9) {
    fprintf(stderr, "Please call me with 0 or 8 arguments\n");
    return 1;
  }

  display = XOpenDisplay(NULL);
  screen = XDefaultScreen(display);

  assert(XF86VidModeGetModeLine(display, screen, &dotclock, &modeline));
  printf("%d    %d %d %d %d    %d %d %d %d\n", dotclock,
         modeline.hdisplay, modeline.hsyncstart, modeline.hsyncend, modeline.htotal,
         modeline.vdisplay, modeline.vsyncstart, modeline.vsyncend, modeline.vtotal);

  if (argc == 9) {
    bzero(&modeline, sizeof(XF86VidModeModeLine));
    new_modeline.hdisplay = atoi(argv[1]);
    new_modeline.hsyncstart = atoi(argv[2]);
    new_modeline.hsyncend = atoi(argv[3]);
    new_modeline.htotal = atoi(argv[4]);
    new_modeline.vdisplay = atoi(argv[5]);
    new_modeline.vsyncstart = atoi(argv[6]);
    new_modeline.vsyncend = atoi(argv[7]);
    new_modeline.vtotal = atoi(argv[8]);
    new_modeline.flags = modeline.flags;
    assert(XF86VidModeModModeLine(display, screen, &new_modeline));

    assert(XF86VidModeGetModeLine(display, screen, &dotclock, &modeline));
    printf("%d    %d %d %d %d    %d %d %d %d\n", dotclock,
           modeline.hdisplay, modeline.hsyncstart, modeline.hsyncend, modeline.htotal,
           modeline.vdisplay, modeline.vsyncstart, modeline.vsyncend, modeline.vtotal);
  }

  XCloseDisplay(display);

  return 0;

}
