# HIP RTC - hip runtime compiler

hip-rtc is a light weight, non-hip dependent library which allows users to compiler hip-programs in code.

This is made only for AMD GPUs. Will not work with any other GPUs.

Warning - This is for personal experimentation, for official hiprtc please refer to ROCm/clr repo. ROCm ships hiprtc along side it's installation.

## How to build

Pre req: clang, comgr, rocm-smi

To run tests: amdhip64 is required as well to test running compiled kernels
