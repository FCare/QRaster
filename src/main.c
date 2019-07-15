#if defined(_USEGLEW_)
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

static GLFWwindow* g_window = NULL;

static void error_callback(int error, const char* description)
{
  fputs(description, stderr);
}

static int platform_SetupOpenGL(int w, int h, int fullscreen) {
  int i;
  if (!glfwInit())
    return 0;

  glfwSetErrorCallback(error_callback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  glfwWindowHint(GLFW_OPENGL_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RED_BITS,8);
  glfwWindowHint(GLFW_GREEN_BITS,8);
  glfwWindowHint(GLFW_BLUE_BITS,8);
  glfwWindowHint(GLFW_ALPHA_BITS,8);

  g_window = glfwCreateWindow(w, h, "QRaster", NULL, NULL);

  if (!g_window)
  {
    glfwTerminate();
    return 0;
  }

  glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
  glfwMakeContextCurrent(g_window);
  glfwSwapInterval(0);
#if defined(_USEGLEW_)
  glewExperimental=GL_TRUE;
#endif

   int maj, min;
   glGetIntegerv(GL_MAJOR_VERSION, &maj);
   glGetIntegerv(GL_MINOR_VERSION, &min);

  printf("OpenGL version is %d.%d (%s, %s)\n", maj, min, glGetString(GL_VENDOR), glGetString(GL_RENDERER));
  return 1;
}

static int SetupOpenGL(int w, int h) {
  if (!platform_SetupOpenGL(w,h,fullscreen))
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  SetupOpenGL(600, 480);
}
