//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include <adf.h>
#include <aie_api/aie.hpp>

#include "norm.h"
#include "fft_stages.h"
#include "fam_funcs.h"
#include "parameters.h"
#include "fft_twiddle_lut_dit_cfloat.h"


// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

norm::norm(int xoff) : m_xoff(xoff)  
{
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);
}

// ------------------------------------------------------------
// Run
// ------------------------------------------------------------

void norm::run( input_buffer_1d<float,1024,0>& __restrict inputxR, 
                input_buffer_1d<float,1024,0>& __restrict inputxI, 
                output_buffer_1d<cfloat,1024>& __restrict outputy0,
                output_buffer_1d<cfloat,1024>& __restrict outputy1 )
{
    
    float * restrict xbufR =  (float * restrict) inputxR.data();
    float * restrict xbufI =  (float * restrict) inputxI.data();

    cfloat * restrict ybuff0 =  (cfloat * restrict) outputy0.data();
    cfloat * restrict ybuff1 =  (cfloat * restrict) outputy1.data();


    float maxR, maxI;
    abs_max((float * restrict)xbufR, (float * restrict)xbufI, maxR, maxI);

    float invmaxR, invmaxI;
    invmaxR = 1.0f / maxR;
    invmaxI = 1.0f / maxI;
    norm_buf((float * restrict)xbufR, (float * restrict)xbufI, (cfloat * restrict)ybuff0, (cfloat * restrict)ybuff1, invmaxR, invmaxI);

}

/*

void norm::run( input_buffer_1d<float,1024,0>& __restrict inputxR, 
                input_buffer_1d<float,1024,0>& __restrict inputxI, 
                output_buffer_1d<cfloat,1024>& __restrict outputy0,
                output_buffer_1d<cfloat,1024>& __restrict outputy1 )
{
    // 本例采用 SoA 模式：分别存储实部和虚部
    alignas(aie::vector_decl_align) cfloat  tmp1[256]; 
    alignas(aie::vector_decl_align) cfloat  tmp2[256];  
    
    cfloat * restrict xbufR =  (cfloat * restrict) inputxR.data();
    cfloat * restrict xbufI =  (cfloat * restrict) inputxI.data();
    cfloat * restrict ybuff0 =  (cfloat * restrict) outputy0.data();
    cfloat * restrict ybuff1 =  (cfloat * restrict) outputy1.data();


    float maxR,maxI = abs_max(xbufR, xbufI);
    norm_buf(xbufR, xbufI, ybuff0, ybuff1,maxR,maxI);
    


    float bufR[1024];
    float bufI[1024];

    float maxR = 0.0f;
    float maxI = 0.0f;

    // 使用指针操作简化写入过程
    float* pBufR = bufR;
    float* pBufI = bufI;

    // 每次批量处理 32 个 float
    
    constexpr int NUM_BLOCKS  = 1024 / 32;  // 32 个块

    cfloat * restrict out0 = (cfloat * restrict) outputy0.data();
    cfloat * restrict out1 = (cfloat * restrict) outputy1.data();

    int offset = 0;
    // 第一遍：批量读取输入，并利用向量操作计算最大绝对值
    for(int blk = 0; blk < NUM_BLOCKS; blk++) 
        chess_prepare_for_pipelining chess_loop_range(NUM_BLOCKS,NUM_BLOCKS)
    {
        // 读取 32 个实部和 32 个虚部
        aie::vector<float, 32> vr = readincr_v<32>(inputxR);
        aie::vector<float, 32> vi = readincr_v<32>(inputxI);

        // 计算绝对值向量：aie::abs() 对每个元素取绝对值
        aie::vector<float, 32> abs_vr = aie::abs(vr);
        aie::vector<float, 32> abs_vi = aie::abs(vi);

        // reduce_max 求出这 32 个元素中的最大值
        float localMaxR = aie::reduce_max(abs_vr);
        float localMaxI = aie::reduce_max(abs_vi);

        // 更新全局最大值
        if(localMaxR > maxR) maxR = localMaxR;
        if(localMaxI > maxI) maxI = localMaxI;

        // 将读取的 32 个实部/虚部保存到临时数组中
        for(int k = 0; k < 32; k++) 
            chess_prepare_for_pipelining chess_loop_range(1,) 
            {
                *pBufR++ = vr[k];
                *pBufI++ = vi[k];
            }
        
        
    }
    

    // 重置 buf 指针以便后续归一化读取
    pBufR = bufR;
    pBufI = bufI;
    float invMaxR = 1.0f / maxR;
    float invMaxI = 1.0f / maxI;


    //============ 归一化部分 ============//
    // 我们将分块处理 1024 个数据，每次处理 32 个
    aie::vector<float, 32> vR, vI;
    for (int blk = 0; blk < NUM_BLOCKS; blk++) 
        //chess_prepare_for_pipelining chess_loop_range(1,) 
        {            
            // 将 bufR 和 bufI 中当前块数据加载到向量中
            for (int k = 0; k < 32; k++) {
                vR[k] = *pBufR++;
                vI[k] = *pBufI++;
            }
            // 使用广播向量进行归一化            
            //aie::vector<float, 32> vR_norm = vR * aie::vector<float, 32>(aie::broadcast<float, 32>(invMaxR));
            //aie::vector<float, 32> vI_norm = vI * aie::vector<float, 32>(aie::broadcast<float, 32>(invMaxI));

            aie::vector<float, 32> vR_norm = aie::mul(vR, invMaxR);
            aie::vector<float, 32> vI_norm = aie::mul(vI, invMaxI);


            // 将归一化后的向量写回 norm_buf
            for (int k = 0; k < 32; k++) 
            chess_prepare_for_pipelining chess_loop_range(1,)
            {
                int idx = blk * 32 + k;
                out0[idx].real = vR_norm[k];
                out0[idx].imag = vI_norm[k];
                out1[idx].real = vR_norm[k];
                out1[idx].imag = vI_norm[k];
            }
        }
    

}

*/