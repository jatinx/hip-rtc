#include "common.hpp"
#include "hip/hiprtc.h"

#include <hip/hip_runtime.h>
#include <vector>

#define hip_check(hip_call)                                                    \
  {                                                                            \
    auto hip_res = (hip_call);                                                 \
    if (hip_res != hipSuccess) {                                               \
      std::cerr << "Failed in call: " << #hip_call                             \
                << " with error: " << hipGetErrorString(hip_res) << std::endl; \
      std::abort();                                                            \
    }                                                                          \
  }

int main() {
  std::string source = "#include <header2.h>\n #include <header1.h>\n  extern "
                       "\"C\" __global__ void kernel(int *a) { set1(a); }";
  std::string header1 = "__device__ void set1(int *a) { *a = 5; set2(a); } ";
  std::string header2 = "__device__ void set2(int *a) { *a += 5; } ";
  const char *header_wrapper[] = {header1.c_str(), header2.c_str()};
  const char *header_names[] = {"header1.h", "header2.h"};
  hiprtcProgram prog;
  hiprtc_check(hiprtcCreateProgram(&prog, source.c_str(), "source.cpp", 2,
                                   header_wrapper, header_names));
  hiprtc_check(hiprtcCompileProgram(prog, 0, nullptr));

  size_t code_size = 0;
  hiprtc_check(hiprtcGetCodeSize(prog, &code_size));
  check(code_size != 0);

  std::vector<char> code(code_size, 0);
  hiprtc_check(hiprtcGetCode(prog, code.data()));

  hipModule_t module;
  hipFunction_t kernel;

  hip_check(hipModuleLoadData(&module, code.data()));
  hip_check(hipModuleGetFunction(&kernel, module, "kernel"));

  int *d_ptr;
  hip_check(hipMalloc(&d_ptr, sizeof(int)));

  struct {
    int *ptr;
  } args{d_ptr};
  auto arg_size = sizeof(args);
  void *config[] = {HIP_LAUNCH_PARAM_BUFFER_POINTER, &args,
                    HIP_LAUNCH_PARAM_BUFFER_SIZE, &arg_size,
                    HIP_LAUNCH_PARAM_END};

  hip_check(hipModuleLaunchKernel(kernel, 1, 1, 1, 1, 1, 1, 0, nullptr, nullptr,
                                  config));

  int gpu_res = 0;
  hip_check(hipMemcpy(&gpu_res, d_ptr, sizeof(int), hipMemcpyDeviceToHost));
  std::cout << "Res should be 10 and is: " << gpu_res << std::endl;
  check(gpu_res == 10);

  hip_check(hipFree(d_ptr));
  hip_check(hipModuleUnload(module));

  hiprtc_check(hiprtcDestroyProgram(&prog));
}
