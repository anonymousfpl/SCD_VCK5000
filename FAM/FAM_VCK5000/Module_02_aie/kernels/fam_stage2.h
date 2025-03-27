//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include <adf.h>
#include <aie_api/aie.hpp>

using namespace adf;

class fam_stage2 {
private:
    int m_xoff;
    float m_global_max;  
public:
  // Constructor:
  fam_stage2(int xoff);

  void stream_out(output_stream_float* __restrict outputbuf, float * restrict result);
  void compute_abs_square(cfloat* __restrict inbuf,  // 输入缓冲区（32个 cfloat）
                                 float* __restrict outbuf); // 输出缓冲区（16个 cfloat）

  // Run:
  void run(input_stream<cfloat>* __restrict inputx0,
           input_stream<cfloat>* __restrict inputx1,
                     output_stream<float> * __restrict outputy);

  static void registerKernelClass( void )
  {
    REGISTER_FUNCTION( fam_stage2::run );
  }
};

