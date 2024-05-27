#include "common.hpp"
#include "hip/hiprtc.h"

int main() {
  std::string source =
      "extern \"C\" __attribute__((global)) void kernel(int *a) { *a = 10; }";
  hiprtcProgram prog;
  hiprtc_check(
      hiprtcCreateProgram(&prog, source.c_str(), nullptr, 0, nullptr, nullptr));
  hiprtc_check(hiprtcCompileProgram(prog, 0, nullptr));
  hiprtc_check(hiprtcDestroyProgram(&prog));
}
