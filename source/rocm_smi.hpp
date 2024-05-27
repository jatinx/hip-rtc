#pragma once

#include <string>

namespace rsmi {
/**
 * @brief Get the device count object
 *
 * @return unsigned int
 */
unsigned int get_device_count();

/**
 * @brief Get the isa name
 *
 * @param device
 * @return std::string
 */
std::string get_isa_name(unsigned int device = 0);
} // namespace rsmi
