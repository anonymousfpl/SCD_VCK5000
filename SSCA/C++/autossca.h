#ifndef AUTOSSCA_H_
#define AUTOSSCA_H_
#include "fft.h"

extern int IDXS;
extern int IDXSPRE;

void autossca_fftw(complex_t *x, int fs, int Np, int N,
                   real_t *Sx, real_t *alphao, real_t *fo, int *seeds, double &TFFT);



#endif