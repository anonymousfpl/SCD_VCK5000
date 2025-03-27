/******************************************************************************
 * 全 GPU 版示例：  
 *   1) GPU 端生成 1024 个随机复数并归一化  
 *   2) 复制到 2048 点，再零填到 2240 点  
 *   3) 切成 256×32 矩阵 (步长 L=64, 列优先)  
 *   4) 窗口 → FFT+fftshift → DC 系数 → 转置 → CM+32点 FFT
 *
 * 编译示例:
 *   nvcc -ccbin g++ -std=c++17 -arch=sm_70 -O3 \
 *        -o fam fam.cu -lcufft -lcudart -lcurand
 ******************************************************************************/
//nvcc -ccbin g++ -std=c++17 -arch=sm_70 -O3 -o bin/fam fam.cu -lcufft -lcudart -lcurand
//./bin/fam
//nvprof bin/fam
#include <iostream>
#include <cstdlib>
#include <cuda_runtime.h>
#include <cufft.h>
#include <curand.h>
#include <math.h>

// 错误检查宏
#define CHECK_CUDA_ERROR(call)                                \
    do {                                                      \
        cudaError_t err = call;                              \
        if (err != cudaSuccess) {                            \
            std::cerr << "CUDA Error: " << cudaGetErrorString(err) \
                      << " at " << __FILE__ << ":" << __LINE__ \
                      << std::endl;                          \
            exit(EXIT_FAILURE);                              \
        }                                                     \
    } while(0)

#define CHECK_CUFFT_ERROR(call)                               \
    do {                                                      \
        cufftResult err = call;                              \
        if (err != CUFFT_SUCCESS) {                          \
            std::cerr << "CUFFT Error: " << err              \
                      << " at " << __FILE__ << ":" << __LINE__ \
                      << std::endl;                          \
            exit(EXIT_FAILURE);                              \
        }                                                     \
    } while(0)

#define CHECK_CURAND_ERROR(call)                              \
    do {                                                      \
        curandStatus_t err = call;                           \
        if (err != CURAND_STATUS_SUCCESS) {                   \
            std::cerr << "cuRAND Error: " << err             \
                      << " at " << __FILE__ << ":" << __LINE__ \
                      << std::endl;                          \
            exit(EXIT_FAILURE);                              \
        }                                                     \
    } while(0)

// --------------------------- 全局常量 --------------------------- //
static const int origLen = 1024;                 // 原始点数
static const int doubleLen = origLen * 2;        // 2048
static const int P = 32;                         // 矩阵列数
static const int L = 64;                         // 步长
static const int Np = 256;                       // 每列长度
static const int NN = (P - 1) * L + Np;           // 2240 = 31*64 + 256

// --------------------------- 复数运算 --------------------------- //
__device__ cufftComplex ComplexMul(cufftComplex a, cufftComplex b)
{
    cufftComplex c;
    c.x = a.x * b.x - a.y * b.y;
    c.y = a.x * b.y + a.y * b.x;
    return c;
}

// 在 CM 步骤里可能需要 conj(a)*b，保留一个示例
__device__ __host__ inline
cufftComplex ComplexCMul(cufftComplex a, cufftComplex b)
{
    cufftComplex c;
    c.x = a.x * b.x + a.y * b.y;   
    c.y = - a.x * b.y + a.y * b.x;
    return c;
}

// --------------------------- DC 系数，保存在常量内存 --------------------------- //
alignas(32) __constant__ cufftComplex dc_coef1[4] = {
    {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}
};
alignas(32) __constant__ cufftComplex dc_coef2[4] = {
    {1.0f, 0.0f}, {0.0f, -1.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}
};
alignas(32) __constant__ cufftComplex dc_coef3[4] = {
    {1.0f, 0.0f}, {-1.0f,0.0f}, {1.0f, 0.0f}, {-1.0f,0.0f}
};
alignas(32) __constant__ cufftComplex dc_coef4[4] = {
    {1.0f, 0.0f}, {0.0f, 1.0f}, {-1.0f,0.0f}, {0.0f, -1.0f}
};

