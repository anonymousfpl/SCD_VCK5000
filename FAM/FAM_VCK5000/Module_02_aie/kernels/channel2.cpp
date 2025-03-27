//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include <adf.h>
#include <aie_api/aie.hpp>

#include "channel2.h"
#include "fft_stages.h"
#include "fam_funcs.h"
#include "parameters.h"
#include "fft_twiddle_lut_dit_cfloat.h"


// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

channel2::channel2(int xoff) : m_xoff(xoff)  
{
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);
}



// ------------------------------------------------------------
// Run
// ------------------------------------------------------------
void channel2::run(
    input_buffer_1d<cfloat,1024,0> & __restrict inputx0,
    output_stream_cfloat * __restrict outputy0,
    output_stream_cfloat * __restrict outputy1 )
{
    // 1024 cfloat => 512 v2cfloat
    // 强制对齐, 并转换成 v2cfloat指针
    alignas(aie::vector_decl_align) v2cfloat* restrict po1 = 
        (v2cfloat* restrict) inputx0.data();

    v2cfloat zero_val = aie::zeros<cfloat,2>();

    // 每段 128 次写(128 v2cfloat => 256 cfloat)

    // =============== Block #1: seg1 & seg9 ===============
    {
        for(int i = 0; i < 128; i++)
            chess_prepare_for_pipelining
            chess_loop_range(1,)
        {
            int idxA = 0 + i;    // seg1 => cfloat[0..255] => v2cfloat[0..127]
            int idxB = 256 + i;  // seg9 => cfloat[512..767] => v2cfloat[256..383]

            writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
            writeincr<aie_stream_resource_out::b>(outputy1, po1[idxB]);
        }
    }

    // =============== Block #2: seg2 & seg10 ===============
    {
        for(int i = 0; i < 128; i++)
            chess_prepare_for_pipelining
            chess_loop_range(1,)
        {
            int idxA = 32 + i;   // seg2 => cfloat[64..319] => v2cfloat[32..159]
            int idxB = 288 + i;  // seg10 => cfloat[576..831] => v2cfloat[288..415]

            writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
            writeincr<aie_stream_resource_out::b>(outputy1, po1[idxB]);
        }
    }

    // =============== Block #3: seg3 & seg11 ===============
    {
        for(int i = 0; i < 128; i++)
            chess_prepare_for_pipelining
            chess_loop_range(1,)
        {
            int idxA = 64 + i;   // seg3 => cfloat[128..383] => v2cfloat[64..191]
            int idxB = 320 + i;  // seg11 => cfloat[640..895] => v2cfloat[320..447]

            writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
            writeincr<aie_stream_resource_out::b>(outputy1, po1[idxB]);
        }
    }

    // =============== Block #4: seg4 & seg12 ===============
    {
        for(int i = 0; i < 128; i++)
            chess_prepare_for_pipelining
            chess_loop_range(1,)
        {
            int idxA = 96 + i;   // seg4 => cfloat[192..447] => v2cfloat[96..223]
            int idxB = 352 + i;  // seg12 => cfloat[704..959] => v2cfloat[352..479]

            writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
            writeincr<aie_stream_resource_out::b>(outputy1, po1[idxB]);
        }
    }

    // =============== Block #5: seg5 & seg13 ===============
    {
        for(int i = 0; i < 128; i++)
            chess_prepare_for_pipelining
            chess_loop_range(1,)
        {
            int idxA = 128 + i;  // seg5 => cfloat[256..511] => v2cfloat[128..255]
            int idxB = 384 + i;  // seg13 => cfloat[768..1023] => v2cfloat[384..511]

            writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
            writeincr<aie_stream_resource_out::b>(outputy1, po1[idxB]);
        }
    }

    // =============== Block #6: seg6 & seg14 ===============
    // 第一部分 (idxB 在 416 到 511 之间，直接写)
        {
            for(int i = 0; i < 96; i++)  // 416..511 => 96 次
                chess_prepare_for_pipelining
                chess_loop_range(1,)
            {
                int idxA = 160 + i;   // seg6 => cfloat[320..575] => v2cfloat[160..287]
                int idxB = 416 + i;   // seg14 => cfloat[832..1023] => v2cfloat[416..511]

                writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
                writeincr<aie_stream_resource_out::b>(outputy1, po1[idxB]);
            }
        }

        // 第二部分 (idxB 在 512 以上，需要减 512 进行环绕)
        {
            for(int i = 0; i < 32; i++)  // 512..543 => 32 次
                chess_prepare_for_pipelining
                chess_loop_range(1,)
            {
                int idxA = 256 + i;   // seg6 的剩余部分
                int idxB = i;         // seg14 => [1024..1087] => [0..63]

                writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
                writeincr<aie_stream_resource_out::b>(outputy1, zero_val);
            }
        }

    // =============== Block #7: seg7 & seg15 ===============
        {
        // 第一部分：i 从 0 到 63，idxB = 448 + i（不需要环绕）
        for (int i = 0; i < 64; i++)
            chess_prepare_for_pipelining
            chess_loop_range(1,)
        {
            int idxA = 192 + i;   // seg7: v2cfloat[192..255+]
            int idxB = 448 + i;   // seg15: v2cfloat[448..511]
            writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
            writeincr<aie_stream_resource_out::b>(outputy1, po1[idxB]);
        }

        // 第二部分：i 从 64 到 127，idxB = (448 + i) - 512
        for (int i = 64; i < 128; i++)
            chess_prepare_for_pipelining
            chess_loop_range(1,)
        {
            int idxA = 192 + i;   // seg7: v2cfloat[256..319]
            int idxB = (448 + i) - 512;  // seg15: for i=64 => 512-512=0; i=127 => 575-512=63
            writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
            writeincr<aie_stream_resource_out::b>(outputy1, zero_val);
        }
        }


    // =============== Block #8: seg8 & seg16 ===============
    {
        // 第一部分：i 从 0 到 31，idxB = 480 + i（不会超过 511）
        for (int i = 0; i < 32; i++)
            chess_prepare_for_pipelining
            chess_loop_range(1,)
        {
            int idxA = 224 + i;   // seg8: v2cfloat[224..255]
            int idxB = 480 + i;   // seg16: v2cfloat[480..511]
            writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
            writeincr<aie_stream_resource_out::b>(outputy1, po1[idxB]);
        }

        // 第二部分：i 从 32 到 127，idxB = (480 + i) - 512
        for (int i = 32; i < 128; i++)
            chess_prepare_for_pipelining
            chess_loop_range(1,)
        {
            int idxA = 224 + i;   // seg8: v2cfloat[256..351]
            int idxB = (480 + i) - 512;  // for i=32: (480+32)-512=0; for i=127: (480+127)-512=95
            writeincr<aie_stream_resource_out::a>(outputy0, po1[idxA]);
            writeincr<aie_stream_resource_out::b>(outputy1, zero_val);
        }
    }

}