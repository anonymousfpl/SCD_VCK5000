#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex>
#include <fftw3.h>
#include <map>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <unordered_map>

#include "SSCA.h"
#include "fft.h"
#include "fftw.h"
#include "timer.h"
#include "utils.h"
#include "autossca.h"

using namespace std;
int IDXS;
int IDXSPRE;
// g++ -Wall -Wconversion -O2   -c -o autossca.o autossca.cpp
// g++ -Wall -Wconversion -O2  -g  SSCA_co2.cpp parameters.o timer.o plot.o fftw.o computefourier.o filters.o utils.o autossca.o  -lfftw3 -lfftw3f -lm -lrt -o SSCA_co2


void autossca_fftw(complex_t *x, int fs, int Np, int N,
                   real_t *Sx, real_t *alphao, real_t *fo, int *seeds, double &TFFT)
{
    int i, j, k, m, idxfreq, idxalpha;
    complex_t *XW, *XF1, *Et, *XD, *xc, *XM;
    real_t *XF2;
    float alpha, f;
    printf("Test autossca_fftw\n");


    if (Np < 2)
    {
        printf("Error: Np must be at least 2.\n");
        exit(1);
    }

    if (N < Np)
    {
        printf("Error: N must be at least Np.\n");
        exit(1);
    }

    if (fs <= 0)
    {
        printf("Error: fs must be greater than zero.\n");
        exit(1);
    }
    /*
    *******************
    Windowing
    *******************
    */
    XW = (complex_t *)malloc(sizeof(complex_t) * Np * N);
    double Dwin = get_time();
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < Np; j++)
        {
            XW[j + i * Np] = x[i + j] * chebwin_128[j];
        }
    }
    Dwin = get_time() - Dwin;

    /*
    *******************
    First FFT
    *******************
    */
    double DFFT1 = get_time();
    complex_t *XFx1, *XFf1;
    XFx1 = (complex_t *)malloc(sizeof(complex_t) * Np);
    XFf1 = (complex_t *)malloc(sizeof(complex_t) * Np);

    fftw_plan p;

    p = fftw_plan_dft_1d(Np, reinterpret_cast<real_t(*)[2]>(XFx1), // in
                         reinterpret_cast<real_t(*)[2]>(XFf1),     // out
                         FFTW_FORWARD, FFTW_MEASURE);
    for (j = 0; j < N; j++)
    {
        // fftw_dft(XFf1, n, XFx1,0);
        for (i = 0; i < Np; i++)
        {
            XFx1[i] = XW[i + j * Np];
        }
        fftw_execute(p);
        for (i = 0; i < Np / 2; i++)
        {
            XW[i + j * Np] = XFf1[i + Np / 2]; // with frequency shift
        }
        for (i = Np / 2; i < Np; i++)
        {
            XW[i + j * Np] = XFf1[i - Np / 2]; // with frequency shift
        }
    }
    fftw_destroy_plan(p);
    DFFT1 = get_time() - DFFT1;
    free(XFx1);
    free(XFf1);
    // free(XW);

    /*
*******************************************
Down Conversion + Conjugate Multiplication
*******************************************
*/

    double DDC = get_time();
    Et = (complex_t *)malloc(sizeof(complex_t) * Np);

    for (k = 0; k < Np; k++)
    {
        real_t dex = real_t(k) / Np;
        Et[k] = cos(-2.0 * M_PI * dex) + I * sin(-2.0 * M_PI * dex);
    }

    for (m = 0; m < N; m++)
    {
        complex_t xc = conj(x[m + Np / 2]);
        for (k = 0; k < Np; k++)
        {
            XW[k + m * Np] = Et[(((k + Np / 2) % Np) * m) % Np] * XW[k + m * Np] * xc;
        }
    }
    free(Et);

    DDC = get_time() - DDC;
    /*
    ************************
    Second FFT
    ************************
    */
    double DFFT2 = get_time();
    complex_t *XFx2, *XFf2;
    XFx2 = (complex_t *)malloc(sizeof(complex_t) * N);
    XFf2 = (complex_t *)malloc(sizeof(complex_t) * N);

    fftw_plan p2;
    p2 = fftw_plan_dft_1d(N, reinterpret_cast<real_t(*)[2]>(XFx2), // in
                          reinterpret_cast<real_t(*)[2]>(XFf2),    // out
                          FFTW_FORWARD, FFTW_ESTIMATE);
    for (j = 0; j < Np; j++)
    {

        for (i = 0; i < N; i++)
        {
            XFx2[i] = XW[i * Np + j];
        }
        double DDD = get_time();
        fftw_execute(p2);
        DDD = get_time() - DDD;
        TFFT += DDD;

        for (i = 0; i < N / 2; i++)
        {
            XW[i * Np + j] = XFf2[i + N / 2] / N;
        }
        for (i = N / 2; i < N; i++)
        {
            XW[i * Np + j] = XFf2[i - N / 2] / N;
        }
    }
    fftw_destroy_plan(p2);
    free(XFx2);
    free(XFf2);
    DFFT2 = get_time() - DFFT2;
    /*
    ************************
    SCD Matrix
    ************************
    */
    double DMAP = get_time();
    double alpha_start = -1.0;
    double alpha_step = 1.0 / N;
    for (i = 0; i < 2 * N; i++)
        alphao[i] = alpha_start + i * alpha_step;
    double freq_start = -0.5;
    double freq_step = 1.0 / Np;
    for (i = 0; i < Np; i++)
        fo[i] = freq_start + i * freq_step;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < Np; j++)
        {
            alpha = ((float)i) / N + ((float)j) / Np;
            f = (((float)j) / Np - ((float)i) / N) / 2;
            idxfreq = round(Np * (f + 0.5));
            idxalpha = round(N * (alpha));
            Sx[idxfreq + idxalpha * Np] = (real_t)cabs(XW[i * Np + j]);
        }
    }
    DMAP = get_time() - DMAP;
    double TimeTotal = (Dwin + DFFT1 + DDC + DFFT2 + DMAP) / 100.0;
    printf("N = %d, Np = %d\n", N, Np);
    printf("Time distribution:  Window     FirstFFT    DownConversion+Multi    SecondFFT     Map \n");
    printf("                    %lf,  %lf,   %lf,             %lf,    %lf\n", Dwin / TimeTotal, DFFT1 / TimeTotal, DDC / TimeTotal, DFFT2 / TimeTotal, DMAP / TimeTotal);
    printf("FFTW in Second FFT is %lf\n", TFFT / DFFT2 * 100);

    free(XW);
}