// --------------------------- (A) 生成 1024 随机复数并归一化 --------------------------- //
// 1) 先用 cuRAND 生成 2*origLen 个 [0,1) 浮点，转为 [-1,1] 存到 d_x
__global__ void genComplexInMinusOneToOne(const float* __restrict__ d_rand,
                                          cufftComplex* __restrict__ d_x,
                                          int n)
{
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx < n) {
        float r  = 2.f*(d_rand[2*idx]  - 0.5f); // [-1,1]
        float im = 2.f*(d_rand[2*idx+1]- 0.5f);
        d_x[idx] = make_cuFloatComplex(r, im);
    }
}

// 2) 幅度平方
__global__ void computeMagSquared(const cufftComplex* __restrict__ d_in,
                                  float* __restrict__ d_out, int n)
{
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx < n) {
        cufftComplex c = d_in[idx];
        d_out[idx] = c.x*c.x + c.y*c.y;
    }
}

// 3) reduceMax 简单实现：块内归约 => 写回 d_data[blockIdx.x]
__global__ void reduceMax(float* d_data, int n)
{
    extern __shared__ float sdata[]; // 动态共享内存
    int tid = threadIdx.x;
    int idx = blockDim.x * blockIdx.x + threadIdx.x;

    if (idx < n) {
        sdata[tid] = d_data[idx];
    } else {
        sdata[tid] = 0.0f;
    }
    __syncthreads();

    for(int stride = blockDim.x/2; stride>0; stride >>= 1){
        if(tid < stride){
            if(sdata[tid] < sdata[tid+stride]){
                sdata[tid] = sdata[tid+stride];
            }
        }
        __syncthreads();
    }

    if(tid==0){
        d_data[blockIdx.x] = sdata[0];
    }
}

// 4) 归一化
__global__ void normalizeComplex(cufftComplex* d_x, float maxAmp, float Amp, int n)
{
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if(idx < n){
        cufftComplex c = d_x[idx];
        d_x[idx].x = Amp*(c.x / maxAmp);
        d_x[idx].y = Amp*(c.y / maxAmp);
    }
}

// --------------------------- (B) 复制 => zero-pad => 构建 256×32 矩阵 --------------------------- //
// 1) 复制: d_out[:n]=d_in, d_out[n:2n]=d_in
__global__ void replicateSignal(const cufftComplex* d_in, cufftComplex* d_out, int n)
{
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if(idx < n){
        d_out[idx]   = d_in[idx];
        d_out[idx+n] = d_in[idx];
    }
}

// 2) zero-pad 到 NN=2240
__global__ void zeroPad(const cufftComplex* d_in, cufftComplex* d_out,
                        int oldLen, int newLen)
{
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if(idx<newLen){
        if(idx<oldLen)
            d_out[idx]=d_in[idx];
        else
            d_out[idx]=make_cuFloatComplex(0.f,0.f);
    }
}

// 3) 根据步长 L=64, 每列 256 点，构建 256×32
__global__ void buildOverlapMatrix(const cufftComplex* __restrict__ d_src,
                                   cufftComplex* __restrict__ d_matrix,
                                   int rows, int L, int P, int totalLen)
{
    // rows=256, P=32
    int col = blockIdx.x;   // [0..31]
    int row = threadIdx.x;  // [0..255]
    if(col<P && row<rows){
        int start = col * L;
        int idxDest = col*rows + row; // 列优先
        if(start+row < totalLen){
            d_matrix[idxDest] = d_src[start+row];
        } else {
            d_matrix[idxDest] = make_cuFloatComplex(0.f,0.f);
        }
    }
}

// --------------------------- (C) Window, FFT+shift, DC, 转置, CM+32点FFT --------------------------- //
// （以下逻辑保留你原来的代码）

