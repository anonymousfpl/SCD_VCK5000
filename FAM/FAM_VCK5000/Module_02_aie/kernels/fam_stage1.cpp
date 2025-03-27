//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include <adf.h>
#include <aie_api/aie.hpp>

#include "fam_stage1.h"
#include "fft_stages.h"
#include "fam_funcs.h"
#include "parameters.h"
#include "fft_twiddle_lut_dit_cfloat.h"

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

fam_stage1::fam_stage1(int xoff) : m_xoff(xoff) 
{
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);
}

// ------------------------------------------------------------
// Run
// ------------------------------------------------------------

void fam_stage1::run(
    input_stream_cfloat * __restrict inputx0,  // 现在为输入流
    output_buffer_1d<cfloat,2048>& __restrict outputy )
{ 
    int index = m_xoff & 3;
    bool tlast = false;
  
    // 临时工作缓冲区，保持对齐
    alignas(aie::vector_decl_align) cfloat  tmp1[256]; 
    alignas(aie::vector_decl_align) cfloat  tmp2[256];  
    // 用于每次读取 256 个 cfloat 的缓冲区
    alignas(aie::vector_decl_align) v16cfloat xbuf0[16];  

    // 输出缓冲区指针
    cfloat * restrict ybuff = (cfloat * restrict) outputy.data();


  
  for (int i = 0; i < 1; i++)
      chess_prepare_for_pipelining chess_loop_range(1,)
    {
     
      for (int block = 0; block < 4; block++) 
          chess_prepare_for_pipelining chess_loop_range(4,4)
      {
          // 读取当前 block 的 256 个 cfloat 从输入流
          for (int i = 0; i < 16; i++)
            chess_prepare_for_pipelining chess_loop_range(1,)
            {
                xbuf0[i] = readincr_v<16>(inputx0);
            }

          int transpose_idx = block;  // 0-3
          
          window_fam((cfloat * restrict)xbuf0, (cfloat * restrict)tmp1);
          FFT_256pt((cfloat * restrict)tmp1, 
                  (cfloat * restrict)tmp2, 
                  (cfloat * restrict)xbuf0);
          stage1_dc((cfloat * restrict)xbuf0, block, (cfloat * restrict)tmp2);
          transpose_256((cfloat * restrict)tmp2, ybuff, transpose_idx);
      }
  }

 
  for (int i = 0; i < 1; i++)
      chess_prepare_for_pipelining chess_loop_range(1,)
  {
     
      for (int block = 0; block < 4; block++) 
          chess_prepare_for_pipelining chess_loop_range(4,4)
      {
          // 读取当前 block 的 256 个 cfloat 从输入流
          for (int i = 0; i < 16; i++)
            chess_prepare_for_pipelining chess_loop_range(1,)
            {
                xbuf0[i] = readincr_v<16>(inputx0);
            }
          int transpose_idx = block + 4;  // 4-7
          
          window_fam((cfloat * restrict)xbuf0, (cfloat * restrict)tmp1);
          FFT_256pt((cfloat * restrict)tmp1, 
                  (cfloat * restrict)tmp2, 
                  (cfloat * restrict)xbuf0);
          stage1_dc((cfloat * restrict)xbuf0, block, (cfloat * restrict)tmp2);
          transpose_256((cfloat * restrict)tmp2, ybuff, transpose_idx);
      }
  }


}
