#ifndef VDP1_COMPUTE_H
#define VDP1_COMPUTE_H

enum
{
  TEST_PRG,
  NB_PRG
};

typedef struct
{
  int P[8];
} cmdparameter;

extern int vdp1_compute_init(int width, int height, float ratio);
extern int vdp1_compute();
extern int vdp1_add(cmdparameter* cmd);

#endif //VDP1_COMPUTE_H