// 窗口 kernel
__global__ void windowingKernel(cufftComplex *d_matrix, const float *d_window, int rows, int cols)
{
    int col = blockIdx.x * blockDim.x + threadIdx.x; 
    int row = blockIdx.y * blockDim.y + threadIdx.y; 
    if (row < rows && col < cols) {
        int index = col * rows + row;
        float w = d_window[row];
        d_matrix[index].x *= w;
        d_matrix[index].y *= w;
    }
}
void applyWindow(cufftComplex *d_matrix, const float *d_window, int rows, int cols)
{
    dim3 blockDim(16, 16);
    dim3 gridDim((cols + blockDim.x - 1) / blockDim.x,
                 (rows + blockDim.y - 1) / blockDim.y);
    windowingKernel<<<gridDim, blockDim>>>(d_matrix, d_window, rows, cols);
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
}

// FFT + fftshift
__global__ void fftShiftKernelSimple(cufftComplex* in, cufftComplex* out, int rows, int cols) {
    int offset = blockIdx.x * blockDim.x + threadIdx.x;
    int total  = rows * cols;
    if (offset < total) {
        int col = offset / rows;
        int row = offset % rows;
        int newRow = (row + (rows / 2)) % rows;
        int destIdx = col * rows + newRow;
        out[destIdx] = in[offset];
    }
}
void performFFT(cufftComplex *d_matrix, int rows, int cols)
{
    cufftHandle plan;
    int n[1] = { rows };
    CHECK_CUFFT_ERROR(cufftPlanMany(&plan, 1, n,
                      nullptr, 1, rows,
                      nullptr, 1, rows,
                      CUFFT_C2C, cols));
    CHECK_CUFFT_ERROR(cufftExecC2C(plan, d_matrix, d_matrix, CUFFT_FORWARD));
    CHECK_CUFFT_ERROR(cufftDestroy(plan));

    // fftshift
    size_t bytes = rows * cols * sizeof(cufftComplex);
    cufftComplex* d_temp = nullptr;
    CHECK_CUDA_ERROR(cudaMalloc((void**)&d_temp, bytes));

    int total = rows * cols;
    int blockSize = 256;
    int gridSize = (total + blockSize - 1) / blockSize;
    fftShiftKernelSimple<<<gridSize, blockSize>>>(d_matrix, d_temp, rows, cols);
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());

    CHECK_CUDA_ERROR(cudaMemcpy(d_matrix, d_temp, bytes, cudaMemcpyDeviceToDevice));
    CHECK_CUDA_ERROR(cudaFree(d_temp));
}

// DC 系数乘法
__global__ void applyDCCoefKernel(cufftComplex *d_matrix, int rows, int cols)
{
    int col = blockIdx.x * blockDim.x + threadIdx.x; 
    int row = blockIdx.y * blockDim.y + threadIdx.y; 
    if (row < rows && col < cols) {
        int idx = col * rows + row;
        int patternIdx = row % 4;    
        int coefCol    = col % 4;    

        cufftComplex coef;
        if (coefCol == 0) {
            coef = dc_coef1[patternIdx];
        } else if (coefCol == 1) {
            coef = dc_coef2[patternIdx];
        } else if (coefCol == 2) {
            coef = dc_coef3[patternIdx];
        } else {
            coef = dc_coef4[patternIdx];
        }
        d_matrix[idx] = ComplexMul(d_matrix[idx], coef);
    }
}
void applyDCCoef(cufftComplex *d_matrix, int rows, int cols)
{
    dim3 blockDim(16, 16);
    dim3 gridDim((cols + blockDim.x - 1) / blockDim.x,
                 (rows + blockDim.y - 1) / blockDim.y);
    applyDCCoefKernel<<<gridDim, blockDim>>>(d_matrix, rows, cols);
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
}

// 转置
__global__ void transposeKernel(const cufftComplex *in, cufftComplex *out,
                                int oldRows, int oldCols)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x; 
    int j = blockIdx.y * blockDim.y + threadIdx.y; 
    if (i < oldCols && j < oldRows) {
        int idx_in  = i * oldRows + j; 
        int idx_out = j * oldCols + i; 
        out[idx_out] = in[idx_in];
    }
}
void transposeMatrix(cufftComplex *d_in, cufftComplex *d_out, int oldRows, int oldCols)
{
    dim3 blockDim(16, 16);
    dim3 gridDim((oldCols + blockDim.x - 1) / blockDim.x,
                 (oldRows + blockDim.y - 1) / blockDim.y);
    transposeKernel<<<gridDim, blockDim>>>(d_in, d_out, oldRows, oldCols);
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
}

