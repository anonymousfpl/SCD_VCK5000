# SSCA_CUDA
Compile
```
nvcc -ccbin g++ -std=c++17 -arch sm_86 -O3 -dc -m64 -o bin/ssca_callback2.o -c ssca_callback2.cu
nvcc -ccbin g++ -arch sm_86 -o bin/ssca_callback2 bin/ssca_callback2.o -lcufft_static -lculibos

```

Run
```
./bin/ssca_callback2
```