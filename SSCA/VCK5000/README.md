# Versal-based Implementation of SSCA
By default, this project targets the `xilinx_vck5000_gen4x8_qdma_2_202220_1` platform for VCK5000. 

## Dependencies 
This project requires: AMD/Xilinx Versal VCK5000 (`xilinx_vck5000_gen4x8_qdma_2_202220_1`, Vitis 2022.2-2023.1)

## Guide
### 1. Make bitstream and host for project
```bash
make all
```

### 2. Run the code
```bash
cd host
make run_cpp
```

