//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include <adf.h>
#include <aie_api/aie.hpp>

#include "fam_stage2.h"
#include "fft_stages.h"
#include "fam_funcs.h"
#include "parameters.h"
// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

fam_stage2::fam_stage2(int xoff) : m_xoff(xoff), m_global_max(0.0f)
{
  aie::set_rounding(aie::rounding_mode::symmetric_inf);
  aie::set_saturation(aie::saturation_mode::saturate);
}

inline __attribute__((always_inline))
void fam_stage2::compute_abs_square(cfloat* __restrict inbuf,  // 输入缓冲区（32个 cfloat）
                                    float* __restrict outbuf)  // 输出缓冲区（16个 float）
{
    // 使用 reinterpret_cast 将 inbuf 视为 `aie::vector<cfloat, 8>*`
    aie::vector<cfloat, 8> *restrict vin1 = reinterpret_cast<aie::vector<cfloat, 8>*>(&inbuf[16]); 
    aie::vector<cfloat, 8> *restrict vin2 = reinterpret_cast<aie::vector<cfloat, 8>*>(&inbuf[8]);

    // 使用 reinterpret_cast 直接访问 outbuf
    aie::vector<float, 8> *restrict vout1 = reinterpret_cast<aie::vector<float, 8>*>(&outbuf[0]);  
    aie::vector<float, 8> *restrict vout2 = reinterpret_cast<aie::vector<float, 8>*>(&outbuf[8]);

    // 计算模平方并存储
    *vout1 = aie::abs_square(*vin1);
    *vout2 = aie::abs_square(*vin2);

    // 新增逻辑：求本次生成的两个向量中的最大值
    aie::vector<float, 8> max_vector = aie::max(*vout1, *vout2);
    float current_max = aie::reduce_max(max_vector);

    // 若当前的最大值大于全局最大值（假设 m_global_max 为类成员变量），则更新全局最大值
    if (current_max > m_global_max) {
        m_global_max = current_max;
    }
}



inline __attribute__((always_inline))
void fam_stage2::stream_out(output_stream_float* __restrict outputbuf, float * restrict result)
{
    // 从 result[0] 中依次提取 8 个 4cfloat 元素（每个元素代表一个 32 位复数向量段），写入输出流
    v16float * restrict pi1 = (v16float * restrict) result;
    writeincr(outputbuf, ext_v(pi1[0], 0));
    writeincr(outputbuf, ext_v(pi1[0], 1));
    writeincr(outputbuf, ext_v(pi1[0], 2));
    writeincr(outputbuf, ext_v(pi1[0], 3));   
}


void fam_stage2::run(input_stream_cfloat* __restrict inputx0,
                      input_stream_cfloat* __restrict inputx1,
                     output_stream_float * __restrict outputy)
{   
    int index1 = m_xoff ;
    int index;
    alignas(aie::vector_decl_align) v16cfloat xbuf0[2];  
    alignas(aie::vector_decl_align) v16cfloat xbuf1[2];  
    alignas(aie::vector_decl_align) v16cfloat conj[2];   
    alignas(aie::vector_decl_align) v16cfloat ybuff[2];  
    alignas(aie::vector_decl_align) v16cfloat tmp2[2];
    alignas(aie::vector_decl_align) v16cfloat tmp1[2];

    alignas(aie::vector_decl_align) v16float result[1];

    const int n = 256; 
    if (index1 > 128)
    {
       index = index1 - 128;
    }
    else
    {
        index = index1;
    }
    
    
    if (index > 1) {
    for (int i = 0; i < index - 1; i++)
        chess_prepare_for_pipelining chess_loop_range(1,) 
    {   
        readincr_v<16>(inputx0); 
        readincr_v<16>(inputx1);                            
    }
    }
    
    if (index > 0) {
        xbuf0[0] = readincr_v<16>(inputx0);
        xbuf0[1] = readincr_v<16>(inputx1);
        xbuf1[0] = readincr_v<16>(inputx0);
        xbuf1[1] = readincr_v<16>(inputx1);

    }

    
    if (index < 127) {
        for (int i = 0; i < 128 - index-1; i++)
            chess_prepare_for_pipelining chess_loop_range(1,) 
        {   
            readincr_v<16>(inputx0);  
            readincr_v<16>(inputx1);                           
        }
    }

    


    for (int i = 0; i < n; i++)  
    //chess_prepare_for_pipelining chess_loop_range(1,)  
    {   
        conj[0] = readincr_v<16>(inputx0);
        conj[1] = readincr_v<16>(inputx1);        
        stage2_cm((cfloat * restrict)xbuf0, (cfloat * restrict)conj, (cfloat * restrict)tmp1);
        FFT_32pt((cfloat * restrict)tmp1, (cfloat * restrict)tmp2, (cfloat * restrict)ybuff);  
        //Amplitude((cfloat * restrict)ybuff, (cfloat * restrict)result);  
        compute_abs_square((cfloat * restrict)ybuff, (float * restrict)result);   
        

        stream_out(outputy, (float * restrict)result);
    }



    for (int i = 0; i < n; i++)  
    //chess_prepare_for_pipelining chess_loop_range(1,)  
    {   
        conj[0] = readincr_v<16>(inputx0);
        conj[1] = readincr_v<16>(inputx1);        
        stage2_cm((cfloat * restrict)xbuf1, (cfloat * restrict)conj, (cfloat * restrict)tmp1);
        FFT_32pt((cfloat * restrict)tmp1, (cfloat * restrict)tmp2, (cfloat * restrict)ybuff);       
       
        //FFTshift + cutting out the high frequency and low frequency       
        //Amplitude((cfloat * restrict)ybuff, (cfloat * restrict)result);     
        compute_abs_square((cfloat * restrict)ybuff, (float * restrict)result);
        //FFTshift + cutting out the high frequency and low frequency       
        
        /*
        writeincr(outputy, ext_v(ybuff[1], 0));
        writeincr(outputy, ext_v(ybuff[1], 1));
        writeincr(outputy, ext_v(ybuff[1], 2));
        writeincr(outputy, ext_v(ybuff[1], 3));
       
        writeincr(outputy, ext_v(ybuff[0], 4));
        writeincr(outputy, ext_v(ybuff[0], 5));
        writeincr(outputy, ext_v(ybuff[0], 6));
        writeincr(outputy, ext_v(ybuff[0], 7)); 
        
        writeincr(outputy, ext_v(result[0], 0));
        writeincr(outputy, ext_v(result[0], 1));
        writeincr(outputy, ext_v(result[0], 2));
        writeincr(outputy, ext_v(result[0], 3));
       
        writeincr(outputy, ext_v(result[0], 4));
        writeincr(outputy, ext_v(result[0], 5));
        writeincr(outputy, ext_v(result[0], 6));
        writeincr(outputy, ext_v(result[0], 7));    
        */
        stream_out(outputy, (float * restrict)result);
    }

    aie::vector<float, 4> globalVec = aie::broadcast<float, 4>(m_global_max);
    // 只输出其中一个元素（或根据需求输出整个向量）
    writeincr(outputy, globalVec, true);
  
}





