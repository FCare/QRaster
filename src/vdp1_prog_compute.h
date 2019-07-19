#ifndef VDP1_PROG_COMPUTE_H
#define VDP1_PROG_COMPUTE_H

#include "standard_compute.h"

static const char vdp1_start_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"layout(local_size_x = 8, local_size_y = 8) in;\n"
"layout(rgba8, binding = 0) writeonly highp uniform image2D outSurface;\n"
" struct cmdparameter_struct{ \n"
" uint coord[4];\n"
"};\n"
"layout(std430, binding = 1) readonly buffer NB_CMD { uint nbCmd[]; };\n"
"layout(std430, binding = 2) readonly buffer CMD { \n"
"  cmdparameter_struct cmd[];\n"
"};\n"

"vec4 finalColor = vec4(0.0);\n"
"ivec2 texel = ivec2(0,0);\n"
"void main()\n"
"{\n"
"  texel = ivec2(gl_GlobalInvocationID.xy);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  if (nbCmd[gl_WorkGroupID.x + gl_WorkGroupID.y * gl_NumWorkGroups.x] == 0u) return;\n";

static const char vdp1_end_f[] =
"  imageStore(outSurface,texel,finalColor);\n"
"}\n";

static const char vdp1_test_f[] =
"  finalColor = vec4(0.5);\n";

#endif //VDP1_PROG_COMPUTE_H
