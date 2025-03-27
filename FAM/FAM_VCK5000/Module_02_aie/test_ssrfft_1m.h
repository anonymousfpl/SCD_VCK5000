//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include "fft_subsys.h"
#include "fam_sys.h"
#include "parameters.h"
#define PLFreq 500 //1000MHZ

using namespace adf ;



class dut: public graph {

public:

    static constexpr unsigned int stage1_kernel_num = 4;
    static constexpr unsigned int stage2_kernel_num = 32*4; 

    input_plio      stage1_in[2];
	output_plio    fam_out[128];
    stage1_graph_x2<22,6> stage1_graph;
    //stage2<1,7,1> stage2_graph;
    stage2_graph_x128<0> stage2_graph;
    


    dut(){
        for (int i = 0; i < 2; ++i) {
            std::string fname1_in = "data_stage1_x4x2/FAMDataIn_" + std::to_string(i) + ".txt"; 

            stage1_in[i] = input_plio::create("FAMDataIn_" + std::to_string(i), plio_64_bits, fname1_in, PLFreq);

            connect<>(stage1_in[i].out[0], stage1_graph.in[i]);
        }
               
        connect<>(stage1_graph.out0[0], stage2_graph.in0[0]);
        connect<>(stage1_graph.out0[1], stage2_graph.in0[1]);
        connect<>(stage1_graph.out1[0], stage2_graph.in1[0]);
        connect<>(stage1_graph.out1[1], stage2_graph.in1[1]);
      for (int i = 0; i < 128; ++i) {        
            std::string fname1_out = "data_stage1_x4x2/FAMOut" + std::to_string(i) + ".txt";

            fam_out[i] = output_plio::create("FAMOut_" + std::to_string(i), plio_64_bits, fname1_out, PLFreq);

            connect<>(stage2_graph.out[i], fam_out[i].in[0]);
        }

	};

}; // end of class

