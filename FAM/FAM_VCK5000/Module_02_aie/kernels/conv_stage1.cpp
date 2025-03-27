//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include <adf.h>
#include <aie_api/aie.hpp>

#include "conv_stage1.h"
#include "fft_stages.h"
#include "fam_funcs.h"
#include "parameters.h"
#include "fft_twiddle_lut_dit_cfloat.h"


// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

conv_stage1::conv_stage1(int xoff) : m_xoff(xoff)  
{
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);
}

// ------------------------------------------------------------
// Run
// ------------------------------------------------------------

void conv_stage1::run( input_buffer_1d<cfloat,2048,0>& __restrict  inputx0,
                       input_buffer_1d<cfloat,2048,0>& __restrict  inputx1,
                       output_stream_cfloat * __restrict  outputy0 ,
                       output_stream_cfloat * __restrict  outputy1 )
{ 
  alignas(aie::vector_decl_align) v2cfloat * restrict po1 = (v2cfloat * restrict) inputx0.data();   
  alignas(aie::vector_decl_align) v2cfloat * restrict po2 = (v2cfloat * restrict) inputx1.data();   
  
  for (int i = 0; i < 128; i++)  
        chess_prepare_for_pipelining 
        //chess_flatten_loop 
        chess_loop_range(128,128)
    {  
        //161334400 ps
        //outputy0
        writeincr<aie_stream_resource_out::a>(outputy0, po1[i * 4]);          
        writeincr<aie_stream_resource_out::a>(outputy0, po1[i * 4 + 1]);
        writeincr<aie_stream_resource_out::a>(outputy0, po1[i * 4 + 2]);
        writeincr<aie_stream_resource_out::a>(outputy0, po1[i * 4 + 3]);     
        //outputy1   
        writeincr<aie_stream_resource_out::b>(outputy1, po1[(i+128) * 4]);
        writeincr<aie_stream_resource_out::b>(outputy1, po1[(i+128) * 4 + 1]);         
        writeincr<aie_stream_resource_out::b>(outputy1, po1[(i+128) * 4 + 2]);             
        writeincr<aie_stream_resource_out::b>(outputy1, po1[(i+128) * 4 + 3]);
        //outputy0
        writeincr<aie_stream_resource_out::a>(outputy0, po2[i * 4]);        
        writeincr<aie_stream_resource_out::a>(outputy0, po2[i * 4 + 1]);
        writeincr<aie_stream_resource_out::a>(outputy0, po2[i * 4 + 2]);
        writeincr<aie_stream_resource_out::a>(outputy0, po2[i * 4 + 3]);  
        //outputy1
        writeincr<aie_stream_resource_out::b>(outputy1, po2[(i+128) * 4]);
        writeincr<aie_stream_resource_out::b>(outputy1, po2[(i+128) * 4 + 1]);        
        writeincr<aie_stream_resource_out::b>(outputy1, po2[(i+128) * 4 + 2]);        
        writeincr<aie_stream_resource_out::b>(outputy1, po2[(i+128) * 4 + 3]);

        
    }
  
  for (int repeat = 0; repeat < 2; repeat++)
      chess_prepare_for_pipelining 
      chess_loop_range(2,2)
  {
      
      for (int i = 0; i < 256; i++)  
          chess_prepare_for_pipelining 
          //chess_flatten_loop 
          chess_loop_range(256,256)
      {  
          //161334400 ps
          writeincr<aie_stream_resource_out::a>(outputy0, po1[i * 4]);          
          writeincr<aie_stream_resource_out::a>(outputy0, po1[i * 4 + 1]);
          writeincr<aie_stream_resource_out::a>(outputy0, po1[i * 4 + 2]);
          writeincr<aie_stream_resource_out::a>(outputy0, po1[i * 4 + 3]);     

          writeincr<aie_stream_resource_out::b>(outputy1, po1[i * 4]);
          writeincr<aie_stream_resource_out::b>(outputy1, po1[i * 4 + 1]);         
          writeincr<aie_stream_resource_out::b>(outputy1, po1[i * 4 + 2]);             
          writeincr<aie_stream_resource_out::b>(outputy1, po1[i * 4 + 3]);

          writeincr<aie_stream_resource_out::a>(outputy0, po2[i * 4]);        
          writeincr<aie_stream_resource_out::a>(outputy0, po2[i * 4 + 1]);
          writeincr<aie_stream_resource_out::a>(outputy0, po2[i * 4 + 2]);
          writeincr<aie_stream_resource_out::a>(outputy0, po2[i * 4 + 3]);  
          

          writeincr<aie_stream_resource_out::b>(outputy1, po2[i * 4]);
          writeincr<aie_stream_resource_out::b>(outputy1, po2[i * 4 + 1]);        
          writeincr<aie_stream_resource_out::b>(outputy1, po2[i * 4 + 2]);        
          writeincr<aie_stream_resource_out::b>(outputy1, po2[i * 4 + 3]);

          
      }
  }        
}


