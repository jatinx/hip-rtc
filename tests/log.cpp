#include "common.hpp"
#include "hip/hiprtc.h"

int main() {
  std::string source = "obviously invalid c++ program";
  hiprtcProgram prog;
  hiprtc_check(
      hiprtcCreateProgram(&prog, source.c_str(), nullptr, 0, nullptr, nullptr));
  check(hiprtcCompileProgram(prog, 0, nullptr) != HIPRTC_SUCCESS);

  size_t log_size = 0;
  hiprtc_check(hiprtcGetProgramLogSize(prog, &log_size));
  // check(log_size != 0);

  std::string log(log_size, 0);
  hiprtc_check(hiprtcGetProgramLog(prog, log.data()));

  hiprtc_check(hiprtcDestroyProgram(&prog));
}
