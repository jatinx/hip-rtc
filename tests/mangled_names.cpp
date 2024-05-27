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
  std::string source =
      "template<typename T> __global__ void kernel(T *a) { *a = 10; }";
  hiprtcProgram prog;
  hiprtc_check(
      hiprtcCreateProgram(&prog, source.c_str(), nullptr, 0, nullptr, nullptr));
  const char *kernel_name = "kernel<int>";
  hiprtc_check(hiprtcAddNameExpression(prog, kernel_name));
  hiprtc_check(hiprtcCompileProgram(prog, 0, nullptr));

  const char *lowered_name;
  hiprtc_check(hiprtcGetLoweredName(prog, kernel_name, &lowered_name));

  size_t code_size = 0;
  hiprtc_check(hiprtcGetCodeSize(prog, &code_size));
  check(code_size != 0);

  std::vector<char> code(code_size, 0);
  hiprtc_check(hiprtcGetCode(prog, code.data()));

  hipModule_t module;
  hipFunction_t kernel;

  hip_check(hipModuleLoadData(&module, code.data()));
  hip_check(hipModuleGetFunction(&kernel, module, lowered_name));

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
