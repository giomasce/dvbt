
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>

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

inline static size_t min(size_t x, size_t y) {

  return (x < y) ? x : y;

}

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
  printf("data_buf_len: %u\n", (unsigned int) data_buf_len);
  data_buf = (unsigned char*) malloc(data_buf_len * sizeof(unsigned char));
  for (size_t i = 0; i < data_buf_len; i++) {
    data_buf[i] = (unsigned char) round(255.0 * 0.5 * (1.0 + sin(2.0 * M_PI * ((double) i) / ((double) data_buf_len))));
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
  printf("data_buf_len: %u\n", (unsigned int) data_buf_len);

}

#define init_data_buf init_data_buf_file

inline static const unsigned char *GetData(size_t request, size_t *num) {

  *num = min(request, data_buf_len - data_pos);
  unsigned char *res = data_buf + data_pos;
  data_pos += *num;
  data_pos %= data_buf_len;

  return res;

}

inline static void consume_data(size_t request) {

  while (request > 0) {
    size_t num;
    GetData(request, &num);
    request -= num;
  }

}

unsigned char *screen = NULL;
unsigned int pbo_buf[2];
size_t pbo_idx = 0;
unsigned int texture;

void DisplayCallback() {

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  //gluOrtho2D(0, modeline.hdisplay, 0, modeline.vdisplay);

  glClear(GL_COLOR_BUFFER_BIT);

  const unsigned char *buf;
  for (int line = 0; line < modeline.vdisplay; line++) {
    size_t num;
    for (int pos = 0; pos < modeline.hdisplay; pos += num) {
      //printf("  %d %d\n", pos, line);
      buf = GetData(modeline.hdisplay - pos, &num);
      memcpy(screen + pos + line * modeline.hdisplay, buf, num);
      /*int base = pos + line * modeline.hdisplay;
      for (int i = 0; i < num; i++) {
        screen[base + i] = buf[i];
        }*/
    }
    consume_data(modeline.htotal - modeline.hdisplay);
  }
  consume_data(modeline.htotal * (modeline.vtotal - modeline.vdisplay));

  glBindBufferARB(GL_PIXEL_PACK_BUFFER, pbo_buf[pbo_idx]);
  assert(glGetError() == 0);
  glBufferSubDataARB(GL_PIXEL_PACK_BUFFER, 0, modeline.vdisplay * modeline.hdisplay * sizeof(unsigned char), screen);
  assert(glGetError() == 0);
  glBindBufferARB(GL_PIXEL_PACK_BUFFER, 0);
  assert(glGetError() == 0);
  //screen = (unsigned char*) glMapBufferARB(GL_PIXEL_PACK_BUFFER, GL_WRITE_ONLY);
  //assert(glGetError() == 0);
  //assert(screen);

  // Apparently we don't really need two PBOs
  //pbo_idx = (pbo_idx + 1) % 2;

  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER, pbo_buf[pbo_idx]);
  assert(glGetError() == 0);
  //glRasterPos2i(0, 0);
  //assert(glGetError() == 0);
  //glDrawPixels(modeline.hdisplay, modeline.vdisplay, GL_RED, GL_UNSIGNED_BYTE, 0);
  //assert(glGetError() == 0);
  glBindTexture(GL_TEXTURE_2D, texture);
  assert(glGetError() == 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  assert(glGetError() == 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, modeline.vdisplay, modeline.hdisplay, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
  assert(glGetError() == 0);
  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER, 0);
  assert(glGetError() == 0);

  glBegin(GL_QUADS);
  glTexCoord2d(-1.0, -1.0);
  glVertex2d(-1.0, -1.0);
  glTexCoord2d(1.0, -1.0);
  glVertex2d(1.0, -1.0);
  glTexCoord2d(1.0, 1.0);
  glVertex2d(1.0, 1.0);
  glTexCoord2d(-1.0, 1.0);
  glVertex2d(-1.0, 1.0);
  glEnd();
  assert(glGetError() == 0);

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
    double first_diff = (ts.tv_sec - first_ts.tv_sec) + 1e-9 * (ts.tv_nsec - first_ts.tv_nsec);
    double est_num = first_diff * fps;
    double diff = (ts.tv_sec - prev_ts.tv_sec) + 1e-9 * (ts.tv_nsec - prev_ts.tv_nsec);
    double var = fps * diff - 1.0;
    const char *sig = "";
    if (var < -0.1) sig = "-";
    if (var > 0.1) sig = "+";
    printf("%d %f %f %f %f %s\n", frames, est_num, est_num - frames, diff, var, sig);
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

  glDeleteBuffersARB(2, pbo_buf);
  assert(glGetError() == 0);
  glGenBuffersARB(2, pbo_buf);
  assert(glGetError() == 0);
  for (size_t idx = 0; idx < 2; idx++) {
    glBindBufferARB(GL_PIXEL_PACK_BUFFER, pbo_buf[idx]);
    assert(glGetError() == 0);
    glBufferDataARB(GL_PIXEL_PACK_BUFFER, w * h * sizeof(unsigned char), NULL, GL_STREAM_DRAW);
    assert(glGetError() == 0);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER, 0);
    assert(glGetError() == 0);
  }

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

  glGenBuffersARB(2, pbo_buf);
  assert(glGetError() == 0);
  glGenTextures(1, &texture);
  assert(glGetError() == 0);
  glEnable(GL_TEXTURE_2D);
  assert(glGetError() == 0);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  assert(glGetError() == 0);

  glutMainLoop();

  return 0;

}
