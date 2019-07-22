#ifndef VDP1_PROG_COMPUTE_H
#define VDP1_PROG_COMPUTE_H

#include "standard_compute.h"

#define QuoteIdent(ident) #ident
#define Stringify(macro) QuoteIdent(macro)

#define NB_COARSE_RAST_X 8
#define NB_COARSE_RAST_Y 8

static const char vdp1_start_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"layout(local_size_x = 8, local_size_y = 8) in;\n"
"layout(rgba8, binding = 0) writeonly highp uniform image2D outSurface;\n"
"struct point{\n"
"  int x;\n"
"  int y;\n"
"};\n"
"struct cmdparameter_struct{ \n"
"  int P[8];\n"
"};\n"
"layout(std430, binding = 1) readonly buffer NB_CMD { uint nbCmd[]; };\n"
"layout(std430, binding = 2) readonly buffer CMD { \n"
"  cmdparameter_struct cmd[];\n"
"};\n"

"ivec2 texel = ivec2(0,0);\n"
"uint index = 0;\n"
"uint discarded = 0;\n"

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
"    return ( (P1.x - P0.x) * (P2.y - P0.y) - (P2.x -  P0.x) * (P1.y - P0.y) );\n"
"}\n"
// wn_PnPoly(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (=0 only when P is outside)
"int wn_PnPoly( ivec2 P, point V[5]){\n"
"    int wn = 0;\n"    // the  winding number counter
     // loop through all edges of the polygon
"    for (int i=0; i<5; i++) {\n"   // edge from V[i] to  V[i+1]
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
#if 0
int wn_PnPoly2(Point P, vector<Point> V, int n)
{
    int    wn = 0;    // the  winding number counter
                      // loop through all edges of the polygon
    for (int i = 0; i<n; i++) {   // edge from V[i] to  V[i+1]
        if (V[i].Y <= P.Y) {          // start y <= P.y
            if (V[i + 1].Y  > P.Y)      // an upward crossing
            {
                int l = isLeft(V[i], V[i + 1], P);
                if (l > 0)  // P left of  edge
                    ++wn;            // have  a valid up intersect
                else if (l == 0) // boundary
                    return 0;
            }
        }
        else {                        // start y > P.y (no test needed)
            if (V[i + 1].Y <= P.Y)     // a downward crossing
            {
                int l = isLeft(V[i], V[i + 1], P);
                if (l < 0)  // P right of  edge
                    --wn;            // have  a valid down intersect
                else if (l == 0)
                    return 0;
            }
        }
    }
    return wn;
}
#endif
"int cn_PnPoly( ivec2 P, point V[5]){\n"
"  int cn = 0;\n"    // the  crossing number counter
    // loop through all edges of the polygon
"    for (int i=0; i<5; i++) {\n"    // edge from V[i]  to V[i+1]
"       if (((V[i].y <= P.y) && (V[i+1].y > P.y))\n"     // an upward crossing
"        || ((V[i].y > P.y) && (V[i+1].y <=  P.y))) {\n" // a downward crossing
            // compute  the actual edge-ray intersect x-coordinate
"            float vt = float(P.y  - V[i].y) / (V[i+1].y - V[i].y);\n"
"            if (P.x <  V[i].x + vt * (V[i+1].x - V[i].x))\n" // P.x < intersect
"                 ++cn;\n"   // a valid crossing of y=P.y right of P.x
"        }\n"
"    }\n"
"    return (cn&1);\n"    // 0 if even (out), and 1 if  odd (in)
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
"  if (wn_PnPoly(P, Quad) != 0) return 1u;\n"
//"  if (cn_PnPoly(P, Quad) == 1) return 1u;\n"
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
"  vec4 finalColor = vec4(0.0);\n"
"  texel = ivec2(gl_GlobalInvocationID.xy);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  index = (texel.x / (size.x/"Stringify(NB_COARSE_RAST_X)")) + (texel.y / (size.y/"Stringify(NB_COARSE_RAST_Y)"))*"Stringify(NB_COARSE_RAST_X)";\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  if (nbCmd[index] == 0u) discarded = 2;\n"
"  if (discarded == 0) {\n"
"    int cmdindex = getCmd(texel, index*2000u, 0u, nbCmd[index]);\n"
"    if (cmdindex == -1) discarded = 1;\n";

static const char vdp1_end_f[] =
"  if (discarded == 1) finalColor = vec4(0.5);\n"
"  if (discarded == 2) finalColor = vec4(0.2);\n"
"  imageStore(outSurface,texel,finalColor);\n"
"}\n";

static const char vdp1_test_f[] =
//"    else finalColor = vec4(float(cmd[cmdindex].P[1].x)/800.0, float(cmd[cmdindex].P[1].y)/480.0, 1.0, 1.0);\n"
"    else finalColor = vec4(1.0);\n"
"  }\n";

#endif //VDP1_PROG_COMPUTE_H
