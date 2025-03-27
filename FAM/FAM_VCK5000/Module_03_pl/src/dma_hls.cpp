/***************************************************************************/
/*  Copyright (C) 2023 Advanced Micro Devices, Inc
 *  SPDX-License-Identifier: MIT
 *
 *  Example: Using ap_uint<512> for AXI Master interface, transferring 8 cfloat (64bit) at a time
 *  Reading 8 input channels × 1024 cfloat from memin
 *  Writing to 128 output channels × 8192 cfloat
 */
/***************************************************************************/

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

// Each cfloat = 64bit
typedef ap_axiu<64, 0, 0, 0> axis_data_64;
typedef ap_axiu<32, 0, 0, 0> axis_data_32;
// Number of input channels & output channels
#define N_IN_CH   8
#define N_OUT_CH  128

// Each input channel contains 1024 cfloat
#define NSAMP_IN  1024
// Each output channel contains 8192 cfloat
#define NSAMP_OUT 8192

//------------------------------
// Top-level HLS kernel
//------------------------------
extern "C"
void dma_hls(
    // Using ap_uint<512> as port type
    ap_uint<32> *memin0,
    ap_uint<32> *memin1,   
    ap_uint<512> *memout,

    
    hls::stream<axis_data_32> &FAMDataIn_0,
    hls::stream<axis_data_32> &FAMDataIn_1,
    

    
    hls::stream<axis_data_64> &FAMOut_0,
    hls::stream<axis_data_64> &FAMOut_1,
    hls::stream<axis_data_64> &FAMOut_2,
    hls::stream<axis_data_64> &FAMOut_3,
    hls::stream<axis_data_64> &FAMOut_4,
    hls::stream<axis_data_64> &FAMOut_5,
    hls::stream<axis_data_64> &FAMOut_6,
    hls::stream<axis_data_64> &FAMOut_7,
    hls::stream<axis_data_64> &FAMOut_8,
    hls::stream<axis_data_64> &FAMOut_9,
    hls::stream<axis_data_64> &FAMOut_10,
    hls::stream<axis_data_64> &FAMOut_11,
    hls::stream<axis_data_64> &FAMOut_12,
    hls::stream<axis_data_64> &FAMOut_13,
    hls::stream<axis_data_64> &FAMOut_14,
    hls::stream<axis_data_64> &FAMOut_15,
    hls::stream<axis_data_64> &FAMOut_16,
    hls::stream<axis_data_64> &FAMOut_17,
    hls::stream<axis_data_64> &FAMOut_18,
    hls::stream<axis_data_64> &FAMOut_19,
    hls::stream<axis_data_64> &FAMOut_20,
    hls::stream<axis_data_64> &FAMOut_21,
    hls::stream<axis_data_64> &FAMOut_22,
    hls::stream<axis_data_64> &FAMOut_23,
    hls::stream<axis_data_64> &FAMOut_24,
    hls::stream<axis_data_64> &FAMOut_25,
    hls::stream<axis_data_64> &FAMOut_26,
    hls::stream<axis_data_64> &FAMOut_27,
    hls::stream<axis_data_64> &FAMOut_28,
    hls::stream<axis_data_64> &FAMOut_29,
    hls::stream<axis_data_64> &FAMOut_30,
    hls::stream<axis_data_64> &FAMOut_31,
    hls::stream<axis_data_64> &FAMOut_32,
    hls::stream<axis_data_64> &FAMOut_33,
    hls::stream<axis_data_64> &FAMOut_34,
    hls::stream<axis_data_64> &FAMOut_35,
    hls::stream<axis_data_64> &FAMOut_36,
    hls::stream<axis_data_64> &FAMOut_37,
    hls::stream<axis_data_64> &FAMOut_38,
    hls::stream<axis_data_64> &FAMOut_39,
    hls::stream<axis_data_64> &FAMOut_40,
    hls::stream<axis_data_64> &FAMOut_41,
    hls::stream<axis_data_64> &FAMOut_42,
    hls::stream<axis_data_64> &FAMOut_43,
    hls::stream<axis_data_64> &FAMOut_44,
    hls::stream<axis_data_64> &FAMOut_45,
    hls::stream<axis_data_64> &FAMOut_46,
    hls::stream<axis_data_64> &FAMOut_47,
    hls::stream<axis_data_64> &FAMOut_48,
    hls::stream<axis_data_64> &FAMOut_49,
    hls::stream<axis_data_64> &FAMOut_50,
    hls::stream<axis_data_64> &FAMOut_51,
    hls::stream<axis_data_64> &FAMOut_52,
    hls::stream<axis_data_64> &FAMOut_53,
    hls::stream<axis_data_64> &FAMOut_54,
    hls::stream<axis_data_64> &FAMOut_55,
    hls::stream<axis_data_64> &FAMOut_56,
    hls::stream<axis_data_64> &FAMOut_57,
    hls::stream<axis_data_64> &FAMOut_58,
    hls::stream<axis_data_64> &FAMOut_59,
    hls::stream<axis_data_64> &FAMOut_60,
    hls::stream<axis_data_64> &FAMOut_61,
    hls::stream<axis_data_64> &FAMOut_62,
    hls::stream<axis_data_64> &FAMOut_63,
    hls::stream<axis_data_64> &FAMOut_64,
    hls::stream<axis_data_64> &FAMOut_65,
    hls::stream<axis_data_64> &FAMOut_66,
    hls::stream<axis_data_64> &FAMOut_67,
    hls::stream<axis_data_64> &FAMOut_68,
    hls::stream<axis_data_64> &FAMOut_69,
    hls::stream<axis_data_64> &FAMOut_70,
    hls::stream<axis_data_64> &FAMOut_71,
    hls::stream<axis_data_64> &FAMOut_72,
    hls::stream<axis_data_64> &FAMOut_73,
    hls::stream<axis_data_64> &FAMOut_74,
    hls::stream<axis_data_64> &FAMOut_75,
    hls::stream<axis_data_64> &FAMOut_76,
    hls::stream<axis_data_64> &FAMOut_77,
    hls::stream<axis_data_64> &FAMOut_78,
    hls::stream<axis_data_64> &FAMOut_79,
    hls::stream<axis_data_64> &FAMOut_80,
    hls::stream<axis_data_64> &FAMOut_81,
    hls::stream<axis_data_64> &FAMOut_82,
    hls::stream<axis_data_64> &FAMOut_83,
    hls::stream<axis_data_64> &FAMOut_84,
    hls::stream<axis_data_64> &FAMOut_85,
    hls::stream<axis_data_64> &FAMOut_86,
    hls::stream<axis_data_64> &FAMOut_87,
    hls::stream<axis_data_64> &FAMOut_88,
    hls::stream<axis_data_64> &FAMOut_89,
    hls::stream<axis_data_64> &FAMOut_90,
    hls::stream<axis_data_64> &FAMOut_91,
    hls::stream<axis_data_64> &FAMOut_92,
    hls::stream<axis_data_64> &FAMOut_93,
    hls::stream<axis_data_64> &FAMOut_94,
    hls::stream<axis_data_64> &FAMOut_95,
    hls::stream<axis_data_64> &FAMOut_96,
    hls::stream<axis_data_64> &FAMOut_97,
    hls::stream<axis_data_64> &FAMOut_98,
    hls::stream<axis_data_64> &FAMOut_99,
    hls::stream<axis_data_64> &FAMOut_100,
    hls::stream<axis_data_64> &FAMOut_101,
    hls::stream<axis_data_64> &FAMOut_102,
    hls::stream<axis_data_64> &FAMOut_103,
    hls::stream<axis_data_64> &FAMOut_104,
    hls::stream<axis_data_64> &FAMOut_105,
    hls::stream<axis_data_64> &FAMOut_106,
    hls::stream<axis_data_64> &FAMOut_107,
    hls::stream<axis_data_64> &FAMOut_108,
    hls::stream<axis_data_64> &FAMOut_109,
    hls::stream<axis_data_64> &FAMOut_110,
    hls::stream<axis_data_64> &FAMOut_111,
    hls::stream<axis_data_64> &FAMOut_112,
    hls::stream<axis_data_64> &FAMOut_113,
    hls::stream<axis_data_64> &FAMOut_114,
    hls::stream<axis_data_64> &FAMOut_115,
    hls::stream<axis_data_64> &FAMOut_116,
    hls::stream<axis_data_64> &FAMOut_117,
    hls::stream<axis_data_64> &FAMOut_118,
    hls::stream<axis_data_64> &FAMOut_119,
    hls::stream<axis_data_64> &FAMOut_120,
    hls::stream<axis_data_64> &FAMOut_121,
    hls::stream<axis_data_64> &FAMOut_122,
    hls::stream<axis_data_64> &FAMOut_123,
    hls::stream<axis_data_64> &FAMOut_124,
    hls::stream<axis_data_64> &FAMOut_125,
    hls::stream<axis_data_64> &FAMOut_126,
    hls::stream<axis_data_64> &FAMOut_127,

    unsigned int iter  
)
{
    /************************************************************************/
    /*  HLS Interface Configuration: 512-bit width                        */
    /************************************************************************/


#pragma HLS INTERFACE s_axilite bundle=control port=memin0
#pragma HLS INTERFACE s_axilite bundle=control port=memin1
#pragma HLS INTERFACE s_axilite bundle=control port=memout
#pragma HLS INTERFACE s_axilite bundle=control port=iter   
#pragma HLS INTERFACE s_axilite bundle=control port=return 
#pragma HLS interface m_axi offset=slave bundle=gmem0 port=memin0 max_read_burst_length=16 num_read_outstanding=64
#pragma HLS interface m_axi offset=slave bundle=gmem1 port=memin1 max_read_burst_length=16 num_read_outstanding=64
#pragma HLS interface m_axi offset=slave bundle=gmem8 port=memout max_write_burst_length=16 num_write_outstanding=64
   
#pragma HLS INTERFACE axis port=FAMDataIn_0
#pragma HLS INTERFACE axis port=FAMDataIn_1



#pragma HLS INTERFACE axis port=FAMOut_0
#pragma HLS INTERFACE axis port=FAMOut_1
#pragma HLS INTERFACE axis port=FAMOut_2
#pragma HLS INTERFACE axis port=FAMOut_3
#pragma HLS INTERFACE axis port=FAMOut_4
#pragma HLS INTERFACE axis port=FAMOut_5
#pragma HLS INTERFACE axis port=FAMOut_6
#pragma HLS INTERFACE axis port=FAMOut_7
#pragma HLS INTERFACE axis port=FAMOut_8
#pragma HLS INTERFACE axis port=FAMOut_9
#pragma HLS INTERFACE axis port=FAMOut_10
#pragma HLS INTERFACE axis port=FAMOut_11
#pragma HLS INTERFACE axis port=FAMOut_12
#pragma HLS INTERFACE axis port=FAMOut_13
#pragma HLS INTERFACE axis port=FAMOut_14
#pragma HLS INTERFACE axis port=FAMOut_15
#pragma HLS INTERFACE axis port=FAMOut_16
#pragma HLS INTERFACE axis port=FAMOut_17
#pragma HLS INTERFACE axis port=FAMOut_18
#pragma HLS INTERFACE axis port=FAMOut_19
#pragma HLS INTERFACE axis port=FAMOut_20
#pragma HLS INTERFACE axis port=FAMOut_21
#pragma HLS INTERFACE axis port=FAMOut_22
#pragma HLS INTERFACE axis port=FAMOut_23
#pragma HLS INTERFACE axis port=FAMOut_24
#pragma HLS INTERFACE axis port=FAMOut_25
#pragma HLS INTERFACE axis port=FAMOut_26
#pragma HLS INTERFACE axis port=FAMOut_27
#pragma HLS INTERFACE axis port=FAMOut_28
#pragma HLS INTERFACE axis port=FAMOut_29
#pragma HLS INTERFACE axis port=FAMOut_30
#pragma HLS INTERFACE axis port=FAMOut_31
#pragma HLS INTERFACE axis port=FAMOut_32
#pragma HLS INTERFACE axis port=FAMOut_33
#pragma HLS INTERFACE axis port=FAMOut_34
#pragma HLS INTERFACE axis port=FAMOut_35
#pragma HLS INTERFACE axis port=FAMOut_36
#pragma HLS INTERFACE axis port=FAMOut_37
#pragma HLS INTERFACE axis port=FAMOut_38
#pragma HLS INTERFACE axis port=FAMOut_39
#pragma HLS INTERFACE axis port=FAMOut_40
#pragma HLS INTERFACE axis port=FAMOut_41
#pragma HLS INTERFACE axis port=FAMOut_42
#pragma HLS INTERFACE axis port=FAMOut_43
#pragma HLS INTERFACE axis port=FAMOut_44
#pragma HLS INTERFACE axis port=FAMOut_45
#pragma HLS INTERFACE axis port=FAMOut_46
#pragma HLS INTERFACE axis port=FAMOut_47
#pragma HLS INTERFACE axis port=FAMOut_48
#pragma HLS INTERFACE axis port=FAMOut_49
#pragma HLS INTERFACE axis port=FAMOut_50
#pragma HLS INTERFACE axis port=FAMOut_51
#pragma HLS INTERFACE axis port=FAMOut_52
#pragma HLS INTERFACE axis port=FAMOut_53
#pragma HLS INTERFACE axis port=FAMOut_54
#pragma HLS INTERFACE axis port=FAMOut_55
#pragma HLS INTERFACE axis port=FAMOut_56
#pragma HLS INTERFACE axis port=FAMOut_57
#pragma HLS INTERFACE axis port=FAMOut_58
#pragma HLS INTERFACE axis port=FAMOut_59
#pragma HLS INTERFACE axis port=FAMOut_60
#pragma HLS INTERFACE axis port=FAMOut_61
#pragma HLS INTERFACE axis port=FAMOut_62
#pragma HLS INTERFACE axis port=FAMOut_63
#pragma HLS INTERFACE axis port=FAMOut_64
#pragma HLS INTERFACE axis port=FAMOut_65
#pragma HLS INTERFACE axis port=FAMOut_66
#pragma HLS INTERFACE axis port=FAMOut_67
#pragma HLS INTERFACE axis port=FAMOut_68
#pragma HLS INTERFACE axis port=FAMOut_69
#pragma HLS INTERFACE axis port=FAMOut_70
#pragma HLS INTERFACE axis port=FAMOut_71
#pragma HLS INTERFACE axis port=FAMOut_72
#pragma HLS INTERFACE axis port=FAMOut_73
#pragma HLS INTERFACE axis port=FAMOut_74
#pragma HLS INTERFACE axis port=FAMOut_75
#pragma HLS INTERFACE axis port=FAMOut_76
#pragma HLS INTERFACE axis port=FAMOut_77
#pragma HLS INTERFACE axis port=FAMOut_78
#pragma HLS INTERFACE axis port=FAMOut_79
#pragma HLS INTERFACE axis port=FAMOut_80
#pragma HLS INTERFACE axis port=FAMOut_81
#pragma HLS INTERFACE axis port=FAMOut_82
#pragma HLS INTERFACE axis port=FAMOut_83
#pragma HLS INTERFACE axis port=FAMOut_84
#pragma HLS INTERFACE axis port=FAMOut_85
#pragma HLS INTERFACE axis port=FAMOut_86
#pragma HLS INTERFACE axis port=FAMOut_87
#pragma HLS INTERFACE axis port=FAMOut_88
#pragma HLS INTERFACE axis port=FAMOut_89
#pragma HLS INTERFACE axis port=FAMOut_90
#pragma HLS INTERFACE axis port=FAMOut_91
#pragma HLS INTERFACE axis port=FAMOut_92
#pragma HLS INTERFACE axis port=FAMOut_93
#pragma HLS INTERFACE axis port=FAMOut_94
#pragma HLS INTERFACE axis port=FAMOut_95
#pragma HLS INTERFACE axis port=FAMOut_96
#pragma HLS INTERFACE axis port=FAMOut_97
#pragma HLS INTERFACE axis port=FAMOut_98
#pragma HLS INTERFACE axis port=FAMOut_99
#pragma HLS INTERFACE axis port=FAMOut_100
#pragma HLS INTERFACE axis port=FAMOut_101
#pragma HLS INTERFACE axis port=FAMOut_102
#pragma HLS INTERFACE axis port=FAMOut_103
#pragma HLS INTERFACE axis port=FAMOut_104
#pragma HLS INTERFACE axis port=FAMOut_105
#pragma HLS INTERFACE axis port=FAMOut_106
#pragma HLS INTERFACE axis port=FAMOut_107
#pragma HLS INTERFACE axis port=FAMOut_108
#pragma HLS INTERFACE axis port=FAMOut_109
#pragma HLS INTERFACE axis port=FAMOut_110
#pragma HLS INTERFACE axis port=FAMOut_111
#pragma HLS INTERFACE axis port=FAMOut_112
#pragma HLS INTERFACE axis port=FAMOut_113
#pragma HLS INTERFACE axis port=FAMOut_114
#pragma HLS INTERFACE axis port=FAMOut_115
#pragma HLS INTERFACE axis port=FAMOut_116
#pragma HLS INTERFACE axis port=FAMOut_117
#pragma HLS INTERFACE axis port=FAMOut_118
#pragma HLS INTERFACE axis port=FAMOut_119
#pragma HLS INTERFACE axis port=FAMOut_120
#pragma HLS INTERFACE axis port=FAMOut_121
#pragma HLS INTERFACE axis port=FAMOut_122
#pragma HLS INTERFACE axis port=FAMOut_123
#pragma HLS INTERFACE axis port=FAMOut_124
#pragma HLS INTERFACE axis port=FAMOut_125
#pragma HLS INTERFACE axis port=FAMOut_126
#pragma HLS INTERFACE axis port=FAMOut_127


//#pragma HLS DATAFLOW

    axis_data_32 tmp0, tmp1;
    for(int i=0; i<NSAMP_IN; i++){
    #pragma HLS PIPELINE II=1    
        tmp0.data = memin0[i];  FAMDataIn_0.write(tmp0);
        tmp1.data = memin1[i];  FAMDataIn_1.write(tmp1);
    }
    




    hls::stream<axis_data_64>* FAMOutArray[128] = {
        &FAMOut_0, &FAMOut_1, &FAMOut_2, &FAMOut_3, &FAMOut_4, &FAMOut_5, &FAMOut_6, &FAMOut_7,
        &FAMOut_8, &FAMOut_9, &FAMOut_10, &FAMOut_11, &FAMOut_12, &FAMOut_13, &FAMOut_14, &FAMOut_15,
        &FAMOut_16, &FAMOut_17, &FAMOut_18, &FAMOut_19, &FAMOut_20, &FAMOut_21, &FAMOut_22, &FAMOut_23,
        &FAMOut_24, &FAMOut_25, &FAMOut_26, &FAMOut_27, &FAMOut_28, &FAMOut_29, &FAMOut_30, &FAMOut_31,
        &FAMOut_32, &FAMOut_33, &FAMOut_34, &FAMOut_35, &FAMOut_36, &FAMOut_37, &FAMOut_38, &FAMOut_39,
        &FAMOut_40, &FAMOut_41, &FAMOut_42, &FAMOut_43, &FAMOut_44, &FAMOut_45, &FAMOut_46, &FAMOut_47,
        &FAMOut_48, &FAMOut_49, &FAMOut_50, &FAMOut_51, &FAMOut_52, &FAMOut_53, &FAMOut_54, &FAMOut_55,
        &FAMOut_56, &FAMOut_57, &FAMOut_58, &FAMOut_59, &FAMOut_60, &FAMOut_61, &FAMOut_62, &FAMOut_63,
        &FAMOut_64, &FAMOut_65, &FAMOut_66, &FAMOut_67, &FAMOut_68, &FAMOut_69, &FAMOut_70, &FAMOut_71,
        &FAMOut_72, &FAMOut_73, &FAMOut_74, &FAMOut_75, &FAMOut_76, &FAMOut_77, &FAMOut_78, &FAMOut_79,
        &FAMOut_80, &FAMOut_81, &FAMOut_82, &FAMOut_83, &FAMOut_84, &FAMOut_85, &FAMOut_86, &FAMOut_87,
        &FAMOut_88, &FAMOut_89, &FAMOut_90, &FAMOut_91, &FAMOut_92, &FAMOut_93, &FAMOut_94, &FAMOut_95,
        &FAMOut_96, &FAMOut_97, &FAMOut_98, &FAMOut_99, &FAMOut_100, &FAMOut_101, &FAMOut_102, &FAMOut_103,
        &FAMOut_104, &FAMOut_105, &FAMOut_106, &FAMOut_107, &FAMOut_108, &FAMOut_109, &FAMOut_110, &FAMOut_111,
        &FAMOut_112, &FAMOut_113, &FAMOut_114, &FAMOut_115, &FAMOut_116, &FAMOut_117, &FAMOut_118, &FAMOut_119,
        &FAMOut_120, &FAMOut_121, &FAMOut_122, &FAMOut_123, &FAMOut_124, &FAMOut_125, &FAMOut_126, &FAMOut_127
    };
    #pragma HLS array_partition variable=FAMOutArray complete
    //#pragma HLS PIPELINE II=1
    //#pragma HLS UNROLL factor=8
    //#pragma HLS UNROLL
    
    for (int i = 0; i < 513; i++) {
    #pragma HLS PIPELINE II=1        
    for (int ch = 0; ch < 128; ch++) {
        #pragma HLS PIPELINE II=1           
        ap_uint<512> tmp1 = 0;  // 初始化为0
        
        if (i < 512) {
            // 主数据写入：每个 512 位字内读入8个 axis_data_64
            for (int k = 0; k < 8; k++) {
                #pragma HLS UNROLL factor=4
                axis_data_64 x1 = FAMOutArray[ch]->read();
                tmp1(64 * k + 63, 64 * k) = x1.data;
            }
        } else {
            // i == 512：尾部数据写入，只读2个 axis_data_64（其总宽度128位），其他位保留为0
            for (int j = 0; j < 2; j++) {
                #pragma HLS UNROLL factor=4
                axis_data_64 x1 = FAMOutArray[ch]->read();
                tmp1(64 * j + 63, 64 * j) = x1.data;
            }
        }
        int base_addr = ch * 513 + i;  // 每个通道分配 1025 个连续地址
        memout[base_addr] = tmp1;
    }
    }

}