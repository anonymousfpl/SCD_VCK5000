#include <cufft.h>
#include <cufftXt.h>
#include <cuda_runtime.h>
#include <iostream>

#include "common.h"


constexpr int N = 1<<20;
constexpr int Np = 64;
constexpr int FFT1_SIZE = Np;

__device__ __host__ inline 
cufftComplex ComplexMul(cufftComplex a, cufftComplex b)
{
    cufftComplex c;
    c.x = a.x * b.x - a.y * b.y;
    c.y = a.x * b.y + a.y * b.x;
    return c;
}


__device__ __host__ inline 
cufftComplex ComplexCMul(cufftComplex a, cufftComplex b)
{
    cufftComplex c;
    c.x = a.x * b.x + a.y * b.y;
    c.y = - a.x * b.y + a.y * b.x;
    return c;
}

__device__ cufftComplex window_callback(void *dataIn, size_t offset, void *callerInfo, void *sharedPtr) {
    static __constant__ float chebwin_128[64] = {
    0.000380749937797,0.000878369075872,0.001843297143275,0.003447797656931,0.005956253668122,0.009691337887956,0.015033089879754,0.022413859893738,
    0.032308562430926,0.045219940772657,0.061658868688135,0.082120089136043,0.107054187233385,0.136836986271137,0.171737908447306,0.211889123375446,
    0.257257487088739,0.307621327529376,0.362554042892542,0.421416240431513,0.483357760728313,0.547330423224679,0.612111721499399,0.676339029480229,
    0.738553197603042,0.797249769654733,0.850935485298114,0.898187294620820,0.937710836166540,0.968395244405256,0.989361268614028,1.000000000000000,
    1.000000000000000,0.989361268614028,0.968395244405256,0.937710836166540,0.898187294620820,0.850935485298114,0.797249769654733,0.738553197603042,
    0.676339029480229,0.612111721499399,0.547330423224679,0.483357760728313,0.421416240431513,0.362554042892542,0.307621327529376,0.257257487088739,
    0.211889123375446,0.171737908447306,0.136836986271137,0.107054187233385,0.082120089136043,0.061658868688135,0.045219940772657,0.032308562430926,
    0.022413859893738,0.015033089879754,0.009691337887956,0.005956253668122,0.003447797656931,0.001843297143275,0.000878369075872,0.000380749937797,
    };
    cufftComplex input = static_cast<cufftComplex*>(dataIn)[offset];
    // float *filter = static_cast<float*>(callerInfo);  // The scale factor is passed via callerInfo
    cufftComplex output;
    output.x = input.x * chebwin_128[offset%Np];
    output.y = input.y * chebwin_128[offset%Np];
    return output;
}

__device__ void transpose_callback(void *dataOut, size_t offset, cufftComplex element, void *callerInfo, void *sharedPtr) {
    // Down conversion first then conjugate multiplication and transpose
    cufftComplex *x = static_cast<cufftComplex*>(callerInfo);
    size_t row = offset / FFT1_SIZE;
    size_t col = (offset + FFT1_SIZE/2) % FFT1_SIZE;// include FFTshift
    float theta = -2.0*M_PI* (int(col) - Np/2) *row/Np;
    cufftComplex texp = make_cuFloatComplex(cosf(theta), sinf(theta));
    cufftComplex tempx = x[row + FFT1_SIZE/2];
    cufftComplex temp = ComplexMul(element, texp);
    temp = ComplexCMul(temp, tempx);
    // transpose
    size_t idx = col * N + row;
    static_cast<cufftComplex*>(dataOut)[idx] = temp;
}


__device__ 
cufftCallbackLoadC d_loadCallbackPtr = window_callback; 
__device__ 
cufftCallbackStoreC d_storeCallbackPtr = transpose_callback;




int main() {

    cufftHandle plan, plan2;
    size_t work_size, work_size2;
    float milliseconds2;
    cufftComplex *d_fft1;
    cufftComplex *d_fft2;
    cufftComplex *d_data;

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    const int TSIZE  = N * Np;
    // Allocate device memory
    CHECK_ERROR(cudaMallocManaged(&d_fft1, sizeof(cufftComplex) * TSIZE));
    CHECK_ERROR(cudaMallocManaged(&d_fft2, sizeof(cufftComplex) * TSIZE));
    CHECK_ERROR(cudaMallocManaged(&d_data, sizeof(cufftComplex) * (N + Np -1)));
    // Set initial value
    
    for (int i = 0; i < N + Np-1; i++){
        if(i<N){
            d_data[i].x = float(i)/1024;
            d_data[i].y = float(i)/1024;
        }else{
            d_data[i].x = float(0);
            d_data[i].y = float(0);
        }
        // std::cout << "h_data[" << i << "] = " << h_data[i].x << ", " << h_data[i].y<<"j" << std::endl;
    }
    for (int i = 0; i< N ; i++){
        for (int j = 0; j< Np; j++){
            d_fft1[i*Np +j] = d_data[i+j];
            // std::cout << "input[" << i*Np +j << "] = " << h_input[i*Np +j].x << ", " << h_input[i*Np +j].y << "j" <<std::endl;
        }
    }


    // Copy data from host to device
    cudaEventRecord(start, 0);

    for (int itr = 0; itr<1; itr++){

    // Create cuFFT plan
    cufftCreate(&plan);
    cufftMakePlan1d(plan, Np, CUFFT_C2C, N, &work_size);
    cufftCreate(&plan2);
    cufftMakePlan1d(plan2, N, CUFFT_C2C, Np, &work_size2);

    // cudaEventRecord(start, 0);
    // Preprocessing callback setup
    cufftCallbackLoadC h_windowCallback;
    CHECK_ERROR(cudaMemcpyFromSymbol(&h_windowCallback, d_loadCallbackPtr, sizeof(h_windowCallback)));
    CHECK_ERROR(cufftXtSetCallback(plan, (void **)&h_windowCallback, CUFFT_CB_LD_COMPLEX, nullptr));

    // Postprocessing callback setup
    cufftCallbackStoreC h_postprocessingCallback;
    CHECK_ERROR(cudaMemcpyFromSymbol(&h_postprocessingCallback, d_storeCallbackPtr, sizeof(h_postprocessingCallback)));
    CHECK_ERROR(cufftXtSetCallback(plan, (void **)&h_postprocessingCallback, CUFFT_CB_ST_COMPLEX, (void **)&d_data));

    // Execute the FFT
    CHECK_ERROR(cufftExecC2C(plan, d_fft1, d_fft2, CUFFT_FORWARD));
    CHECK_ERROR(cufftExecC2C(plan2, d_fft2, d_fft2, CUFFT_FORWARD));
    }
    // Copy data from device to host
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    milliseconds2 = 0;
    cudaEventElapsedTime(&milliseconds2, start, stop);
    std::cout << "Elapsed time: " << milliseconds2 << " ms" << std::endl;

    // Clean up
    cufftDestroy(plan);
    cufftDestroy(plan2);
    cudaFree(d_fft1);
    cudaFree(d_fft2);
    cudaFree(d_data);

    std::cout << "----------------FFT executed with callbacks.-------------" << std::endl;
    return 0;
}

// nvcc -ccbin g++ -std=c++17 -arch sm_86 -O3 -dc -m64 -o callback_example.o -c callback_example.cu
// nvcc -ccbin g++ -arch sm_86 -o callback_example callback_example.o -lcufft_static -lculibos


