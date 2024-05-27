#include "common.hpp"

int main() {
  int major = 0, minor = 0;
  hiprtc_check(hiprtcVersion(&major, &minor));
  check(major != 0 || minor != 0);
}