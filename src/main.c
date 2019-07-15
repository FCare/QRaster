/***************************************************************************/
/* Francois CARON - 15/07/2019                                             */
/* QRaster is intented to implement the Sega saturn VDP1 rasterizer and    */
/* Texture mapper as a compute shader in order to avoid issu of converting */
/* Quads to triangles and the processing of the openGL coordinates         */
/***************************************************************************/

#include <stdio.h>
#include <unistd.h>

#if defined(_USEGLEW_)
#include <GL/glew.h>
#endif

#define WIDTH 800
#define HEIGHT 480

#include <GLFW/glfw3.h>

static GLFWwindow* g_window = NULL;

static void error_callback(int error, const char* description)
{
  fputs(description, stderr);
}

static int setupOpenGL(int w, int h) {
  int i;
  if (!glfwInit())
    return 0;

  glfwSetErrorCallback(error_callback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RED_BITS,8);
  glfwWindowHint(GLFW_GREEN_BITS,8);
  glfwWindowHint(GLFW_BLUE_BITS,8);
  glfwWindowHint(GLFW_ALPHA_BITS,8);

  g_window = glfwCreateWindow(w, h, "QRaster", NULL, NULL);

  if (!g_window)
  {
    glfwTerminate();
    return -1;
  }

  glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
  glfwMakeContextCurrent(g_window);
  glfwSwapInterval(0);
#if defined(_USEGLEW_)
  glewExperimental=GL_TRUE;
  if (glewInit() != 0) {
    printf("Glew can not init\n");
    return -1;
  }
#endif

   int maj, min;
   glGetIntegerv(GL_MAJOR_VERSION, &maj);
   glGetIntegerv(GL_MINOR_VERSION, &min);

  printf("OpenGL version is %d.%d (%s, %s)\n", maj, min, glGetString(GL_VENDOR), glGetString(GL_RENDERER));

}

static void releaseOpenGL() {
  glfwSetWindowShouldClose(g_window, GL_TRUE);
  glfwDestroyWindow(g_window);
  glfwTerminate();
}

static int shouldClose() {
  return glfwWindowShouldClose(g_window);
}

static void pollEvents() {
  glfwPollEvents();
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

static void setKeyCallback() {
  glfwSetKeyCallback(g_window, key_callback);
}

static void swapBuffers(void) {
   if( g_window == NULL ){
      return;
   }
   glfwSwapBuffers(g_window);
}

static void initDrawVDP1() {
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);

  glViewport(0, 0, WIDTH, HEIGHT);
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

static void updateVDP1() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  swapBuffers();
}

int main(int argc, char *argv[]) {

  setupOpenGL(WIDTH, HEIGHT);
  initDrawVDP1();
  setKeyCallback();

  while (!shouldClose())
  {
       pollEvents();
       updateVDP1();
  }

  releaseOpenGL();
}
