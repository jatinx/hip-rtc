#include "hiprtc_internal.hpp"
#include <hip/hiprtc.h>

#include <cstring>
#include <string>
#include <vector>

const char *hiprtcGetErrorString(hiprtcResult result) {
  switch (result) {
  case HIPRTC_SUCCESS:
    return "HIPRTC_SUCCESS";
  case HIPRTC_ERROR_OUT_OF_MEMORY:
    return "HIPRTC_ERROR_OUT_OF_MEMORY";
  case HIPRTC_ERROR_PROGRAM_CREATION_FAILURE:
    return "HIPRTC_ERROR_PROGRAM_CREATION_FAILURE";
  case HIPRTC_ERROR_INVALID_INPUT:
    return "HIPRTC_ERROR_INVALID_INPUT";
  case HIPRTC_ERROR_INVALID_PROGRAM:
    return "HIPRTC_ERROR_INVALID_PROGRAM";
  case HIPRTC_ERROR_INVALID_OPTION:
    return "HIPRTC_ERROR_INVALID_OPTION";
  case HIPRTC_ERROR_COMPILATION:
    return "HIPRTC_ERROR_COMPILATION";
  case HIPRTC_ERROR_BUILTIN_OPERATION_FAILURE:
    return "HIPRTC_ERROR_BUILTIN_OPERATION_FAILURE";
  case HIPRTC_ERROR_NO_NAME_EXPRESSIONS_AFTER_COMPILATION:
    return "HIPRTC_ERROR_NO_NAME_EXPRESSIONS_AFTER_COMPILATION";
  case HIPRTC_ERROR_NO_LOWERED_NAMES_BEFORE_COMPILATION:
    return "HIPRTC_ERROR_NO_LOWERED_NAMES_BEFORE_COMPILATION";
  case HIPRTC_ERROR_NAME_EXPRESSION_NOT_VALID:
    return "HIPRTC_ERROR_NAME_EXPRESSION_NOT_VALID";
  default:
    return "HIPRTC_ERROR_INTERNAL_ERROR";
  }
}

hiprtcResult hiprtcVersion(int *major, int *minor) {
  if (major == nullptr || minor == nullptr) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

#if defined(HIPRTC_MAJOR_VERSION) and defined(HIPRTC_MINOR_VERSION)
  *major = HIPRTC_MAJOR_VERSION;
  *minor = HIPRTC_MINOR_VERSION;
#else
  *major = *minor = 0;
#endif

  return HIPRTC_SUCCESS;
}

hiprtcResult hiprtcCreateProgram(hiprtcProgram *prog, const char *src,
                                 const char *name, int num_headers,
                                 const char **headers,
                                 const char **include_names) {
  if (prog == nullptr || src == nullptr) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  auto p = new hiprtc_program;
  p->name_ = (name != nullptr) ? name : "CompileSource";
  p->source_ = src;
  p->state_ = hiprtc_program_state::Created;

  // add headers
  for (int i = 0; i < num_headers; i++) {
    if (headers[i] == nullptr || include_names[i] == nullptr) {
      return HIPRTC_ERROR_INVALID_INPUT;
    }
    p->headers_.push_back(
        std::make_pair(std::string(include_names[i]), std::string(headers[i])));
  }

  *prog = reinterpret_cast<hiprtcProgram>(p);

  return HIPRTC_SUCCESS;
}

hiprtcResult hiprtcDestroyProgram(hiprtcProgram *prog) {
  if (prog == nullptr) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  delete reinterpret_cast<hiprtc_program *>(*prog);

  return HIPRTC_SUCCESS;
}

