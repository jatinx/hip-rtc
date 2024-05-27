#include <algorithm>
#include <amd_comgr/amd_comgr.h>
#include <string>

#include "hiprtc_internal.hpp"
#include "rocm_smi.hpp"

bool create_action(amd_comgr_action_info_t &action, const std::string &isa_name,
                   const std::vector<std::string> &options) {
  if (auto comgr_res = amd_comgr_create_action_info(&action);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    return false;
  }

  if (auto comgr_res =
          amd_comgr_action_info_set_language(action, AMD_COMGR_LANGUAGE_HIP);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_action_info(action);
    return false;
  }

  if (auto comgr_res =
          amd_comgr_action_info_set_isa_name(action, isa_name.c_str());
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_action_info(action);
    return false;
  }

  std::vector<const char *> opts_ptr;
  opts_ptr.reserve(options.size());
  std::for_each(options.begin(), options.end(), [&](const std::string &opt) {
    opts_ptr.push_back(opt.c_str());
  });

  if (auto comgr_res = amd_comgr_action_info_set_option_list(
          action, opts_ptr.data(), opts_ptr.size());
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_action_info(action);
    return false;
  }

  return true;
}

std::string get_build_log(amd_comgr_data_set_t &data_set) {
  size_t count = 0;
  if (auto comgr_res = amd_comgr_action_data_count(
          data_set, AMD_COMGR_DATA_KIND_LOG, &count);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    return "";
  }

  if (count > 0) {
    amd_comgr_data_t binary_data;

    if (auto res = amd_comgr_action_data_get_data(
            data_set, AMD_COMGR_DATA_KIND_LOG, 0, &binary_data);
        res != AMD_COMGR_STATUS_SUCCESS) {
      return "";
    }

    size_t binary_size = 0;
    if (auto res = amd_comgr_get_data(binary_data, &binary_size, NULL);
        res != AMD_COMGR_STATUS_SUCCESS) {
      (void)amd_comgr_release_data(binary_data);
      return "";
    }

    std::string log(binary_size, 0);

    if (auto res = amd_comgr_get_data(binary_data, &binary_size, log.data());
        res != AMD_COMGR_STATUS_SUCCESS) {
      (void)amd_comgr_release_data(binary_data);
      return "";
    }

    (void)amd_comgr_release_data(binary_data);
    return log;
  } else {
    return "";
  }
}

// Big func, might refactor later
bool compile_program(hiprtc_program *prog,
                     const std::vector<std::string> &options) {
  // Create comgr dataset
  amd_comgr_data_set_t data_set;
  if (auto comgr_res = amd_comgr_create_data_set(&data_set);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    return false;
  }

  // Create data
  amd_comgr_data_t data;
  if (auto comgr_res = amd_comgr_create_data(AMD_COMGR_DATA_KIND_SOURCE, &data);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    return false;
  }

  // Add source
  if (auto comgr_res =
          amd_comgr_set_data(data, prog->source_.size(), prog->source_.data());
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_release_data(data);
    return false;
  }

  // Set name
  if (auto comgr_res = amd_comgr_set_data_name(data, prog->name_.c_str());
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_release_data(data);
    return false;
  }

  // Add data to dataset
  if (auto comgr_res = amd_comgr_data_set_add(data_set, data);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_release_data(data);
    return false;
  }

  // TODO: maybe release data here
  (void)amd_comgr_release_data(data);

  // Get isa name
  auto isa_name = rsmi::get_isa_name();

  // Create action
  amd_comgr_action_info_t action;
  if (!create_action(action, isa_name, options)) {
    (void)amd_comgr_destroy_data_set(data_set);
    return false;
  }

  // Create relocatable
  amd_comgr_data_set_t reloc;
  if (auto comgr_res = amd_comgr_create_data_set(&reloc);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_action_info(action);
    return false;
  }

  // Compile to relocatable
  if (auto comgr_res =
          amd_comgr_do_action(AMD_COMGR_ACTION_COMPILE_SOURCE_TO_RELOCATABLE,
                              action, data_set, reloc);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    prog->log_ += get_build_log(reloc);
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_action_info(action);
    (void)amd_comgr_destroy_data_set(reloc);
    return false;
  }

  prog->log_ += get_build_log(reloc);

  // Destroy the action
  (void)amd_comgr_destroy_action_info(action);

  // Create executable
  amd_comgr_data_set_t exe;
  if (auto comgr_res = amd_comgr_create_data_set(&exe);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_data_set(reloc);
    return false;
  }

  // Create action again for reloc to exe
  if (!create_action(action, isa_name, options)) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_data_set(reloc);
    (void)amd_comgr_destroy_data_set(exe);
    return false;
  }

  // Compile to link
  if (auto comgr_res = amd_comgr_do_action(
          AMD_COMGR_ACTION_LINK_RELOCATABLE_TO_EXECUTABLE, action, reloc, exe);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    prog->log_ += get_build_log(exe);
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_data_set(reloc);
    (void)amd_comgr_destroy_data_set(exe);
    (void)amd_comgr_destroy_action_info(action);
    return false;
  }

  // Destroy the action
  (void)amd_comgr_destroy_action_info(action);

  // Extract Binary
  amd_comgr_data_t binary;
  if (auto comgr_res = amd_comgr_action_data_get_data(
          exe, AMD_COMGR_DATA_KIND_EXECUTABLE, 0, &binary);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_data_set(reloc);
    (void)amd_comgr_destroy_data_set(exe);
    return false;
  }

  // Get binary size
  size_t binary_size = 0;
  if (auto comgr_res = amd_comgr_get_data(binary, &binary_size, NULL);
      comgr_res != AMD_COMGR_STATUS_SUCCESS || binary_size == 0) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_data_set(reloc);
    (void)amd_comgr_destroy_data_set(exe);
    (void)amd_comgr_release_data(binary);
    return false;
  }

  // Allocate binary size to copy
  prog->object_.resize(binary_size);

  // Copy binary
  if (auto comgr_res =
          amd_comgr_get_data(binary, &binary_size, prog->object_.data());
      comgr_res != AMD_COMGR_STATUS_SUCCESS || binary_size == 0) {
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_data_set(reloc);
    (void)amd_comgr_destroy_data_set(exe);
    (void)amd_comgr_release_data(binary);
    return false;
  }

  (void)amd_comgr_release_data(binary);
  (void)amd_comgr_destroy_data_set(data_set);
  (void)amd_comgr_destroy_data_set(reloc);
  (void)amd_comgr_destroy_data_set(exe);
  return true;
}
