#include "comgr_wrapper.hpp"
#include <algorithm>

/**
 * @brief Create a data object which holds source for comgr
 *
 * @param data reference of data to be created
 * @param kind Kind of data
 * @param src Source to be input
 * @param src_len Length of src
 * @param name Name
 * @return true Success
 * @return false Failed
 */
bool create_data(amd_comgr_data_t &data, amd_comgr_data_kind_t kind,
                 const char *src, size_t src_len, const char *name) {
  if (auto comgr_res = amd_comgr_create_data(kind, &data);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    return false;
  }

  // Add source
  if (auto comgr_res = amd_comgr_set_data(data, src_len, src);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_release_data(data);
    return false;
  }

  // Set name
  if (auto comgr_res = amd_comgr_set_data_name(data, name);
      comgr_res != AMD_COMGR_STATUS_SUCCESS) {
    (void)amd_comgr_release_data(data);
    return false;
  }
  return true;
}

/**
 * @brief Create a action object for comgr
 *
 * @param action reference to action object to be populated
 * @param isa_name isa name in amdgcn-amd-amdhsa--gfxnnn:features
 * @param options compiler options to be passed
 * @return true
 * @return false
 */
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