hiprtcResult hiprtcCompileProgram(hiprtcProgram prog, int num_options,
                                  const char **options) {
  auto p = reinterpret_cast<hiprtc_program *>(prog);

  if ((num_options == 0 && options != nullptr) ||
      (num_options != 0 && options == nullptr) || (p == nullptr)) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  if (p->state_ != hiprtc_program_state::Created) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  /* Append user options */
  std::vector<std::string> opts;
  opts.reserve(num_options + 7);
  opts.push_back("-O3");
  opts.push_back("-std=c++17");
  opts.push_back("-nogpuinc");
  opts.push_back("-include");
  opts.push_back("hiprtc_internal_header.h");
  opts.push_back("-Wno-gnu-line-marker");
  opts.push_back("-Wno-missing-prototypes");

  if (num_options != 0) {
    for (int i = 0; i < num_options; i++) {
      opts.push_back(options[i]);
    }
  }

  if (!compile_program(p, opts)) {
    p->state_ = hiprtc_program_state::Error;
    return HIPRTC_ERROR_COMPILATION;
  }
  p->state_ = hiprtc_program_state::Compiled;

  return HIPRTC_SUCCESS;
}

hiprtcResult hiprtcGetProgramLogSize(hiprtcProgram prog, size_t *log_size) {
  if (log_size == nullptr) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  auto p = reinterpret_cast<hiprtc_program *>(prog);
  *log_size = p->log_.size();

  return HIPRTC_SUCCESS;
}

hiprtcResult hiprtcGetProgramLog(hiprtcProgram prog, const char *dst) {
  if (dst == nullptr) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  auto p = reinterpret_cast<hiprtc_program *>(prog);
  std::memcpy((void *)dst, p->log_.data(), p->log_.size());

  return HIPRTC_SUCCESS;
}

hiprtcResult hiprtcGetCodeSize(hiprtcProgram prog, size_t *binary_size) {
  if (binary_size == nullptr) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  auto p = reinterpret_cast<hiprtc_program *>(prog);
  if (p->state_ != hiprtc_program_state::Compiled) {
    return HIPRTC_ERROR_COMPILATION;
  }

  *binary_size = p->object_.size();

  return HIPRTC_SUCCESS;
}

hiprtcResult hiprtcGetCode(hiprtcProgram prog, char *binary) {
  if (binary == nullptr) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  auto p = reinterpret_cast<hiprtc_program *>(prog);
  if (p->state_ != hiprtc_program_state::Compiled) {
    return HIPRTC_ERROR_COMPILATION;
  }

  std::memcpy((void *)binary, p->object_.data(), p->object_.size());

  return HIPRTC_SUCCESS;
}

hiprtcResult hiprtcAddNameExpression(hiprtcProgram prog,
                                     const char *name_expression) {
  if (name_expression == nullptr) {
    return HIPRTC_ERROR_NAME_EXPRESSION_NOT_VALID;
  }

  auto p = reinterpret_cast<hiprtc_program *>(prog);
  if (p->state_ != hiprtc_program_state::Created) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  std::string name = name_expression;
  p->lowered_names_[std::string(name_expression)] = std::string();

  // Now add code that instantiates the template
  std::string gcn_expr = "__amdgcn_name_expr_";
  std::string size = std::to_string(p->lowered_names_.size());
  const auto var1{"\n static __device__ const void* " + gcn_expr + size +
                  "[]= {\"" + name + "\", (void*)&" + name + "};"};
  const auto var2{"\n static auto __amdgcn_name_expr_stub_" + size + " = " +
                  gcn_expr + size + ";\n"};
  const auto code{var1 + var2};

  p->source_ += code;

  return HIPRTC_SUCCESS;
}

hiprtcResult hiprtcGetLoweredName(hiprtcProgram prog,
                                  const char *name_expression,
                                  const char **lowered_name) {
  if (name_expression == nullptr || lowered_name == nullptr) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  auto p = reinterpret_cast<hiprtc_program *>(prog);
  if (p->state_ != hiprtc_program_state::Compiled) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  std::string name(name_expression);
  if (p->lowered_names_.find(name) == p->lowered_names_.end()) {
    return HIPRTC_ERROR_INVALID_INPUT;
  }

  *lowered_name = p->lowered_names_[name].data();

  return HIPRTC_SUCCESS;
}