Please run
```
nvcc -ccbin g++ -std=c++17 -arch=sm_86 -O3 -o bin/fam fam.cu -lcufft -lcudart -lcurand
```
to compile the code
and
```
./bin/fam
```
to run the CUDA code.