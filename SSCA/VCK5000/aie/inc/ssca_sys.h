#ifndef __SSCA_SYS_H__
#define __SSCA_SYS_H__

#include <adf.h>

#include "fft_subsys.h"

using namespace adf;

template<int xoff>
class ssr_fft_rts_graph: public graph {
public:
  kernel core[1];
  port<input> in[2];
  port<output> out[2];

  ssr_fft_rts_graph(void) {
    // Create FFT kernels
    core[0] = kernel::create_object<ssr_fft_fp_rotate>(std::vector<int>{TWIDROT00},std::vector<int>(2048));
    runtime<ratio>(core[0]) = 0.8;
    source(core[0]) = "ssr_fft_fp_rotate.cpp";

// 设定一种链接方式
#define	TWD_CONN(coreid, ioid)	connect<>(in[2*(ioid)+0], core[coreid].in[0]); \
                                connect<>(in[2*(ioid)+1], core[coreid].in[1]);         \
				connect<>(core[coreid].out[0], out[2*(ioid)+0]); \
				connect<>(core[coreid].out[1], out[2*(ioid)+1]);

// 设定位置和分配的坐标
#define TWD_LOC(coreid, xloc, yoff)  location<kernel>   (core[coreid])          =  tile(xloc+2, yoff+1); \
                                     location<stack>    (core[coreid])          =  bank(xloc+2, yoff+1, 3); \
				     location<parameter>(core[coreid].param[1]) =  bank(xloc+2, yoff,   2); \
				     location<parameter>(core[coreid].param[0]) =  bank(xloc+2, yoff,   3); \
				     location<buffer>(core[coreid].out[0])      = {address(xloc+2, yoff, 0x0000), address(xloc+2, yoff, 0x2000)}; \
				     location<buffer>(core[coreid].out[1])      = {address(xloc+2, yoff, 0x1000), address(xloc+2, yoff, 0x3000)};

    TWD_CONN(0, 0);  
    TWD_LOC(0,    xoff ,        0);

#undef 	TWD_LOC
#undef 	TWD_CONN
  };
};


template<int xoff>
class ssr_fft_rts_2_graph: public graph {
public:
  kernel core[1];
  port<input> in[3];
  port<output> out[2];

  ssr_fft_rts_2_graph(void) {
    // Create FFT kernels
    core[0] = kernel::create_object<ssr_fft_fp_rotate_2>(std::vector<int>(2048));
    runtime<ratio>(core[0]) = 0.8;
    source(core[0]) = "ssr_fft_fp_rotate_2.cpp";

// 设定一种链接方式
#define	TWD_CONN(coreid, ioid)	connect<>(in[2*(ioid)+0], core[coreid].in[0]); \
                                connect<>(in[2*(ioid)+1], core[coreid].in[1]);         \
                                connect<>(in[2*(ioid)+2], core[coreid].in[2]); \
				connect<>(core[coreid].out[0], out[2*(ioid)+0]); \
				connect<>(core[coreid].out[1], out[2*(ioid)+1]);

// 设定位置和分配的坐标
#define TWD_LOC(coreid, xloc, yoff)  location<kernel>   (core[coreid])          =  tile(xloc+2, yoff+1); \
                                     location<stack>    (core[coreid])          =  bank(xloc+2, yoff+1, 3); \
				     location<parameter>(core[coreid].param[0]) =  bank(xloc+2, yoff,   2); \
				     location<buffer>(core[coreid].out[0])      = {address(xloc+2, yoff, 0x0000), address(xloc+2, yoff, 0x2000)}; \
				     location<buffer>(core[coreid].out[1])      = {address(xloc+2, yoff, 0x1000), address(xloc+2, yoff, 0x3000)};

    TWD_CONN(0, 0);  
    TWD_LOC(0,    xoff ,        0);

#undef 	TWD_LOC
#undef 	TWD_CONN
  };
};


template<int xoff>
class ssr_fft_fp_1kpt_graph_rotate: public graph {
// 1k-pt FFT with rotate factors; ssr_fft_fp_1kpt_graph_x4 contains 4 1k FFT; dds is rotate factors.
public:

  ssr_fft_fp_1kpt_graph<xoff,0>  k0;
  ssr_fft_rts_graph<xoff>       trs;

  port<input>  in[2];
  port<output> out[2];


