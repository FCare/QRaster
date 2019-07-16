#include "vdp1_compute.h"
#include "vdp1_prog_compute.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#define VDP1CDEBUG
#ifdef VDP1CDEBUG
#define VDP1CPRINT printf
#else
#define VDP1CPRINT
#endif

static int tex_width;
static int tex_height;

static GLuint compute_tex = 0;
static GLuint prg_vdp1[NB_PRG] = {0};

static const GLchar * a_prg_vdp1[NB_PRG][3] = {
	{
		vdp1_start_f,
		vdp1_test_f,
		vdp1_end_f
  }
};

static int getProgramId() {
  return TEST_PRG;
}

int ErrorHandle(const char* name)
{
#ifdef VDP1CDEBUG
  GLenum   error_code = glGetError();
  if (error_code == GL_NO_ERROR) {
    return  1;
  }
  do {
    const char* msg = "";
    switch (error_code) {
    case GL_INVALID_ENUM:      msg = "INVALID_ENUM";      break;
    case GL_INVALID_VALUE:     msg = "INVALID_VALUE";     break;
    case GL_INVALID_OPERATION: msg = "INVALID_OPERATION"; break;
    case GL_OUT_OF_MEMORY:     msg = "OUT_OF_MEMORY";     break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:  msg = "INVALID_FRAMEBUFFER_OPERATION"; break;
    default:  msg = "Unknown"; break;
    }
    VDP1CPRINT("GLErrorLayer:ERROR:%04x'%s' %s\n", error_code, msg, name);
    error_code = glGetError();
  } while (error_code != GL_NO_ERROR);
  abort();
  return 0;
#else
  return 1;
#endif
}

static GLuint createProgram(int count, const GLchar** prg_strs) {
  GLint status;
  GLuint result = glCreateShader(GL_COMPUTE_SHADER);

  glShaderSource(result, count, prg_strs, NULL);
  glCompileShader(result);
  glGetShaderiv(result, GL_COMPILE_STATUS, &status);

  if (status == GL_FALSE) {
    GLint length;
    glGetShaderiv(result, GL_INFO_LOG_LENGTH, &length);
    GLchar *info = malloc(sizeof(GLchar) *length);
    glGetShaderInfoLog(result, length, NULL, info);
    VDP1CPRINT("[COMPILE] %s\n", info);
    free(info);
    abort();
  }
  GLuint program = glCreateProgram();
  glAttachShader(program, result);
  glLinkProgram(program);
  glDetachShader(program, result);
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    GLint length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    GLchar *info = malloc(sizeof(GLchar) *length);
    glGetProgramInfoLog(program, length, NULL, info);
    VDP1CPRINT("[LINK] %s\n", info);
    free(info);
    abort();
  }
  return program;
}

static int generateComputeBuffer(int w, int h) {
  if (compute_tex != 0) {
    glDeleteTextures(1,&compute_tex);
  }
  glGenTextures(1, &compute_tex);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, compute_tex);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return 0;
}

int vdp1_compute_init(int width, int height)
{
  tex_width = width;
  tex_height = height;
  generateComputeBuffer(width, height);
}

int vdp1_compute() {
  GLuint error;
  int local_size_x = 16;
  int local_size_y = 16;

	int work_groups_x = (tex_width) / local_size_x;
  int work_groups_y = (tex_height) / local_size_y;

	int progId = getProgramId();
	if (prg_vdp1[progId] == 0)
    prg_vdp1[progId] = createProgram(sizeof(a_prg_vdp1[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp1[progId]);

  glUseProgram(prg_vdp1[progId]);
	ErrorHandle("glUseProgram");

	VDP1CPRINT("Draw RBG0\n");
	glBindImageTexture(0, compute_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	ErrorHandle("glBindImageTexture 0");

  glDispatchCompute(work_groups_x, work_groups_y, 1);
	// glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  ErrorHandle("glDispatchCompute");

  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  return compute_tex;
}
