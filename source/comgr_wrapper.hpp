#pragma once

#include <amd_comgr/amd_comgr.h>

#include <string>
#include <vector>

bool create_data(amd_comgr_data_t &data, amd_comgr_data_kind_t kind,
                 const char *src, size_t src_len, const char *name);

bool create_action(amd_comgr_action_info_t &action, const std::string &isa_name,
                   const std::vector<std::string> &options);