  ssr_fft_fp_1kpt_graph_rotate() {

    for(int i=0; i<2; i++){
      connect<>(in[i], k0.in[i]);
      connect<>(k0.out[i], trs.in[i]);
      connect<>(trs.out[i], out[i]);
    }
  };
};


template<int xoff>
class ssr_fft_fp_1kpt_graph_rotate_2: public graph {
// 1k-pt FFT with rotate factors; ssr_fft_fp_1kpt_graph_x4 contains 4 1k FFT; dds is rotate factors.
public:

  ssr_fft_fp_1kpt_graph<xoff,0>  k0;
  ssr_fft_rts_2_graph<xoff>       trs;

  port<input>  in[3];
  port<output> out[2];


  ssr_fft_fp_1kpt_graph_rotate_2() {

    for(int i=0; i<2; i++){
      connect<>(in[i], k0.in[i]);
      connect<>(k0.out[i], trs.in[i]);
      connect<>(trs.out[i], out[i]);
    }
    connect<>(in[2], trs.in[2]);
  };
};


template<int xoff>
class ssr_fft_fp_ssca_graph: public graph {
public:

  ssr_fft_fp_1kpt_graph_rotate<xoff>      ffta;
  ssr_fft_fp_1kpt_graph<xoff+3,0>    fftb;

  port<input>        in[2];
  port<output>  mid_out[2];
  port<input>   mid_in[ 2];
  port<output>     out[ 2];

  ssr_fft_fp_ssca_graph() {

    for(int i=0; i<2; i++){
      connect<>(in[i], ffta.in[i]);
      connect<>(ffta.out[i], mid_out[i]);
      connect<>(mid_in[i],  fftb.in[i]);
      connect<>(fftb.out[i], out[i]);
    }
  };
};

// ------------------
// input[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]
// input[1]:fft1[0]/fft3[0]/fft5[0]/ffy7[0]
// output[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]/fft1[0]/fft3[0]/fft5[0]/ffy7[0]....
//           fft8[0]/fft10[0]/fft12[0]/fft14[0]/fft9[0]/fft11[0]/fft13[0]/ffy15[0]....
// ------------------
// Just 64 point FFT: tested corrected
template<int xoff>
class ssr_fft_fp_16x64pt_graph: public graph {
public:

  ssr_fft_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        in[2];
  port<output>     out[ 1];

  ssr_fft_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(in[1], ffta.in[1]);
    connect<>(ffta.out[0], out[0]);
  };
};

// ------------------
// input[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]
// input[1]:fft1[0]/fft3[0]/fft5[0]/ffy7[0]
// output[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]/fft1[0]/fft3[0]/fft5[0]/ffy7[0]....
//           fft8[0]/fft10[0]/fft12[0]/fft14[0]/fft9[0]/fft11[0]/fft13[0]/ffy15[0]....
// ------------------
// 64 point FFT with window function
template<int xoff>
class ssr_ssca_win_fft_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_fft_fp_64pt_x16_graph<xoff,0>      ffta;
  // ssr_ssca_fft_fp_64pt_x16_graph_test<xoff,0>      ffta;

  port<input>        in[2];
  port<output>     out[ 1];

  ssr_ssca_win_fft_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(in[1], ffta.in[1]);
    connect<>(ffta.out[0], out[0]);
  };
};

// ------------------
// input[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]
// input[1]:fft1[0]/fft3[0]/fft5[0]/ffy7[0]
// output[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]/fft1[0]/fft3[0]/fft5[0]/ffy7[0]....
//           fft8[0]/fft10[0]/fft12[0]/fft14[0]/fft9[0]/fft11[0]/fft13[0]/ffy15[0]....
// ------------------
// window function+ 64 point FFT + DownConversion 
template<int xoff>
class ssr_ssca_win_fft_dc_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_win_fft_dc_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        in[2];
  port<output>     out[ 1];

  ssr_ssca_win_fft_dc_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(in[1], ffta.in[1]);
    connect<>(ffta.out[0], out[0]);
  };
};

// ------------------
// input[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]
// input[1]:fft1[0]/fft3[0]/fft5[0]/ffy7[0]
// output[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]/fft1[0]/fft3[0]/fft5[0]/ffy7[0]....
//           fft8[0]/fft10[0]/fft12[0]/fft14[0]/fft9[0]/fft11[0]/fft13[0]/ffy15[0]....
// ------------------
// window function+ 64 point FFT + DownConversion + cx
template<int xoff>
class ssr_ssca_win_fft_dc_cx_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_win_fft_dc_cx_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        in[2];
  port<output>     out[ 2];

  ssr_ssca_win_fft_dc_cx_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(in[1], ffta.in[1]);
    connect<>(ffta.out[0], out[0]);
    connect<>(ffta.out[1], out[1]);
  };
};

