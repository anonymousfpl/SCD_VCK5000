//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include "fft_subsys.h"
#include "ssca_sys.h"

#define PLFreq 520

using namespace adf ;


class dut: public graph {

public:

	input_plio      in[7];
	output_plio    out[6];

	ssca_fp_2_graph<23> ssca;

    dut(){
		in[  0]   = input_plio::create(  "scd_in_00a",   plio_64_bits, "data/cdp_test_00a.txt", PLFreq);
		in[  1]   = input_plio::create(  "scd_in_00b",   plio_64_bits, "data/cdp_test_00b.txt", PLFreq);
		in[  2]   = input_plio::create( "fft1_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
		in[  3]   = input_plio::create( "fft1_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);
		in[  4]   = input_plio::create( "fft2_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
		in[  5]   = input_plio::create( "fft2_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);
		in[  6]   = input_plio::create( "fft1_in_00c",   plio_64_bits, "data/fft_rotate_coeff.txt", PLFreq);
		out[ 0]  = output_plio::create( "scd_out_00a",   plio_64_bits,  "data/scd_out_00a.txt", PLFreq);
		out[ 1]  = output_plio::create( "scd_out_00b",   plio_64_bits,  "data/scd_out_00b.txt", PLFreq);
		out[ 2]  = output_plio::create("fft1_out_00a",   plio_64_bits, "data/fft1_out_00a.txt", PLFreq);
		out[ 3]  = output_plio::create("fft1_out_00b",   plio_64_bits, "data/fft1_out_00b.txt", PLFreq);
		out[ 4]  = output_plio::create("fft2_out_00a",   plio_64_bits, "data/fft2_out_00a.txt", PLFreq);
		out[ 5]  = output_plio::create("fft2_out_00b",   plio_64_bits, "data/fft2_out_00b.txt", PLFreq);

		for(int i=0; i<2; i++){
			connect<>(in[0+i].out[0],          ssca.cdp_in[i]);
			connect<>(in[2+i].out[0],       ssca.fft_s1_in[i]);
			connect<>(in[4+i].out[0],       ssca.fft_s2_in[i]);

			connect<>(ssca.cdp_out[i],    out[0+i].in[0]);
			connect<>(ssca.fft_s1_out[i], out[2+i].in[0]);
			connect<>(ssca.fft_s2_out[i], out[4+i].in[0]);
    	}
		connect<>(in[6].out[0],   ssca.fft_s1_in[2]);

	};

}; // end of class


// class dut: public graph {

// public:

// 	input_plio      in[5];
// 	output_plio    out[4];

// 	cdp_fft1_fp_2_graph<23> ssca;

//     dut(){
// 		in[  0]   = input_plio::create(  "scd_in_00a",   plio_64_bits, "data/cdp_test_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create(  "scd_in_00b",   plio_64_bits, "data/cdp_test_00b.txt", PLFreq);
// 		in[  2]   = input_plio::create( "fft1_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  3]   = input_plio::create( "fft1_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);
// 		in[  4]   = input_plio::create( "fft1_in_00c",   plio_64_bits, "data/fft_rotate_coeff.txt", PLFreq);
// 		out[ 0]  = output_plio::create( "scd_out_00a",   plio_64_bits,  "data/cdp_out_00a.txt", PLFreq);
// 		out[ 1]  = output_plio::create( "scd_out_00b",   plio_64_bits,  "data/cdp_out_00b.txt", PLFreq);
// 		out[ 2]  = output_plio::create("fft1_out_00a",   plio_64_bits, "data/fft1_out_00a.txt", PLFreq);
// 		out[ 3]  = output_plio::create("fft1_out_00b",   plio_64_bits, "data/fft1_out_00b.txt", PLFreq);

// 		for(int i=0; i<2; i++){
// 			connect<>(in[0+i].out[0],          ssca.cdp_in[i]);
// 			connect<>(in[2+i].out[0],       ssca.fft_s1_in[i]);

// 			connect<>(ssca.cdp_out[i],    out[0+i].in[0]);
// 			connect<>(ssca.fft_s1_out[i], out[2+i].in[0]);
//     	}
// 		connect<>(in[4].out[0],   ssca.fft_s1_in[2]);

// 	};

// }; // end of class


// class dut: public graph {

// public:

// 	input_plio      in[6];
// 	output_plio    out[6];

// 	ssca_fp_graph<23> ssca;

//     dut(){
// 		in[  0]   = input_plio::create(  "scd_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create(  "scd_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);
// 		in[  2]   = input_plio::create( "fft1_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  3]   = input_plio::create( "fft1_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);
// 		in[  4]   = input_plio::create( "fft2_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  5]   = input_plio::create( "fft2_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);
// 		out[ 0]  = output_plio::create( "scd_out_00a",   plio_64_bits,  "data/scd_out_00a.txt", PLFreq);
// 		out[ 1]  = output_plio::create( "scd_out_00b",   plio_64_bits,  "data/scd_out_00b.txt", PLFreq);
// 		out[ 2]  = output_plio::create("fft1_out_00a",   plio_64_bits, "data/fft1_out_00a.txt", PLFreq);
// 		out[ 3]  = output_plio::create("fft1_out_00b",   plio_64_bits, "data/fft1_out_00b.txt", PLFreq);
// 		out[ 4]  = output_plio::create("fft2_out_00a",   plio_64_bits, "data/fft2_out_00a.txt", PLFreq);
// 		out[ 5]  = output_plio::create("fft2_out_00b",   plio_64_bits, "data/fft2_out_00b.txt", PLFreq);

// 		for(int i=0; i<2; i++){
// 			connect<>(in[0+i].out[0],          ssca.cdp_in[i]);
// 			connect<>(in[2+i].out[0],       ssca.fft_s1_in[i]);
// 			connect<>(in[4+i].out[0],       ssca.fft_s2_in[i]);

// 			connect<>(ssca.cdp_out[i],    out[0+i].in[0]);
// 			connect<>(ssca.fft_s1_out[i], out[2+i].in[0]);
// 			connect<>(ssca.fft_s2_out[i], out[4+i].in[0]);
//     	}

// 	};

// }; // end of class

// // -----------------------------------------
// // window + FFT
// // -----------------------------------------
// class dut: public graph {

// public:

// 	input_plio      in[2];
// 	output_plio   out[1];


// 	ssr_ssca_win_fft_fp_16x64pt_graph<23> fft;
// 	// ssr_win_fp_16x64pt_graph<23> fft;

//     dut(){
// 		in[  0]   = input_plio::create( "fft_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create( "fft_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);

// 		out[0] = output_plio::create("fft_out_00a",  plio_64_bits, "data/fft_out_00a.txt", PLFreq);

// 		// out[1] = output_plio::create("fft_out_00b",  plio_64_bits, "data/fft_out_00b.txt", PLFreq);

// 		connect<>(in[0].out[0],     fft.in[0]);
// 		connect<>(in[1].out[0],     fft.in[1]);
// 		connect<>(fft.out[0], out[0].in[0]);

// 	};

// }; // end of class

// // ---------------------------
// // down conversion
// // ---------------------------
// class dut: public graph {

// public:

// 	input_plio      in[1];
// 	output_plio   out[1];


// 	ssr_dcm_fp_16x64pt_graph<23> fft;

//     dut(){
// 		in[  0]   = input_plio::create( "fft_in_00a",   plio_64_bits, "data/fft_golden_00a.txt", PLFreq);
// 		out[0] = output_plio::create("fft_out_00a",  plio_64_bits, "data/fft_out_00a.txt", PLFreq);

// 		connect<>(in[0].out[0],     fft.in[0]);
// 		connect<>(fft.out[0], out[0].in[0]);

// 	};

// }; // end of class

// // -----------------------------------------
// // window + FFT + DownConversion
// // -----------------------------------------
// class dut: public graph {

// public:

// 	input_plio      in[2];
// 	output_plio   out[1];


// 	ssr_ssca_win_fft_dc_fp_16x64pt_graph<23> fft;

//     dut(){
// 		in[  0]   = input_plio::create( "fft_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create( "fft_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);

// 		out[0] = output_plio::create("fft_out_00a",  plio_64_bits, "data/fft_out_00a.txt", PLFreq);

// 		connect<>(in[0].out[0],     fft.in[0]);
// 		connect<>(in[1].out[0],     fft.in[1]);
// 		connect<>(fft.out[0], out[0].in[0]);

// 	};

// }; // end of class

// // -----------------------------------------
// // window + cx
// // -----------------------------------------
// class dut: public graph {

// public:

// 	input_plio      in[2];
// 	output_plio   out[2];


// 	ssr_cx_fp_16x64pt_graph<23> fft;

//     dut(){
// 		in[  0]   = input_plio::create( "fft_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create( "fft_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);

// 		out[0] = output_plio::create("fft_out_00a",  plio_64_bits, "data/fft_out_00a.txt", PLFreq);
// 		out[1] = output_plio::create("fft_out_00b",  plio_64_bits, "data/fft_out_00b.txt", PLFreq);

// 		connect<>(in[0].out[0],     fft.in[0]);
// 		connect<>(in[1].out[0],     fft.in[1]);
// 		connect<>(fft.out[0], out[0].in[0]);
// 		connect<>(fft.out[1], out[1].in[0]);

// 	};

// }; // end of class

// // -----------------------------------------
// //  DownConversion + cx
// // -----------------------------------------
// class dut: public graph {

// public:

// 	input_plio      in[2];
// 	output_plio   out[2];


// 	ssr_dcm_cx_fp_16x64pt_graph<23> fft;

//     dut(){
// 		in[  0]   = input_plio::create( "fft_in_00a",   plio_64_bits, "data/fft_golden_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create( "fft_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);

// 		out[0] = output_plio::create("fft_out_00a",  plio_64_bits, "data/fft_out_00a.txt", PLFreq);
// 		out[1] = output_plio::create("fft_out_00b",  plio_64_bits, "data/fft_out_00b.txt", PLFreq);

// 		connect<>(in[0].out[0],     fft.in[0]);
// 		connect<>(in[1].out[0],     fft.in[1]);
// 		connect<>(fft.out[0], out[0].in[0]);
// 		connect<>(fft.out[1], out[1].in[0]);

// 	};

// }; // end of class

// // -----------------------------------------
// // window + FFT + DownConversion + cx
// // -----------------------------------------
// class dut: public graph {

// public:

// 	input_plio      in[2];
// 	output_plio   out[2];


// 	ssr_ssca_win_fft_dc_cx_fp_16x64pt_graph<23> fft;

//     dut(){
// 		in[  0]   = input_plio::create( "fft_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create( "fft_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);

// 		out[0] = output_plio::create("fft_out_00a",  plio_64_bits, "data/fft_out_00a.txt", PLFreq);
// 		out[1] = output_plio::create("fft_out_00b",  plio_64_bits, "data/fft_out_00b.txt", PLFreq);

// 		connect<>(in[0].out[0],     fft.in[0]);
// 		connect<>(in[1].out[0],     fft.in[1]);
// 		connect<>(fft.out[0], out[0].in[0]);
// 		connect<>(fft.out[1], out[1].in[0]);

// 	};

// }; // end of class

// // -----------------------------------------
// // window + FFT + DownConversion + cm
// // -----------------------------------------
// class dut: public graph {

// public:

// 	input_plio      in[2];
// 	output_plio   out[2];


// 	ssr_ssca_win_fft_dc_cm_fp_16x64pt_graph<23> fft;

//     dut(){
// 		in[  0]   = input_plio::create( "fft_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create( "fft_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);

// 		out[0] = output_plio::create("fft_out_00a",  plio_64_bits, "data/fft_out_00a.txt", PLFreq);
// 		out[1] = output_plio::create("fft_out_00b",  plio_64_bits, "data/fft_out_00b.txt", PLFreq);

// 		connect<>(in[0].out[0],     fft.in[0]);
// 		connect<>(in[1].out[0],     fft.in[1]);
// 		connect<>(fft.out[0], out[0].in[0]);
// 		connect<>(fft.out[1], out[1].in[0]);

// 	};

// }; // end of class

// // -----------------------------------------
// // cdp
// // window + FFT + DownConversion + cm  stream
// // -----------------------------------------
// class dut: public graph {

// public:

// 	input_plio      in[2];
// 	output_plio   out[2];

// 	ssr_ssca_win_fft_dc_cm_stream2_fp_16x64pt_graph<23> fft;

//     dut(){
// 		in[  0]   = input_plio::create( "scd_in_00a",   plio_64_bits, "data/cdp_test_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create( "scd_in_00b",   plio_64_bits, "data/cdp_test_00b.txt", PLFreq);

// 		out[0] = output_plio::create("scd_out_00a",  plio_64_bits, "data/cdp_out_00a.txt", PLFreq);
// 		out[1] = output_plio::create("scd_out_00b",  plio_64_bits, "data/cdp_out_00b.txt", PLFreq);

// 		connect<>(in[0].out[0],     fft.cdp_in[0]);
// 		connect<>(in[1].out[0],     fft.cdp_in[1]);
// 		connect<>(fft.cdp_out[0], out[0].in[0]);
// 		connect<>(fft.cdp_out[1], out[1].in[0]);

// 	};

// }; // end of class


// // -----------------------------------------
// // FFT1 + rotate
// // -----------------------------------------
// class dut: public graph {

// public:

// 	input_plio      in[3];
// 	output_plio   out[2];

// 	ssr_fft_fp_1kpt_graph_rotate_2<23> fft;
// 	// ssr_fft_fp_1kpt_graph_rotate<23> fft;

//     dut(){
// 		in[  0]   = input_plio::create( "fft1_in_00a",   plio_64_bits, "data/fft_test_00a.txt", PLFreq);
// 		in[  1]   = input_plio::create( "fft1_in_00b",   plio_64_bits, "data/fft_test_00b.txt", PLFreq);
// 		in[  2]   = input_plio::create( "fft1_in_00c",   plio_64_bits, "data/fft_rotate_coeff.txt", PLFreq);

// 		out[0] = output_plio::create("fft1_out_00a",  plio_64_bits, "data/fft_out_00a.txt", PLFreq);
// 		out[1] = output_plio::create("fft1_out_00b",  plio_64_bits, "data/fft_out_00b.txt", PLFreq);

// 		connect<>(in[0].out[0],     fft.in[0]);
// 		connect<>(in[1].out[0],     fft.in[1]);
// 		connect<>(in[2].out[0],     fft.in[2]);
// 		connect<>(fft.out[0], out[0].in[0]);
// 		connect<>(fft.out[1], out[1].in[0]);

// 	};

// }; // end of class