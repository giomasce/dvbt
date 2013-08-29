
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>

#include <GL/glew.h>
#include <GL/glxew.h>
#include <GL/freeglut.h>

unsigned int frames = 0;
unsigned int colors = 3;

void DisplayCallback() {

  glClear(GL_COLOR_BUFFER_BIT);
  assert(glGetError() == 0);

  /* Just draw a big quad whose color depends on (frames % colors) */
  double color = (double) (frames % colors) / (colors-1);
  glColor3d(color, color, color);
  assert(glGetError() == 0);
  glBegin(GL_QUADS);
  glVertex2d(-1.0, -1.0);
  glVertex2d(1.0, -1.0);
  glVertex2d(1.0, 1.0);
  glVertex2d(-1.0, 1.0);
  glEnd();
  assert(glGetError() == 0);

  glutSwapBuffers();

  frames++;

  /* Schedule a new display call; the framerate is limited by the
     swap_control extension */
  glutPostRedisplay();

}

void KeyboardCallback(unsigned char key, int x, int y) {

  if (key == 'x' || key == 'X') {
    exit(0);
  }

}

void ReshapeCallback(int w, int h) {

  glViewport(0, 0, w, h);

}

int main(int argc, char** argv) {

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutCreateWindow("");
  glutFullScreen();

  glutDisplayFunc(DisplayCallback);
  glutKeyboardFunc(KeyboardCallback);
  glutReshapeFunc(ReshapeCallback);

  /* Init GLEW library */
  GLenum res = glewInit();
  if (res != GLEW_OK) {
    fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
    return 1;
  }

  /* Check that GLX_SGI_swap_control is available and enable swap
     control */
  if (GLXEW_SGI_swap_control) {
    glXSwapIntervalSGI(1);
    assert(glGetError() == 0);
  } else {
    printf("Extension GLX_SGI_swap_control not found, aborting...\n");
    return 1;
  }

  /* Reset some bits of the OpenGL context */
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  assert(glGetError() == 0);
  glMatrixMode(GL_PROJECTION);
  assert(glGetError() == 0);
  glLoadIdentity();
  assert(glGetError() == 0);
  glMatrixMode(GL_MODELVIEW);
  assert(glGetError() == 0);
  glLoadIdentity();
  assert(glGetError() == 0);

  glutMainLoop();

  return 0;

}
