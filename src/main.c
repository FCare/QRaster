/***************************************************************************/
/* Francois CARON - 15/07/2019                                             */
/* QRaster is intented to implement the Sega saturn VDP1 rasterizer and    */
/* Texture mapper as a compute shader in order to avoid issu of converting */
/* Quads to triangles and the processing of the openGL coordinates         */
/***************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#if defined(_USEGLEW_)
#include <GL/glew.h>
#endif

#define WIDTH 800
#define HEIGHT 480

#define __USE_OPENGL_DEBUG__

#include <GLFW/glfw3.h>

#include "vdp1_compute.h"
#include "standard_compute.h"

static GLFWwindow* g_window = NULL;
static int winprio_prg = -1;
static GLuint vertexPosition_buf = 0;
static GLuint vao = 0;

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

#if defined(__USE_OPENGL_DEBUG__)
static void MessageCallback( GLenum source,
                      GLenum type,
                      GLuint id,
                      GLenum severity,
                      GLsizei length,
                      const GLchar* message,
                      const void* userParam )
{
  printf("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}
#endif

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

#if defined(__USE_OPENGL_DEBUG__)
  // During init, enable debug output
  glEnable              ( GL_DEBUG_OUTPUT );
  glDebugMessageCallback( (GLDEBUGPROC) MessageCallback, 0 );
#endif

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);
  glViewport(0, 0, WIDTH, HEIGHT);
  glGenBuffers(1, &vertexPosition_buf);
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  vdp1_compute_init(WIDTH, HEIGHT);

}

static int blitSimple(GLint tex) {

  float const vertexPosition[] = {
  (float)WIDTH/2.0f, -(float)HEIGHT/2.0f,
  -(float)WIDTH/2.0f, -(float)HEIGHT/2.0f,
  (float)WIDTH/2.0f, (float)HEIGHT/2.0f,
  -(float)WIDTH/2.0f, (float)HEIGHT/2.0f };

  const char winprio_v[] =
    SHADER_VERSION
    "layout (location = 0) in vec2 a_position;   \n"
    "void main()       \n"
    "{ \n"
    " gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0); \n"
    "} ";

  const char winprio_f[] =
    SHADER_VERSION
    "#ifdef GL_ES\n"
    "precision highp float; \n"
    "#endif\n"
    "uniform sampler2D s_texture;  \n"
    "out vec4 fragColor; \n"
    "void main()   \n"
    "{  \n"
    "  fragColor = texelFetch( s_texture, ivec2(gl_FragCoord.xy), 0 );         \n"
    "}  \n";

  const GLchar * fblit_winprio_v[] = { winprio_v, NULL };
  const GLchar * fblit_winprio_f[] = { winprio_f, NULL };

  if (winprio_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;
    if (winprio_prg != -1) glDeleteProgram(winprio_prg);
    winprio_prg = glCreateProgram();
    if (winprio_prg == 0){
      return -1;
    }

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, fblit_winprio_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      printf("Compile error in vertex shader.\n");
      winprio_prg = -1;
      return -1;
    }

    glShaderSource(fshader, 1, fblit_winprio_f, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      printf("Compile error in fragment shader.\n");
      winprio_prg = -1;
      abort();
    }

    glAttachShader(winprio_prg, vshader);
    glAttachShader(winprio_prg, fshader);
    glLinkProgram(winprio_prg);
    glGetProgramiv(winprio_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      printf("Link error..\n");
      winprio_prg = -1;
      abort();
    }

    glUseProgram(winprio_prg);
    glUniform1i(glGetUniformLocation(winprio_prg, "s_texture"), 0);
  }
  else{
    glUseProgram(winprio_prg);
  }

  glDisable(GL_BLEND);

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexPosition_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPosition), vertexPosition, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);


  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  return 0;
}

static void updateVDP1() {
  vdp1add(WIDTH/16, 3*HEIGHT/16, WIDTH/2, 2*HEIGHT/16, 3*WIDTH/16, 3*HEIGHT/16, WIDTH/2, HEIGHT/16);
  int tex = vdp1_compute();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  blitSimple(tex);
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
