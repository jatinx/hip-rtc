#include <amd_comgr/amd_comgr.h>
#include <string>

#include "comgr_wrapper.hpp"
#include "hiprtc_internal.hpp"
#include "rocm_smi.hpp"

const char hiprtc_internal_header[] = {
#include "hiprtc_internal_header_generated.hpp"
};

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

bool get_mangled_names(
    const std::vector<char> &exe,
    std::unordered_map<std::string, std::string> &mangled_names) {
  amd_comgr_data_t data;
  if (auto comgr_res =
          amd_comgr_create_data(AMD_COMGR_DATA_KIND_EXECUTABLE, &data);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    return false;
  }

  if (auto comgr_res = amd_comgr_set_data(data, exe.size(), exe.data());
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_release_data(data);
    return false;
  }

  size_t name_count;
  if (auto comgr_res =
          amd_comgr_populate_name_expression_map(data, &name_count);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_release_data(data);
    return false;
  }

  for (auto &name_pair : mangled_names) {
    size_t name_size = 0;
    char *name_ptr = const_cast<char *>(name_pair.first.data());

    if (auto comgr_res = amd_comgr_map_name_expression_to_symbol_name(
            data, &name_size, name_ptr, NULL);
        comgr_res != AMD_COMGR_STATUS_SUCCESS) {
      (void)amd_comgr_release_data(data);
      return false;
    }

    std::string name(name_size, 0);
    if (auto comgr_res = amd_comgr_map_name_expression_to_symbol_name(
            data, &name_size, name_ptr, const_cast<char *>(name.data()));
        comgr_res != AMD_COMGR_STATUS_SUCCESS) {
      (void)amd_comgr_release_data(data);
      return false;
    }
    name_pair.second = name;
  }

  // Release data
  (void)amd_comgr_release_data(data);
  return true;
}

// Big func, might refactor later
bool compile_program(hiprtc_program *prog,
                     const std::vector<std::string> &options) {
  // clear the existing log
  prog->log_.clear();

  // Create comgr dataset, a superset of all compilation inputs
  amd_comgr_data_set_t data_set;
  if (auto comgr_res = amd_comgr_create_data_set(&data_set);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    return false;
  }

  // Create data for source
  amd_comgr_data_t data;
  if (!create_data(data, AMD_COMGR_DATA_KIND_SOURCE, prog->source_.data(),
                   prog->source_.size(), prog->name_.c_str())) {
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

  // release data here
  (void)amd_comgr_release_data(data);

  // Add internal header
  amd_comgr_data_t include_data;
  if (!create_data(include_data, AMD_COMGR_DATA_KIND_INCLUDE,
                   hiprtc_internal_header, sizeof(hiprtc_internal_header) - 1,
                   "hiprtc_internal_header.h")) {
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

  // Add external headers provided by user
  for (size_t i = 0; i < prog->headers_.size(); i++) {
    amd_comgr_data_t user_header;
    if (!create_data(user_header, AMD_COMGR_DATA_KIND_INCLUDE,
                     prog->headers_[i].second.c_str(),
                     prog->headers_[i].second.size(),
                     prog->headers_[i].first.c_str())) {
      (void)amd_comgr_destroy_data_set(data_set);
      return false;
    }

    // Add include header to dataset
    if (auto comgr_res = amd_comgr_data_set_add(data_set, user_header);
        comgr_res != AMD_COMGR_STATUS_SUCCESS) {
      (void)amd_comgr_destroy_data_set(data_set);
      (void)amd_comgr_release_data(user_header);
      return false;
    }

    // Release include header
    (void)amd_comgr_release_data(user_header);
  }

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
    prog->log_ += "Error in compilation to relocatable:";
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
    prog->log_ += "Error in compilation to exe:";
    prog->log_ += get_build_log(exe);
    (void)amd_comgr_destroy_data_set(data_set);
    (void)amd_comgr_destroy_data_set(reloc);
    (void)amd_comgr_destroy_data_set(exe);
    (void)amd_comgr_destroy_action_info(action);
    return false;
  }

  // Destroy the action
  (void)amd_comgr_destroy_action_info(action);

  prog->log_ += get_build_log(exe);

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

  // Fill up mangled names
  if (prog->lowered_names_.size() > 0) {
    if (!get_mangled_names(prog->object_, prog->lowered_names_)) {
      (void)amd_comgr_destroy_data_set(data_set);
      (void)amd_comgr_destroy_data_set(reloc);
      (void)amd_comgr_destroy_data_set(exe);
      (void)amd_comgr_release_data(binary);
      return false;
    }
  }

  (void)amd_comgr_release_data(binary);
  (void)amd_comgr_destroy_data_set(data_set);
  (void)amd_comgr_destroy_data_set(reloc);
  (void)amd_comgr_destroy_data_set(exe);
  return true;
}