// ------------------
// input[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]
// input[1]:fft1[0]/fft3[0]/fft5[0]/ffy7[0]
// output[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]/fft1[0]/fft3[0]/fft5[0]/ffy7[0]....
//           fft8[0]/fft10[0]/fft12[0]/fft14[0]/fft9[0]/fft11[0]/fft13[0]/ffy15[0]....
// ------------------
// window function+ 64 point FFT + DownConversion + cx
template<int xoff>
class ssr_ssca_win_fft_dc_cm_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_win_fft_dc_cm_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        in[2];
  port<output>     out[ 2];

  ssr_ssca_win_fft_dc_cm_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(in[1], ffta.in[1]);
    connect<>(ffta.out[0], out[0]);
    connect<>(ffta.out[1], out[1]);
  };
};

// ------------------
// input[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]
// input[1]:fft1[0]/fft3[0]/fft5[0]/ffy7[0]
// output[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]/fft1[0]/fft3[0]/fft5[0]/ffy7[0]....
//           fft8[0]/fft10[0]/fft12[0]/fft14[0]/fft9[0]/fft11[0]/fft13[0]/ffy15[0]....
// ------------------
// window function+ 64 point FFT + DownConversion + cx stream
template<int xoff>
class ssr_ssca_win_fft_dc_cm_stream_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_win_fft_dc_cm_stream_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        in[2];
  port<output>     out[ 2];

  ssr_ssca_win_fft_dc_cm_stream_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(in[1], ffta.in[1]);
    connect<>(ffta.out[0], out[0]);
    connect<>(ffta.out[1], out[1]);
  };
};

// ------------------
// input[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]
// input[1]:fft1[0]/fft3[0]/fft5[0]/ffy7[0]
// output[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]/fft1[0]/fft3[0]/fft5[0]/ffy7[0]....
//           fft8[0]/fft10[0]/fft12[0]/fft14[0]/fft9[0]/fft11[0]/fft13[0]/ffy15[0]....
// ------------------
// window function+ 64 point FFT + DownConversion + cx stream
template<int xoff>
class ssr_ssca_win_fft_dc_cm_stream2_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_win_fft_dc_cm_stream2_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        cdp_in[2];
  port<output>     cdp_out[ 2];

  ssr_ssca_win_fft_dc_cm_stream2_fp_16x64pt_graph() {
    connect<>(cdp_in[0], ffta.in[0]);
    connect<>(cdp_in[1], ffta.in[1]);
    connect<>(ffta.out[0], cdp_out[0]);
    connect<>(ffta.out[1], cdp_out[1]);
  };
};

// ------------------
// input[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]
// input[1]:fft1[0]/fft3[0]/fft5[0]/ffy7[0]
// output[0]:fft0[0]/fft2[0]/fft4[0]/fft6[0]/fft1[0]/fft3[0]/fft5[0]/ffy7[0]....
//           fft8[0]/fft10[0]/fft12[0]/fft14[0]/fft9[0]/fft11[0]/fft13[0]/ffy15[0]....
// ------------------
// 64 point FFT with window function 2in 2 out
template<int xoff>
class ssr_ssca_fft_separate_fp_16x64pt_graph: public graph {
public:
  ssr_ssca_win_fp_64pt_x16_graph<xoff-2,0>      wind;
  ssr_fft_fp_64pt_x16_graph<xoff,0>           ffta;

  port<input>        in[4];
  port<output>     out[ 3];

  ssr_ssca_fft_separate_fp_16x64pt_graph() {
    connect<>(in[0], wind.in[0]);
    connect<>(in[1], wind.in[1]);
    connect<>(in[2], ffta.in[0]);
    connect<>(in[3], ffta.in[1]);
    connect<>(wind.out[0], out[0]);
    connect<>(wind.out[1], out[1]);
    connect<>(ffta.out[0], out[2]);
  };
};

// window function
template<int xoff>
class ssr_win_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_win_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        in[2];
  port<output>     out[ 2];

  ssr_win_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(in[1], ffta.in[1]);
    connect<>(ffta.out[0], out[0]);
    connect<>(ffta.out[1], out[1]);
  };
};

