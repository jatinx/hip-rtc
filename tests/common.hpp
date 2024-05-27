#pragma once

#include <hip/hiprtc.h>
#include <iostream>

#define hiprtc_check(hiprtc_call)                                              \
  {                                                                            \
    auto hiprtc_res = (hiprtc_call);                                           \
    if (hiprtc_res != HIPRTC_SUCCESS) {                                        \
      std::cerr << "Failed in call: " << #hiprtc_call                          \
                << " with error: " << hiprtcGetErrorString(hiprtc_res)         \
                << std::endl;                                                  \
      std::abort();                                                            \
    }                                                                          \
  }

#define check(condition)                                                       \
  {                                                                            \
    if (!(condition)) {                                                        \
      std::cout << "Failed check: " << #condition << std::endl;                \
      std::abort();                                                            \
    }                                                                          \
  }