// CM+32点 FFT
__global__ void cmMulKernel(const cufftComplex* d_transposed, cufftComplex* d_cmIn)
{
    int pos    = threadIdx.x;
    int target = blockIdx.x;
    int ref    = blockIdx.y;

    int transformId = ref * 256 + target;
    int outIndex    = transformId * 32 + pos;

    int refIndex    = ref    * 32 + pos;
    int targetIndex = target * 32 + pos;

    cufftComplex a = d_transposed[refIndex];
    cufftComplex b = d_transposed[targetIndex];
    cufftComplex a_conj = make_cuFloatComplex(a.x, -a.y);

    d_cmIn[outIndex] = ComplexMul(a_conj, b);
}
void performCM32ptFFT(const cufftComplex* d_transposed, cufftComplex** d_cmFFTResult)
{
    int numTransforms = 256 * 256;
    int fftSize       = 32;
    size_t totalBytes = numTransforms * fftSize * sizeof(cufftComplex);

    CHECK_CUDA_ERROR(cudaMalloc((void**)d_cmFFTResult, totalBytes));

    dim3 gridDim(256, 256);
    dim3 blockDim(32);
    cmMulKernel<<<gridDim, blockDim>>>(d_transposed, *d_cmFFTResult);
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());

    cufftHandle plan;
    int n[1] = { fftSize };
    CHECK_CUFFT_ERROR(cufftPlanMany(&plan, 1, n,
                      nullptr, 1, fftSize,
                      nullptr, 1, fftSize,
                      CUFFT_C2C, numTransforms));
    CHECK_CUFFT_ERROR(cufftExecC2C(plan, *d_cmFFTResult, *d_cmFFTResult, CUFFT_FORWARD));
    CHECK_CUFFT_ERROR(cufftDestroy(plan));
}

// ============================================================================
// (D) 对 65536×32 个复数结果做幅度平方 (abs^2)
// ============================================================================
__global__ void computeAbsSquareKernel(const cufftComplex* d_in, float* d_out, int total)
{
    int idx = blockDim.x*blockIdx.x + threadIdx.x;
    if(idx<total){
        cufftComplex c = d_in[idx];
        d_out[idx] = c.x*c.x + c.y*c.y; // abs^2
    }
}

// 对外接口：
//   d_in  : CM+32pt FFT 完成后得到的数组指针
//   d_out : 输出 float 数组指针 (本函数会在 GPU 上自行分配)
//   N     : 数组长度(65536×32)
void computeAbsSquareAll(const cufftComplex* d_in, float** d_out, int N)
{
    // 1) 在 GPU 上分配存放 abs^2 结果的 float 数组
    CHECK_CUDA_ERROR(cudaMalloc((void**)d_out, N*sizeof(float)));

    // 2) 每个线程处理 1 个复数 => 计算 abs^2
    dim3 block(256);
    dim3 grid((N+block.x-1)/block.x);
    computeAbsSquareKernel<<<grid, block>>>(d_in, *d_out, N);
    CHECK_CUDA_ERROR(cudaDeviceSynchronize());
}


