#pragma once

#include <string>
#include <unordered_map>
#include <vector>

typedef enum hiprtc_program_state_e {
  Created = 0,
  Compiled = 1,
  Destroyed = 2,
  Error = 3,
} hiprtc_program_state;

struct hiprtc_program {
  hiprtc_program_state state_; // Current state of hiprtc program
  std::string name_;           // Name
  std::string source_;         // Input source
  std::vector<char> object_;   // Output code object
  std::string log_;            // Log
  std::unordered_map<std::string,
                     std::string> lowered_names_; // Lowered names
};

bool compile_program(hiprtc_program *prog,
                     const std::vector<std::string> &options);
