#include "comgr_wrapper.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

const char hiprtc_internal_header[] = {
#include "hiprtc_header_defs.h"
};

const char hiprtc_header_append[] = {
#include "hiprtc_defines.h"
};

bool get_internal_header(std::string &header) {
  std::vector<std::string> options;
  options.reserve(8);
  options.push_back("-D__HIPCC_RTC__");
  options.push_back("-std=c++17");
  options.push_back("-include");
  options.push_back("hiprtc_internal_header.hpp");
  options.push_back("--cuda-device-only");
  options.push_back("-nogpulib");
  options.push_back("--cuda-gpu-arch=gfx1100");
  options.push_back("-P");

  // Create comgr dataset, a superset of all compilation inputs
  amd_comgr_data_set_t data_set;
  if (auto comgr_res = amd_comgr_create_data_set(&data_set);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    return false;
  }

  const char custom_src[] = "#include <hiprtc_internal_header.hpp>";
  // Create data for source
  amd_comgr_data_t data;
  if (!create_data(data, AMD_COMGR_DATA_KIND_SOURCE, custom_src,
                   sizeof(custom_src), "custom.cc")) {
    (void)amd_comgr_destroy_data_set(data_set);
    return false;
  }

  // Add data to dataset
  if (auto comgr_res = amd_comgr_data_set_add(data_set, data);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_release_data(data);
    return false;
  }

  // Add internal header
  amd_comgr_data_t include_data;
  if (!create_data(
          include_data, AMD_COMGR_DATA_KIND_INCLUDE, hiprtc_internal_header,
          sizeof(hiprtc_internal_header) - 1 /* -1 coz null terminated */,
          "hiprtc_internal_header.hpp")) {
    (void)amd_comgr_destroy_data_set(data_set);
    return false;
  }

  // Add include header to dataset
  if (auto comgr_res = amd_comgr_data_set_add(data_set, include_data);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_release_data(include_data);
    return false;
  }

  // Release include header
  (void)amd_comgr_release_data(include_data);

  // Todo, configure it at runtime
  // Get isa name
  // auto isa_name = "amdgcn-amd-amdhsa--gfx1100";

  // Create action
  amd_comgr_action_info_t action;
  if (!create_action(action, /*isa_name*/ "", options)) {
    (void)amd_comgr_destroy_data_set(data_set);
    return false;
  }

  // Create relocatable
  amd_comgr_data_set_t preprocess;
  if (auto comgr_res = amd_comgr_create_data_set(&preprocess);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_action_info(action);
    return false;
  }

  // Compile to preprocess
  if (auto comgr_res =
          amd_comgr_do_action(AMD_COMGR_ACTION_SOURCE_TO_PREPROCESSOR, action,
                              data_set, preprocess);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_action_info(action);
    (void)amd_comgr_destroy_data_set(preprocess);
    return false;
  }

  // Extract Binary
  amd_comgr_data_t p_header;
  if (auto comgr_res = amd_comgr_action_data_get_data(
          preprocess, AMD_COMGR_DATA_KIND_SOURCE, 0, &p_header);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_action_info(action);
    (void)amd_comgr_destroy_data_set(preprocess);
    return false;
  }

  // Get binary size
  size_t preprocess_size = 0;
  if (auto comgr_res = amd_comgr_get_data(p_header, &preprocess_size, NULL);
      comgr_res != AMD_COMGR_STATUS_SUCCESS || preprocess_size == 0) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_action_info(action);
    (void)amd_comgr_destroy_data_set(preprocess);
    return false;
  }

  // Allocate binary size to copy
  header.resize(preprocess_size);

  // Copy binary
  if (auto comgr_res =
          amd_comgr_get_data(p_header, &preprocess_size, header.data());
      comgr_res != AMD_COMGR_STATUS_SUCCESS || preprocess_size == 0) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_action_info(action);
    (void)amd_comgr_destroy_data_set(preprocess);
    return false;
  }

  return true;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    throw std::runtime_error("Pass location to write the header");
  }

  std::string header;
  if (!get_internal_header(header)) {
    throw std::runtime_error("Failed to generate header");
  }

  header = "R\"(\n" + std::string(hiprtc_header_append) + header;
  header += ")\"";
  std::string location = argv[1];
  std::string header_name = "hiprtc_internal_header_generated.hpp";
  std::string full_name = location + "/" + header_name;
  if (std::filesystem::exists(full_name)) {
    std::filesystem::remove(full_name);
  }

  std::ofstream f(full_name);
  f.write(header.c_str(), header.size());
  f.close();
  return 0;
}