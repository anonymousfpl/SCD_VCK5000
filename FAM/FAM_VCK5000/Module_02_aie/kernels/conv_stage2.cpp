//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include <adf.h>
#include <aie_api/aie.hpp>

#include "conv_stage2.h"
#include "fft_stages.h"
#include "fam_funcs.h"
#include "parameters.h"
#include "fft_twiddle_lut_dit_cfloat.h"

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

conv_stage2::conv_stage2(int xoff) : m_xoff(xoff)  
{
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);
}

// ------------------------------------------------------------
// Run
// ------------------------------------------------------------

void conv_stage2::run( input_stream_cfloat * __restrict  inputx0,
                       input_stream_cfloat * __restrict  inputx1,
                       output_stream_cfloat * __restrict  outputy )
{ 
  
    
    for (int i = 0; i < 256; i++)  
        chess_prepare_for_pipelining 
        chess_loop_range(256,256)
    {   
        writeincr(outputy, readincr_v<2>(inputx0));
        writeincr(outputy, readincr_v<2>(inputx0));
        writeincr(outputy, readincr_v<2>(inputx0));
        writeincr(outputy, readincr_v<2>(inputx0));
        writeincr(outputy, readincr_v<2>(inputx0));
        writeincr(outputy, readincr_v<2>(inputx0));
        writeincr(outputy, readincr_v<2>(inputx0));
        writeincr(outputy, readincr_v<2>(inputx0));

        writeincr(outputy, readincr_v<2>(inputx1));
        writeincr(outputy, readincr_v<2>(inputx1));
        writeincr(outputy, readincr_v<2>(inputx1));
        writeincr(outputy, readincr_v<2>(inputx1));
        writeincr(outputy, readincr_v<2>(inputx1));
        writeincr(outputy, readincr_v<2>(inputx1));
        writeincr(outputy, readincr_v<2>(inputx1));
        writeincr(outputy, readincr_v<2>(inputx1));

        
    }
}        



