#ifndef VDP1_COMPUTE_H
#define VDP1_COMPUTE_H

enum
{
  TEST_PRG,
  NB_PRG
};

typedef struct
{
  unsigned int coord[8];
} cmdparameter_struct;

extern int vdp1_compute_init(int width, int height);
extern int vdp1_compute();
extern int vdp1add(int Ax, int Ay, int Bx, int By, int Cx, int Cy, int Dx, int Dy);

#endif //VDP1_COMPUTE_H
