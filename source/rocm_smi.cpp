#include "rocm_smi.hpp"

#include <rocm_smi/rocm_smi.h>

namespace rsmi {
unsigned int get_device_count() { return 0; }

std::string get_isa_name(unsigned int device) {
  std::string isa = "amdgcn-amd-amdhsa--gfx";

  // Fetch number from rocm_smi
  // This is stupid, will change this in future
  if (auto ret = rsmi_init(0); ret != RSMI_STATUS_SUCCESS) {
    return "";
  }

  uint64_t name = 0;
  if (auto ret = rsmi_dev_target_graphics_version_get(device, &name);
      ret != RSMI_STATUS_SUCCESS) {
    return "";
  }

  isa += std::to_string(name);

  (void)rsmi_shut_down();
  return isa;
}
} // namespace rsmi