// --------------------------- 主函数 --------------------------- //
int main()
{
    // 创建 CUDA event 用于计时
    cudaEvent_t startEvent, stopEvent;
    CHECK_CUDA_ERROR(cudaEventCreate(&startEvent));
    CHECK_CUDA_ERROR(cudaEventCreate(&stopEvent));
    float elapsedTime = 0.f;
    
    // 在进入迭代前记录整个流程的起始时间
    CHECK_CUDA_ERROR(cudaEventRecord(startEvent, 0));

    // 循环 10 次，重复整个处理流程
    for (int iter = 0; iter < 1000; iter++) {
        std::cout << "Iteration " << iter << std::endl;
        
        // --------------------------- (A) 生成 1024 随机复数并归一化 ---------------------------
        // 1) 分配 1024 个复数
        cufftComplex* d_x = nullptr;
        CHECK_CUDA_ERROR(cudaMalloc((void**)&d_x, origLen * sizeof(cufftComplex)));

        // 2) 分配 2*origLen 个浮点数，生成随机数
        float* d_rand = nullptr;
        CHECK_CUDA_ERROR(cudaMalloc((void**)&d_rand, 2 * origLen * sizeof(float)));

        curandGenerator_t gen;
        CHECK_CURAND_ERROR(curandCreateGenerator(&gen, CURAND_RNG_PSEUDO_DEFAULT));
        CHECK_CURAND_ERROR(curandSetPseudoRandomGeneratorSeed(gen, 1234ULL));
        CHECK_CURAND_ERROR(curandGenerateUniform(gen, d_rand, 2 * origLen));

        // 3) 将 [0,1) 转换到 [-1,1]，生成复数存入 d_x
        {
            dim3 block(256);
            dim3 grid((origLen + block.x - 1) / block.x);
            genComplexInMinusOneToOne<<<grid, block>>>(d_rand, d_x, origLen);
            CHECK_CUDA_ERROR(cudaDeviceSynchronize());
        }
        CHECK_CUDA_ERROR(cudaFree(d_rand));
        CHECK_CURAND_ERROR(curandDestroyGenerator(gen));

        // 4) 计算幅度平方，归约求最大值并归一化
        float* d_mag2 = nullptr;
        CHECK_CUDA_ERROR(cudaMalloc((void**)&d_mag2, origLen * sizeof(float)));
        {
            dim3 block(256);
            dim3 grid((origLen + block.x - 1) / block.x);
            computeMagSquared<<<grid, block>>>(d_x, d_mag2, origLen);
            CHECK_CUDA_ERROR(cudaDeviceSynchronize());
        }
        int currSize = origLen;
        int blockSize = 256;
        while (currSize > 1) {
            int gridSize = (currSize + blockSize - 1) / blockSize;
            reduceMax<<<gridSize, blockSize, blockSize * sizeof(float)>>>(d_mag2, currSize);
            CHECK_CUDA_ERROR(cudaDeviceSynchronize());
            currSize = gridSize;
        }
        float maxVal2;
        CHECK_CUDA_ERROR(cudaMemcpy(&maxVal2, d_mag2, sizeof(float), cudaMemcpyDeviceToHost));
        float maxVal = sqrtf(maxVal2);

        {
            float Amp = 1.0f; // 放大因子
            dim3 block(256);
            dim3 grid((origLen + block.x - 1) / block.x);
            normalizeComplex<<<grid, block>>>(d_x, maxVal, Amp, origLen);
            CHECK_CUDA_ERROR(cudaDeviceSynchronize());
        }
        CHECK_CUDA_ERROR(cudaFree(d_mag2));

        // --------------------------- (B) 复制 => zero-pad => 构建 256×32 矩阵 ---------------------------
        // 1) 复制到 2048（即把原始信号复制一遍）
        cufftComplex* d_x2 = nullptr;
        CHECK_CUDA_ERROR(cudaMalloc((void**)&d_x2, doubleLen * sizeof(cufftComplex)));
        {
            dim3 block(256);
            dim3 grid((origLen + block.x - 1) / block.x);
            replicateSignal<<<grid, block>>>(d_x, d_x2, origLen);
            CHECK_CUDA_ERROR(cudaDeviceSynchronize());
        }
        CHECK_CUDA_ERROR(cudaFree(d_x)); // 已复制完

        // 2) zero-pad 到 2240 点
        cufftComplex* d_xx = nullptr;
        CHECK_CUDA_ERROR(cudaMalloc((void**)&d_xx, NN * sizeof(cufftComplex)));
        {
            dim3 block(256);
            dim3 grid((NN + block.x - 1) / block.x);
            zeroPad<<<grid, block>>>(d_x2, d_xx, doubleLen, NN);
            CHECK_CUDA_ERROR(cudaDeviceSynchronize());
        }
        CHECK_CUDA_ERROR(cudaFree(d_x2));

        // 3) 构造 256×32 矩阵（列优先，每列256点）
        cufftComplex* d_matrix = nullptr;
        size_t matrixBytes = Np * P * sizeof(cufftComplex);
        CHECK_CUDA_ERROR(cudaMalloc((void**)&d_matrix, matrixBytes));
        {
            // 每个 block 负责一列，block 内 256 线程对应矩阵行
            dim3 block(Np);
            dim3 grid(P);
            buildOverlapMatrix<<<grid, block>>>(d_xx, d_matrix, Np, L, P, NN);
            CHECK_CUDA_ERROR(cudaDeviceSynchronize());
        }
        CHECK_CUDA_ERROR(cudaFree(d_xx));

        // --------------------------- (C) 后续处理：窗口 → FFT+fftshift → DC → 转置 → CM+32点 FFT → abs^2 ---------------------------
        // (C1) 窗口处理：将主机窗口数据拷贝到设备后调用核函数
        float h_window[Np] = {
            0.079999924,0.08013916,0.080558777,0.081256866,0.082231522,0.083486557,0.085018158,
            0.086826324,0.088907242,0.091264725,0.093893051,0.096792221,0.099962234,0.10339737,
            0.10709953,0.111063,0.11528778,0.11976814,0.124506,0.12949562,0.13473511,0.14021873,
            0.1459465,0.15191269,0.15811539,0.16454887,0.17121124,0.17809677,0.18520355,
            0.19252396,0.20005608,0.20779419,0.21573448,0.22387123,0.23220062,0.24071503,
            0.24941254,0.25828743,0.26733208,0.27654266,0.28591156,0.29543686,0.30510902,
            0.31492615,0.32487679,0.33496094,0.34516716,0.35549355,0.36593056,0.37647438,
            0.38711739,0.39785194,0.40867424,0.41957474,0.43054962,0.44159126,0.45269203,
            0.4638443,0.47504425,0.48628426,0.49755669,0.50885391,0.52017021,0.53149986,
            0.54283333,0.55416489,0.56548882,0.57679749,0.58808327,0.59934044,0.61055946,
            0.62173843,0.63286591,0.64393806,0.65494537,0.66588593,0.67674828,0.6875267,
            0.69821739,0.70881081,0.71930122,0.72968483,0.73995209,0.75009727,0.76011467,
            0.77000046,0.7797451,0.78934479,0.79879189,0.80808449,0.81721115,0.82617188,
            0.83495903,0.84356499,0.85198784,0.86022186,0.86826134,0.87610054,0.88373566,
            0.89116287,0.89837646,0.90537262,0.91214752,0.91869545,0.9250145,0.93109894,
            0.93694687,0.94255447,0.94791603,0.95302963,0.95789337,0.96250343,0.96685791,
            0.97095108,0.97478485,0.9783535,0.98165512,0.98468971,0.98745537,0.98994827,
            0.99216843,0.99411201,0.99578285,0.99717522,0.99829102,0.99912834,0.99968529,
            0.99996567,0.99996567,0.99968529,0.99912834,0.99829102,0.99717522,0.99578285,
            0.99411201,0.99216843,0.98994827,0.98745537,0.98468971,0.98165512,0.9783535,
            0.97478485,0.97095108,0.96685791,0.96250343,0.95789337,0.95302963,0.94791603,
            0.94255447,0.93694687,0.93109894,0.9250145,0.91869545,0.91214752,0.90537262,
            0.89837646,0.89116287,0.88373566,0.87610054,0.86826134,0.86022186,0.85198784,
            0.84356499,0.83495903,0.82617188,0.81721115,0.80808449,0.79879189,0.78934479,
            0.7797451,0.77000046,0.76011467,0.75009727,0.73995209,0.72968483,0.71930122,
            0.70881081,0.69821739,0.6875267,0.67674828,0.66588593,0.65494537,0.64393806,
            0.63286591,0.62173843,0.61055946,0.59934044,0.58808327,0.57679749,0.56548882,
            0.55416489,0.54283333,0.53149986,0.52017021,0.50885391,0.49755669,0.48628426,
            0.47504425,0.4638443,0.45269203,0.44159126,0.43054962,0.41957474,0.40867424,
            0.39785194,0.38711739,0.37647438,0.36593056,0.35549355,0.34516716,0.33496094,
            0.32487679,0.31492615,0.30510902,0.29543686,0.28591156,0.27654266,0.26733208,
            0.25828743,0.24941254,0.24071503,0.23220062,0.22387123,0.21573448,0.20779419,
            0.20005608,0.19252396,0.18520355,0.17809677,0.17121124,0.16454887,0.15811539,
            0.15191269,0.1459465,0.14021873,0.13473511,0.12949562,0.124506,0.11976814,
            0.11528778,0.111063,0.10709953,0.10339737,0.099962234,0.096792221,0.093893051,
            0.091264725,0.088907242,0.086826324,0.085018158,0.083486557,0.082231522,
            0.081256866,0.080558777,0.08013916,0.079999924
        };
        float* d_window = nullptr;
        CHECK_CUDA_ERROR(cudaMalloc((void**)&d_window, Np * sizeof(float)));
        CHECK_CUDA_ERROR(cudaMemcpy(d_window, h_window, Np * sizeof(float), cudaMemcpyHostToDevice));
        applyWindow(d_matrix, d_window, Np, P);
        CHECK_CUDA_ERROR(cudaFree(d_window));

        // (C2) FFT + fftshift
        performFFT(d_matrix, Np, P);

        // (C3) DC 系数乘法
        applyDCCoef(d_matrix, Np, P);

        // (C4) 转置 => (32×256)
        cufftComplex* d_transposed = nullptr;
        CHECK_CUDA_ERROR(cudaMalloc((void**)&d_transposed, matrixBytes));
        transposeMatrix(d_matrix, d_transposed, Np, P);

        // (C5) CM+32点 FFT
        cufftComplex* d_cmFFTResult = nullptr;
        performCM32ptFFT(d_transposed, &d_cmFFTResult);
        CHECK_CUDA_ERROR(cudaFree(d_transposed));

        // (D) 对 d_cmFFTResult 进行 abs^2 计算，得到 65536×32 个结果
        int totalCM = 256 * 256 * 32;
        float* d_absSquare = nullptr;
        computeAbsSquareAll(d_cmFFTResult, &d_absSquare, totalCM);

        // ============ 拷回部分结果进行检查 ============
        const int checkTransforms = 4;
        const int checkCount = checkTransforms * 32;
        cufftComplex* h_cmFFTResult = (cufftComplex*)malloc(checkCount * sizeof(cufftComplex));
        CHECK_CUDA_ERROR(cudaMemcpy(h_cmFFTResult, d_cmFFTResult,
                                    checkCount * sizeof(cufftComplex),
                                    cudaMemcpyDeviceToHost));

        float* h_absSquare = (float*)malloc(checkCount * sizeof(float));
        CHECK_CUDA_ERROR(cudaMemcpy(h_absSquare, d_absSquare,
                                    checkCount * sizeof(float),
                                    cudaMemcpyDeviceToHost));
        std::cout << "\n前 " << checkTransforms << " 路(共 " << checkCount << " 点) 的 abs^2 数据(前 8 个):\n";
        for (int i = 0; i < 8; i++) {
            std::cout << " " << h_absSquare[i];
        }
        std::cout << "\n";

        // 释放本次迭代分配的所有资源
        free(h_cmFFTResult);
        free(h_absSquare);
        CHECK_CUDA_ERROR(cudaFree(d_absSquare));
        CHECK_CUDA_ERROR(cudaFree(d_matrix));
        CHECK_CUDA_ERROR(cudaFree(d_cmFFTResult));
    }
    
    // 在所有迭代结束后记录结束时间
    CHECK_CUDA_ERROR(cudaEventRecord(stopEvent, 0));
    CHECK_CUDA_ERROR(cudaEventSynchronize(stopEvent));
    CHECK_CUDA_ERROR(cudaEventElapsedTime(&elapsedTime, startEvent, stopEvent));
    std::cout << "[GPU] Total processing time: " << elapsedTime << " ms\n";
    
    // 清理计时用的 CUDA event
    CHECK_CUDA_ERROR(cudaEventDestroy(startEvent));
    CHECK_CUDA_ERROR(cudaEventDestroy(stopEvent));
    
    return 0;
}
