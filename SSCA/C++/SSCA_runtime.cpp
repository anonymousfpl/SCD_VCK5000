#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex>
#include <fftw3.h>
#include <map>
#include <cassert>
#include <algorithm>
#include <cstring>

#include "SSCA.h"
#include "fft.h"
#include "fftw.h"
#include "timer.h"
#include "utils.h"
#include "autossca.h"

using namespace std;
// Use the autossca library compate the L1 norm with different K for FFTW and Sparse FFT
/*
  x: the signal
  n: the length of x

  lobefrac_loc:   during location, the main lobe of the filter has half
                  width n*lobefrac_loc
  tolerance_loc:  the linf norm of the residuals of the filter
  b_loc:          the number of adjacent filters to add

  B_loc:          number of samples in subsampling during location
  B_thresh:       number of samples considered "heavy"
  loops_loc:      number of location loops
  loops_thresh:   number of times a coordinate must seem heavy.

  *_est:          ditto as above, but for estimation.

  repetitions:    repeat the experiment this many times for timing // no repetitions
  LARGE_FREQ:     locations of largest coefficients.
  k:              number of HHs, used for evaluation only.
  x_f:            true fft.
 */

int main()
{
    int N = 1048576, Np = 64;
    char str[60];
    sprintf(str, "dsss_noisefree_10dB_data.dat");
    int Nfile = 1048576;

    int fs = 1;
    real_t *Sx, *alphao, *freqo, *pxx;
    Sx = (real_t *)malloc(sizeof(real_t) * Np * 2 * N);
    alphao = (real_t *)malloc(sizeof(real_t) * 2 * N);
    freqo = (real_t *)malloc(sizeof(real_t) * Np);


    complex_t *x;
    x = (complex_t *)malloc(sizeof(complex_t) * (N + Np));

    if (N > Nfile)
    {

        int blocks = N / Nfile;
        printf("Extend signal with %d loops\n", blocks);
        for (int jj = 0; jj < blocks; jj++)
        {
            FILE *fp;
            fp = fopen(str, "r");
            if (fp == 0)
            {
                return -1;
            }

            // Read data from file
            double newvalr, newvali;
            for (int ii = 0; ii < Nfile; ii++)
            {
                if (fscanf(fp, "%lf\t%lf\n", &newvalr, &newvali) != 0)
                {
                    x[ii + Nfile * jj] = real_t(newvalr) + real_t(newvali) * I;
                }
                else
                {
                    return -2;
                }
            }
            fclose(fp);
        }
    }
    else
    {
        FILE *fp;
        fp = fopen(str, "r");
        if (fp == 0)
        {
            return -1;
        }
        // Read data from file
        double newvalr, newvali;
        for (int ii = 0; ii < N; ii++)
        {
            if (fscanf(fp, "%lf\t%lf\n", &newvalr, &newvali) != 0)
            {
                x[ii] = real_t(newvalr) + real_t(newvali) * I;
            }
            else
            {
                return -2;
            }
        }
        fclose(fp);
    }
    for (int ii = 0; ii < Np; ii++)
        x[ii + N] = x[ii];

    int *seeds;
    seeds = (int *)malloc(sizeof(int) * Np);

    reset_timer();
    double Tfftw = 0;
    autossca_fftw(x, fs, Np, N, Sx, alphao, freqo, seeds, Tfftw);
    printf("run_time_FFTW = %lf;\n", Tfftw);

    
    

    free(Sx);
    free(alphao);
    free(freqo);
    free(x);
    return 0;
}