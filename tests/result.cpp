#include "common.hpp"
#include <string>

int main() {
  check(std::string(hiprtcGetErrorString(HIPRTC_ERROR_INVALID_INPUT)) ==
        std::string("HIPRTC_ERROR_INVALID_INPUT"));
}