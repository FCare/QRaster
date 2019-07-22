#ifndef VDP1_PROG_COMPUTE_H
#define VDP1_PROG_COMPUTE_H

#include "standard_compute.h"

#define QuoteIdent(ident) #ident
#define Stringify(macro) QuoteIdent(macro)

#define NB_COARSE_RAST_X 8
#define NB_COARSE_RAST_Y 8

#define LOCAL_SIZE_X 8
#define LOCAL_SIZE_Y 8

static const char vdp1_start_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"

"struct point{\n"
"  int x;\n"
"  int y;\n"
"};\n"
"struct cmdparameter_struct{ \n"
"  int P[8];\n"
"};\n"

"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(rgba8, binding = 0) writeonly highp uniform image2D outSurface;\n"
"layout(std430, binding = 1) readonly buffer NB_CMD { uint nbCmd[]; };\n"
"layout(std430, binding = 2) readonly buffer CMD { \n"
"  cmdparameter_struct cmd[];\n"
"};\n"
"layout(std430, binding = 3) readonly buffer COLOR { uint color[]; };\n"
"layout(location = 4) uniform int colWidth;\n"
// from here http://geomalgorithms.com/a03-_inclusion.html
// a Point is defined by its coordinates {int x, y;}
//===================================================================
// isLeft(): tests if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2  on the line
//            <0 for P2  right of the line
//    See: Algorithm 1 "Area of Triangles and Polygons"
"int isLeft( point P0, point P1, ivec2 P2 ){\n"
//This can be used to detect an exact edge
"    return ( (P1.x - P0.x) * (P2.y - P0.y) - (P2.x -  P0.x) * (P1.y - P0.y) );\n"
"}\n"
// wn_PnPoly(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (=0 only when P is outside)
"uint wn_PnPoly( ivec2 P, point V[5]){\n"
"    uint wn = 0;\n"    // the  winding number counter
     // loop through all edges of the polygon
"    for (int i=0; i<4; i++) {\n"   // edge from V[i] to  V[i+1]
"        if (V[i].y <= P.y) {\n"          // start y <= P.y
"            if (V[i+1].y  > P.y)\n"      // an upward crossing
"                 if (isLeft( V[i], V[i+1], P) > 0)\n"  // P left of  edge
"                     ++wn;\n"            // have  a valid up intersect
"        }\n"
"        else {\n"                        // start y > P.y (no test needed)
"            if (V[i+1].y  <= P.y)\n"     // a downward crossing
"                 if (isLeft( V[i], V[i+1], P) < 0)\n"  // P right of  edge
"                     --wn;\n"            // have  a valid down intersect
"        }\n"
"    }\n"
"    return wn;\n"
"}\n"

"uint cn_PnPoly( ivec2 P, point V[5]){\n"
"  uint cn = 0;\n"    // the  crossing number counter
    // loop through all edges of the polygon
"    for (int i=0; i<4; i++) {\n"    // edge from V[i]  to V[i+1]
"       if (((V[i].y <= P.y) && (V[i+1].y > P.y))\n"     // an upward crossing
"        || ((V[i].y > P.y) && (V[i+1].y <=  P.y))) {\n" // a downward crossing
            // compute  the actual edge-ray intersect x-coordinate
"            float vt = float(P.y  - V[i].y) / (V[i+1].y - V[i].y);\n"
"            if (P.x <  V[i].x + vt * (V[i+1].x - V[i].x))\n" // P.x < intersect
"                 ++cn;\n"   // a valid crossing of y=P.y right of P.x
"        }\n"
"    }\n"
"    if ((cn%2) == 1) return 1u;\n"    // 0 if even (out), and 1 if  odd (in)
"    else return 0u;"
"}\n"
//===================================================================

"uint pixIsInside (ivec2 P, uint idx){\n"
"  point Quad[5];\n"
"  Quad[0].x = cmd[idx].P[0];\n"
"  Quad[0].y = cmd[idx].P[1];\n"
"  Quad[1].x = cmd[idx].P[2];\n"
"  Quad[1].y = cmd[idx].P[3];\n"
"  Quad[2].x = cmd[idx].P[4];\n"
"  Quad[2].y = cmd[idx].P[5];\n"
"  Quad[3].x = cmd[idx].P[6];\n"
"  Quad[3].y = cmd[idx].P[7];\n"
"  Quad[4].x = Quad[0].x;\n"
"  Quad[4].y = Quad[0].y;\n"

"  if (wn_PnPoly(P, Quad) != 0u) return 1u;\n"
//"  if (cn_PnPoly(P, Quad) == 1u) return 1u;\n"
"  else return 0u;\n"
"}\n"

"int getCmd(ivec2 P, uint id, uint start, uint end)\n"
"{\n"
"  for(uint i=id+start; i<id+end; i++) {\n"
"   if (pixIsInside(P, i) == 1u) {\n"
"     return int(i);\n"
"   }\n"
"  }\n"
"  return -1;\n"
"}\n"

"void main()\n"
"{\n"
"  uint discarded = 0;\n"
"  vec4 finalColor = vec4(0.0);\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.xy);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  ivec2 index = ivec2((texel.x*"Stringify(NB_COARSE_RAST_X)")/size.x, (texel.y*"Stringify(NB_COARSE_RAST_Y)")/size.y);\n"
"  uint lindex = index.y*"Stringify(NB_COARSE_RAST_X)"+ index.x;\n"
"  uint cmdIndex = lindex * 2000u;\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  if (nbCmd[lindex] == 0u) return;\n"
"    int cmdindex = getCmd(texel, cmdIndex, 0u, nbCmd[lindex]);\n"
"    if (cmdindex == -1) return;\n";

static const char vdp1_end_f[] =
"  imageStore(outSurface,texel,finalColor);\n"
"}\n";

static const char vdp1_test_f[] =
//"    else finalColor = vec4(float(cmd[cmdindex].P[1].x)/800.0, float(cmd[cmdindex].P[1].y)/480.0, 1.0, 1.0);\n"
"    uint col = color[(texel.y * colWidth) + texel.x];"
"    float a = float((col>>24u)&0xFFu)/255.0f;\n"
"    float b = float((col>>16u)&0xFFu)/255.0f;\n"
"    float g = float((col>>8u)&0xFFu)/255.0f;\n"
"    float r = float((col>>0u)&0xFFu)/255.0f;\n"
"    finalColor = vec4(r,g,b,a);\n";

#endif //VDP1_PROG_COMPUTE_H
