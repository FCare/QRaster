#ifndef STD_COMPUTE_H
#define STD_COMPUTE_H

#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1
#include <GL/glew.h>
#include <GL/gl.h>

#if defined(_OGLES3_)
#define SHADER_VERSION "#version 310 es \n"
#define SHADER_VERSION_TESS "#version 310 es \n#extension GL_ANDROID_extension_pack_es31a : enable \n"
#define SHADER_VERSION_COMPUTE "#version 310 es \n"

#else
#define SHADER_VERSION "#version 330 core \n"
#define SHADER_VERSION_TESS "#version 420 core \n"
#define SHADER_VERSION_COMPUTE "#version 430 core \n"
#endif


#endif //STD_COMPUTE_H
