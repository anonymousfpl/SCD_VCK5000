Please make sure install fftw lib, and then compile autossca.cpp
```
g++ -Wall -Wconversion -O2 -pg -I/home/usr/fftwlib/include -c -o autossca.o autossca.cpp \
```
Compile the SSCA_runtime.cpp
```
g++ -fdiagnostics-color=always -Wall -Wconversion -O2 -pg -g \
    -I/home/usr/fftwlib/include \
    -L/home/usr/fftwlib/lib \
    SSCA_runtime.cpp \
    utils.o timer.o fftw.o autossca.o \
    -lfftw3 -lfftw3f -lm -lrt \
    -o SSCA_runtime

```

Run the code

```
./SSCA_runtime
```