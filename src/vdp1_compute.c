#include "vdp1_compute.h"
#include "vdp1_prog_compute.h"
#include "sonic.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#define VDP1CDEBUG
#ifdef VDP1CDEBUG
#define VDP1CPRINT printf
#else
#define VDP1CPRINT
#endif

#define NB_COARSE_RAST (NB_COARSE_RAST_X * NB_COARSE_RAST_Y)

static int local_size_x = LOCAL_SIZE_X;
static int local_size_y = LOCAL_SIZE_Y;

static int tex_width;
static int tex_height;
static int tex_ratio;
static int struct_size;

static int work_groups_x;
static int work_groups_y;

static cmdparameter* cmdVdp1;
static int* nbCmd;

static int* clear;

static GLuint compute_tex = 0;
static GLuint ssbo_cmd_ = 0;
static GLuint ssbo_vram_ = 0;
static GLuint ssbo_nbcmd_ = 0;
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

static char pixel[SONIC_WIDTH*SONIC_HEIGHT*4];

static int generateComputeBuffer(int w, int h) {
  if (compute_tex != 0) {
    glDeleteTextures(1,&compute_tex);
  }
	if (ssbo_vram_ != 0) {
    glDeleteBuffers(1, &ssbo_vram_);
	}
  for (int i = 0; i< SONIC_WIDTH*SONIC_HEIGHT; i++) {
	  HEADER_PIXEL(&header_data, &pixel[4*i]);
	  pixel[4*i+3] = 0xFF;
  }

	glGenBuffers(1, &ssbo_vram_);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
	glBufferData(GL_SHADER_STORAGE_BUFFER, SONIC_WIDTH*SONIC_HEIGHT*4, pixel, GL_STATIC_READ);

  if (ssbo_cmd_ != 0) {
    glDeleteBuffers(1, &ssbo_cmd_);
  }
  glGenBuffers(1, &ssbo_cmd_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cmd_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, struct_size*2000*NB_COARSE_RAST, NULL, GL_DYNAMIC_DRAW);

  if (ssbo_nbcmd_ != 0) {
    glDeleteBuffers(1, &ssbo_nbcmd_);
  }
  glGenBuffers(1, &ssbo_nbcmd_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_nbcmd_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, NB_COARSE_RAST * sizeof(int),NULL,GL_DYNAMIC_DRAW);

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

int vdp1_add(cmdparameter* cmd) {
	int Ax = cmd->P[0];
	int Ay = cmd->P[1];
	int Bx = cmd->P[2];
	int By = cmd->P[3];
	int Cx = cmd->P[4];
	int Cy = cmd->P[5];
	int Dx = cmd->P[6];
	int Dy = cmd->P[7];

  int minx = (Ax < Bx)?Ax:Bx;
  int miny = (Ay < By)?Ay:By;
  int maxx = (Ax > Bx)?Ax:Bx;
  int maxy = (Ay > By)?Ay:By;

  minx = (minx < Cx)?minx:Cx;
  minx = (minx < Dx)?minx:Dx;
  miny = (miny < Cy)?miny:Cy;
  miny = (miny < Dy)?miny:Dy;
  maxx = (maxx > Cx)?maxx:Cx;
  maxx = (maxx > Dx)?maxx:Dx;
  maxy = (maxy > Cy)?maxy:Cy;
  maxy = (maxy > Dy)?maxy:Dy;

  int intersectX = -1;
  int intersectY = -1;
  for (int i = 0; i<NB_COARSE_RAST_X; i++) {
    int blkx = i * (tex_width/NB_COARSE_RAST_X);
    for (int j = 0; j<NB_COARSE_RAST_Y; j++) {
      int blky = j*(tex_height/NB_COARSE_RAST_Y);
      if (!(blkx > maxx
        || (blkx + (tex_width/NB_COARSE_RAST_X)) < minx
        || (blky + (tex_height/NB_COARSE_RAST_Y)) < miny
        || blky > maxy)) {
					memcpy(&cmdVdp1[(i+j*NB_COARSE_RAST_X)*2000 + nbCmd[i+j*NB_COARSE_RAST_X]], cmd, sizeof(cmdparameter));
          nbCmd[i+j*NB_COARSE_RAST_X]++;
      }
    }
  }
}

int vdp1_compute_init(int width, int height, float ratio)
{
  int am = sizeof(cmdparameter) % 16;
  tex_width = width;
  tex_height = height;
	tex_ratio = ratio;
  struct_size = sizeof(cmdparameter);
  if (am != 0) {
    struct_size += 16 - am;
  }

  clear = (int*)malloc(tex_height*tex_ratio * tex_width*tex_ratio * sizeof(int));
	memset(clear, 0, tex_height*tex_ratio * tex_width*tex_ratio * sizeof(int));

  work_groups_x = (tex_width*tex_ratio) / local_size_x;
  work_groups_y = (tex_height*tex_ratio) / local_size_y;

  generateComputeBuffer(width*ratio, height*ratio);
  nbCmd = (int*)malloc(NB_COARSE_RAST *sizeof(int));
  cmdVdp1 = (cmdparameter*)malloc(NB_COARSE_RAST*2000*sizeof(cmdparameter));
  memset(nbCmd, 0, NB_COARSE_RAST*sizeof(int));
	memset(cmdVdp1, 0, NB_COARSE_RAST*2000*sizeof(cmdparameter*));
}

int vdp1_compute() {
  GLuint error;
	int progId = getProgramId();
	if (prg_vdp1[progId] == 0)
    prg_vdp1[progId] = createProgram(sizeof(a_prg_vdp1[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp1[progId]);
  glUseProgram(prg_vdp1[progId]);

	glBindTexture(GL_TEXTURE_2D, compute_tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width*tex_ratio, tex_height*tex_ratio, GL_BGRA, GL_UNSIGNED_BYTE, clear);

	ErrorHandle("glUseProgram");

	VDP1CPRINT("Draw VDP1\n");

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_nbcmd_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int)*NB_COARSE_RAST, (void*)nbCmd);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cmd_);
  for (int i = 0; i < NB_COARSE_RAST; i++) {
    if (nbCmd[i] != 0)
    	glBufferSubData(GL_SHADER_STORAGE_BUFFER, struct_size*i*2000, nbCmd[i]*sizeof(cmdparameter), (void*)&cmdVdp1[2000*i]);
  }

	glBindImageTexture(0, compute_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_nbcmd_);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_cmd_);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_vram_);
	glUniform1i(4, SONIC_WIDTH);
	glUniform1i(5, SONIC_HEIGHT);
	glUniform1f(6, tex_ratio);

  glDispatchCompute(work_groups_x, work_groups_y, 1); //might be better to launch only the right number of workgroup
	// glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  ErrorHandle("glDispatchCompute");
	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BI
  memset(nbCmd, 0, NB_COARSE_RAST*sizeof(int));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  return compute_tex;
}
