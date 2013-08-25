/*

	Copyright 2010 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Tutorial 01 - Create a window
*/

using namespace std;

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include <GL/glew.h>
#include <GL/glxew.h>
#include <GL/freeglut.h>

int frames = 0;

int first = 1;
struct timespec first_ts, ts, prev_ts;

double samp_freq, screen_time, fps;
double signal_freq = 60.0;

int dotclock;
XF86VidModeModeLine modeline;

bool get_modeline() {

  Display *display;
  int screen;

  display = XOpenDisplay(NULL);
  screen = XDefaultScreen(display);
  bool res = XF86VidModeGetModeLine(display, screen, &dotclock, &modeline);

  XCloseDisplay(display);

  return res;

}

unsigned char *data_buf = NULL;
size_t data_pos = 0;
size_t data_buf_len;

void init_data_buf_sine() {

  data_buf_len = samp_freq / signal_freq;
  printf("data_buf_len: %lu\n", data_buf_len);
  data_buf = (unsigned char*) malloc(data_buf_len * sizeof(unsigned char));
  for (size_t i = 0; i < data_buf_len; i++) {
    data_buf[i] = (unsigned char) round(255.0 * 0.5 * (1.0 + sin((double) (2 * M_PI * i) / data_buf_len)));
  }

}

void init_data_buf_file() {

  FILE *fin = fopen("dvbt2.pgm", "r");
  int i;
  for (i = 0; i < 4; i++) {
    while (fgetc(fin) != '\n') {}
  }
  int res;
  unsigned int tmp;
  size_t cap = 1;
  data_buf = (unsigned char*) malloc(cap * sizeof(unsigned char));
  while (true) {
    res = fscanf(fin, "%u", &tmp);
    if (res == 0 || res == EOF) {
      break;
    }
    if (data_buf_len == cap) {
      cap *= 2;
      data_buf = (unsigned char*) realloc(data_buf, cap * sizeof(unsigned char));
    }
    data_buf[data_buf_len++] = tmp;
  }
  fclose(fin);
  printf("data_buf_len: %lu\n", data_buf_len);

}

#define init_data_buf init_data_buf_file

inline const unsigned char *GetData(size_t request, size_t *num) {

  *num = min(request, data_buf_len - data_pos);
  unsigned char *res = data_buf + data_pos;
  data_pos += *num;
  data_pos %= data_buf_len;

  return res;

}

void consume_data(size_t request) {

  while (request > 0) {
    size_t num;
    GetData(request, &num);
    request -= num;
  }

}

unsigned char *screen = NULL;

void DisplayCallback() {

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluOrtho2D(0, modeline.hdisplay, 0, modeline.vdisplay);

  /*if (frames % 2) {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  } else {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    }*/

  //glClear(GL_COLOR_BUFFER_BIT);

  const unsigned char *buf;
  for (int line = 0; line < modeline.vdisplay; line++) {
    size_t num;
    for (int pos = 0; pos < modeline.hdisplay; pos += num) {
      //printf("  %d %d\n", pos, line);
      buf = GetData(modeline.hdisplay - pos, &num);
      memcpy(screen + pos + line * modeline.hdisplay, buf, num);
      //glRasterPos2i(pos, line);
      //glDrawPixels(num, 1, GL_RED, GL_UNSIGNED_BYTE, buf);
    }
    consume_data(modeline.htotal - modeline.hdisplay);
  }
  consume_data(modeline.htotal * (modeline.vtotal - modeline.vdisplay));
  glRasterPos2i(0, 0);
  glDrawPixels(modeline.hdisplay, modeline.vdisplay, GL_RED, GL_UNSIGNED_BYTE, screen);

  //glFinish();
  glutSwapBuffers();
  //glFlush();
  //glFinish();

  if (first) {
    clock_gettime(CLOCK_MONOTONIC, &first_ts);
    first = 0;
    printf("%d\n", frames);
    memcpy(&prev_ts, &first_ts, sizeof(struct timespec));
  } else {
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double diff = (ts.tv_sec - prev_ts.tv_sec) + 1e-9 * (ts.tv_nsec - prev_ts.tv_nsec);
    double var = fps * diff - 1.0;
    const char *sig = "";
    if (var < -0.1) sig = "-";
    if (var > 0.1) sig = "+";
    printf("%d %f %f %s\n", frames, diff, var, sig);
    memcpy(&prev_ts, &ts, sizeof(struct timespec));
  }
  frames++;
  glutPostRedisplay();

}

void KeyboardCallback(unsigned char key, int x, int y) {

  if (key == 'x' || key == 'X') {
    exit(0);
  }

}

void ReshapeCallback(int w, int h) {

  printf("(%d, %d)\n", w, h);
  glViewport(0, 0, w, h);
  free(screen);
  screen = (unsigned char*) malloc(w * h * sizeof(unsigned char));

}

int main(int argc, char** argv) {

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  //glutInitWindowSize(1024, 768);
  //glutInitWindowPosition(100, 100);
  glutCreateWindow("");
  glutFullScreen();

  glutDisplayFunc(DisplayCallback);
  glutKeyboardFunc(KeyboardCallback);
  glutReshapeFunc(ReshapeCallback);

  // Must be done after glut is initialized!
  GLenum res = glewInit();
  if (res != GLEW_OK) {
    fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
    return 1;
  }

  // Check that the swap_control extension is there
  if (GLXEW_SGI_swap_control) {
    printf("Extension found\n");
    glXSwapIntervalSGI(1);
  } else {
    printf("Extension NOT found!\n");
    return 1;
  }

  // Get current mode line
  bool res2 = get_modeline();
  if (res2) {
    printf("%d    %d %d %d %d    %d %d %d %d\n", dotclock,
           modeline.hdisplay, modeline.hsyncstart, modeline.hsyncend, modeline.htotal,
           modeline.vdisplay, modeline.vsyncstart, modeline.vsyncend, modeline.vtotal);
  } else {
    printf("Couldn't get modeline...\n");
    return 1;
  }

  samp_freq = 1000.0 * dotclock;
  fps = samp_freq / modeline.htotal / modeline.vtotal;
  screen_time = 1.0 / fps;
  printf("%f %f %f\n", samp_freq, screen_time, fps);

  init_data_buf();

  glutMainLoop();

  return 0;

}