// dc function
template<int xoff>
class ssr_dcm_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_dcm_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        in[1];
  port<output>     out[ 1];

  ssr_dcm_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(ffta.out[0], out[0]);
  };
};


// dc function + cx
template<int xoff>
class ssr_dcm_cx_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_dcm_cx_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        in[2];
  port<output>     out[ 2];

  ssr_dcm_cx_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(in[1], ffta.in[1]);
    connect<>(ffta.out[0], out[0]);
    connect<>(ffta.out[1], out[1]);
  };
};

// conjugate x function
template<int xoff>
class ssr_cx_fp_16x64pt_graph: public graph {
public:

  ssr_ssca_cx_fp_64pt_x16_graph<xoff,0>      ffta;

  port<input>        in[2];
  port<output>     out[ 2];

  ssr_cx_fp_16x64pt_graph() {
    connect<>(in[0], ffta.in[0]);
    connect<>(in[1], ffta.in[1]);
    connect<>(ffta.out[0], out[0]);
    connect<>(ffta.out[1], out[1]);
  };
};

template<int xoff>
class ssca_fp_graph: public graph {
public:
  ssr_ssca_win_fft_dc_cm_stream2_fp_16x64pt_graph<xoff> cdp;
  ssr_fft_fp_1kpt_graph_rotate<xoff+3>                  ffta;
  ssr_fft_fp_1kpt_graph<xoff+6,0>                       fftb;


  port<input>           cdp_in[2];
  port<output>         cdp_out[2];
  port<input>        fft_s1_in[2];
  port<output>      fft_s1_out[2];
  port<input>        fft_s2_in[2];
  port<output>      fft_s2_out[2];

  ssca_fp_graph() {

    for(int i=0; i<2; i++){
      connect<>(cdp_in[i],       cdp.cdp_in[i]);
      connect<>(cdp.cdp_out[i],     cdp_out[i]);
      connect<>(fft_s1_in[i],   ffta.in[i]);
      connect<>(ffta.out[i], fft_s1_out[i]);
      connect<>(fft_s2_in[i],   fftb.in[i]);
      connect<>(fftb.out[i], fft_s2_out[i]);
    }
  };
};

template<int xoff>
class cdp_fft1_fp_2_graph: public graph {
public:
  ssr_ssca_win_fft_dc_cm_stream2_fp_16x64pt_graph<xoff> cdp;
  ssr_fft_fp_1kpt_graph_rotate_2<xoff+3>                ffta;


  port<input>           cdp_in[2];
  port<output>         cdp_out[2];
  port<input>        fft_s1_in[3];
  port<output>      fft_s1_out[2];

  cdp_fft1_fp_2_graph() {

    for(int i=0; i<2; i++){
      connect<>(cdp_in[i],       cdp.cdp_in[i]);
      connect<>(cdp.cdp_out[i],     cdp_out[i]);
      connect<>(fft_s1_in[i],   ffta.in[i]);
      connect<>(ffta.out[i], fft_s1_out[i]);
    }
    connect<>(fft_s1_in[2], ffta.in[2]);
  };
};

template<int xoff>
class ssca_fp_2_graph: public graph {
public:
  ssr_ssca_win_fft_dc_cm_stream2_fp_16x64pt_graph<xoff> cdp;
  ssr_fft_fp_1kpt_graph_rotate_2<xoff+3>                ffta;
  ssr_fft_fp_1kpt_graph<xoff+6,0>                       fftb;


  port<input>           cdp_in[2];
  port<output>         cdp_out[2];
  port<input>        fft_s1_in[3];
  port<output>      fft_s1_out[2];
  port<input>        fft_s2_in[2];
  port<output>      fft_s2_out[2];

  ssca_fp_2_graph() {

    for(int i=0; i<2; i++){
      connect<>(cdp_in[i],       cdp.cdp_in[i]);
      connect<>(cdp.cdp_out[i],     cdp_out[i]);
      connect<>(fft_s1_in[i],   ffta.in[i]);
      connect<>(ffta.out[i], fft_s1_out[i]);
      connect<>(fft_s2_in[i],   fftb.in[i]);
      connect<>(fftb.out[i], fft_s2_out[i]);
    }
    connect<>(fft_s1_in[2], ffta.in[2]);
  };
};

#endif //__SSCA_SYS_H__

