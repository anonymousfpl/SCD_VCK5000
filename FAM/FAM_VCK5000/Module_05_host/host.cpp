/***************************************************************************/
/*  Copyright (C) 2023 Advanced Micro Devices, Inc
 *  SPDX-License-Identifier: MIT
 *
 *  Example: Read 1024 cfloat from local files (FAMDataIn_0.txt ~ FAMDataIn_7.txt)
 *  for 8 input channels, copy to 8 BOs under the same group_id(0);
 *  output as a single BO of 128×8192 cfloat.
 */
/***************************************************************************/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <chrono>   // 引入计时头文件
#include "xrt.h"
#include "experimental/xrt_kernel.h"

//-----------------------------------------------
// Input/Output parameters (modifiable as needed)
//-----------------------------------------------
static const int N_IN_CH    = 2;    // 2 输入通道
static const int NSAMPLES   = 1024; // 每个通道 1024 个 float
static const int NPSIZE     = 0;    // 如需填充，设置非零值

static const int N_OUT_CH   = 128;  
static const int NSAMP_OUT  = 8192; // 每个输出通道 8192 个 float

static const int ITER       = 10000;    // 执行迭代次数

int main(int argc, char** argv)
{
    // 记录总时间起点
    auto t_total_start = std::chrono::high_resolution_clock::now();

    //-------------------------------------
    // 1) Open the device
    //-------------------------------------
    std::cout << "Open the device...\n";
    auto t_dev_open_start = std::chrono::high_resolution_clock::now();
    auto device = xrt::device(0);
    auto t_dev_open_end = std::chrono::high_resolution_clock::now();
    std::cout << "Device opened.\n";

    //-------------------------------------
    // 2) Load the xclbin
    //-------------------------------------
    std::string xclbin = "../build.hw/fam.xclbin";
    std::cout << "Load the xclbin: " << xclbin << std::endl;
    auto t_xclbin_start = std::chrono::high_resolution_clock::now();
    auto uuid = device.load_xclbin(xclbin);
    auto t_xclbin_end = std::chrono::high_resolution_clock::now();

    //-------------------------------------
    // 3) Get the kernel
    //-------------------------------------
    std::cout << "Get references to datamovers compute units\n";
    auto t_kernel_get_start = std::chrono::high_resolution_clock::now();
    auto dma0 = xrt::kernel(device, uuid, "dma_hls:{dma_hls_0}");
    auto t_kernel_get_end = std::chrono::high_resolution_clock::now();

    //-------------------------------------
    // 4) Define input/output BO sizes
    //-------------------------------------
    size_t in_size_bytes = (NSAMPLES + NPSIZE) * 1 * sizeof(float);
    size_t out_size_bytes = (size_t)N_OUT_CH * (NSAMP_OUT + 16) * 1 * sizeof(float);
    std::cout << "[Info] Input BO size per channel = " << in_size_bytes << " bytes\n";
    std::cout << "[Info] Output BO size = " << out_size_bytes << " bytes (for 128×8192 cfloat)\n";

    //-------------------------------------
    // 5) Read input files into host buffer
    //-------------------------------------
    auto t_file_read_start = std::chrono::high_resolution_clock::now();
    // 为每个输入通道分配一个二维容器，尺寸为 [NSAMPLES+NPSIZE]
    std::vector<std::vector<float>> sample_vector_in(N_IN_CH,
        std::vector<float>((NSAMPLES + NPSIZE)*1, 0.0f)
    );
    for (int ch = 0; ch < N_IN_CH; ch++){
        std::string fname = "./data/FAMDataIn" + std::to_string(ch) + ".txt";
        std::ifstream fin(fname);
        if(!fin.is_open()){
            std::cerr << "Error: Could not open " << fname << "\n";
            return 1;
        }
        float newval;
        for(size_t ii = 0; ii < (size_t)(NSAMPLES + NPSIZE); ii++){
            if(ii < (size_t)NSAMPLES){
                if(!(fin >> newval)){
                    newval = 0.0f;
                }
                sample_vector_in[ch][ii] = newval;
            }
            else {  // 填充部分
                sample_vector_in[ch][ii] = 0.0f;
            }
        }
        fin.close();
    }
    auto t_file_read_end = std::chrono::high_resolution_clock::now();

    //-------------------------------------
    // 6) Allocate BO on FPGA global memory and map them
    //-------------------------------------
    auto t_bo_alloc_start = std::chrono::high_resolution_clock::now();
    auto in_bohdl0 = xrt::bo(device, in_size_bytes, dma0.group_id(0));
    auto in_bohdl1 = xrt::bo(device, in_size_bytes, dma0.group_id(0));
    auto out_bohdl  = xrt::bo(device, out_size_bytes, dma0.group_id(0));
    auto in_bomapped0 = in_bohdl0.map<float*>();
    auto in_bomapped1 = in_bohdl1.map<float*>();
    auto out_bomapped = out_bohdl.map<float*>();
    auto t_bo_alloc_end = std::chrono::high_resolution_clock::now();

    //-------------------------------------
    // 7) Copy sample data to BO and sync to device (Host->Device transfer)
    //-------------------------------------
    auto t_sync_in_start = std::chrono::high_resolution_clock::now();
    // 通道 0
    for(size_t k = 0; k < (size_t)(NSAMPLES + NPSIZE); k++){
        in_bomapped0[k] = sample_vector_in[0][k];
    }
    // 通道 1
    for(size_t k = 0; k < (size_t)(NSAMPLES + NPSIZE); k++){
        in_bomapped1[k] = sample_vector_in[1][k];
    }
    in_bohdl0.sync(XCL_BO_SYNC_BO_TO_DEVICE, in_size_bytes, 0);
    in_bohdl1.sync(XCL_BO_SYNC_BO_TO_DEVICE, in_size_bytes, 0);
    auto t_sync_in_end = std::chrono::high_resolution_clock::now();

    //-------------------------------------
    // 8) Launch the kernel iterations and measure kernel execution time
    //-------------------------------------
    std::cout << "Launching Kernel...\n";
    auto t_kernel_start = std::chrono::high_resolution_clock::now();
    for(int i=0; i< ITER; i++){
       auto dma_run0 = dma0(
            in_bohdl0, in_bohdl1,      // 两个输入 BO
            out_bohdl,   
            nullptr, nullptr,                                                        // input streams
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, // output streams
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            1 // iter
        );
       dma_run0.wait();
    }
    auto t_kernel_end = std::chrono::high_resolution_clock::now();

    //-------------------------------------
    // 9) Sync output BO from device (Device->Host transfer)
    //-------------------------------------
    auto t_sync_out_start = std::chrono::high_resolution_clock::now();
    out_bohdl.sync(XCL_BO_SYNC_BO_FROM_DEVICE, out_size_bytes, 0);
    auto t_sync_out_end = std::chrono::high_resolution_clock::now();

    //-------------------------------------
    // 10) Write output data to a single txt file
    //-------------------------------------
    auto t_file_write_start = std::chrono::high_resolution_clock::now();
    std::string outfname = "./output1/FAMOut_all.txt";
    FILE* fout = fopen(outfname.c_str(), "w");
    if (!fout) {
        std::cerr << "Error: cannot open " << outfname << "\n";
        return 1;
    }
    // 输出BO中存放的是 float 数组，写入到一个txt文件中，每个数一行
    size_t total_floats = out_size_bytes / sizeof(float);
    for (size_t i = 0; i < total_floats; i++){
         fprintf(fout, "%f\n", out_bomapped[i]);
    }
    fclose(fout);
    auto t_file_write_end = std::chrono::high_resolution_clock::now();

    //-------------------------------------
    // 11) Calculate and print timing results
    //-------------------------------------
    auto t_total_end = std::chrono::high_resolution_clock::now();
    double t_dev_open = std::chrono::duration<double>(t_dev_open_end - t_dev_open_start).count();
    double t_xclbin = std::chrono::duration<double>(t_xclbin_end - t_xclbin_start).count();
    double t_kernel_get = std::chrono::duration<double>(t_kernel_get_end - t_kernel_get_start).count();
    double t_file_read = std::chrono::duration<double>(t_file_read_end - t_file_read_start).count();
    double t_bo_alloc = std::chrono::duration<double>(t_bo_alloc_end - t_bo_alloc_start).count();
    double t_sync_in = std::chrono::duration<double>(t_sync_in_end - t_sync_in_start).count();
    double t_kernel = std::chrono::duration<double>(t_kernel_end - t_kernel_start).count();
    double t_sync_out = std::chrono::duration<double>(t_sync_out_end - t_sync_out_start).count();
    double t_file_write = std::chrono::duration<double>(t_file_write_end - t_file_write_start).count();
    double t_total = std::chrono::duration<double>(t_total_end - t_total_start).count();

    std::cout << "\nTiming Breakdown:" << std::endl;
    std::cout << "  Device open:          " << t_dev_open << " s" << std::endl;
    std::cout << "  xclbin load:          " << t_xclbin << " s" << std::endl;
    std::cout << "  Kernel get:           " << t_kernel_get << " s" << std::endl;
    std::cout << "  File read:            " << t_file_read << " s" << std::endl;
    std::cout << "  BO allocation/mapping:" << t_bo_alloc << " s" << std::endl;
    std::cout << "  Sync input (H->D):    " << t_sync_in << " s" << std::endl;
    std::cout << "  Kernel execution:     " << t_kernel << " s (average " << (t_kernel/ITER) << " s per iteration)" << std::endl;
    std::cout << "  Sync output (D->H):   " << t_sync_out << " s" << std::endl;
    std::cout << "  File write:           " << t_file_write << " s" << std::endl;
    std::cout << "  Total time:           " << t_total << " s" << std::endl;

    std::cout << "TEST PASS - Data saved in file: " << outfname << "\n";
    return 0;
}
