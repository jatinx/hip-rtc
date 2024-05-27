#pragma once

#ifndef CUSTOM_HIP_HIPRTC_H
#define CUSTOM_HIP_HIPRTC_H

#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief hiprtcResult status
 *
 */
typedef enum hiprtc_result_e {
  HIPRTC_SUCCESS = 0,                         ///< Success
  HIPRTC_ERROR_OUT_OF_MEMORY = 1,             ///< Out of memory
  HIPRTC_ERROR_PROGRAM_CREATION_FAILURE = 2,  ///< Failed to create program
  HIPRTC_ERROR_INVALID_INPUT = 3,             ///< Invalid input
  HIPRTC_ERROR_INVALID_PROGRAM = 4,           ///< Invalid program
  HIPRTC_ERROR_INVALID_OPTION = 5,            ///< Invalid option
  HIPRTC_ERROR_COMPILATION = 6,               ///< Compilation error
  HIPRTC_ERROR_BUILTIN_OPERATION_FAILURE = 7, ///< Failed in builtin operation
  HIPRTC_ERROR_NO_NAME_EXPRESSIONS_AFTER_COMPILATION =
      8, ///< No name expression after compilation
  HIPRTC_ERROR_NO_LOWERED_NAMES_BEFORE_COMPILATION =
      9, ///< No lowered names before compilation
  HIPRTC_ERROR_NAME_EXPRESSION_NOT_VALID = 10, ///< Invalid name expression
  HIPRTC_ERROR_INTERNAL_ERROR = 11,            ///< Internal error
} hiprtcResult;

/**
 * @brief Get string of hiprtcResult
 *
 * @param result input hiprtc result
 * @return const char* string of hiprtcResult
 */
const char *hiprtcGetErrorString(hiprtcResult result);

/**
 * @brief Get version of hiprtc
 *
 * @param major Major version
 * @param minor Minor version
 * @return hiprtcResult
 */
hiprtcResult hiprtcVersion(int *major, int *minor);

/**
 * @brief Opaque handle of hiprtc program
 *
 */
typedef void *hiprtcProgram;

/**
 * @brief Create a handle of hiprtc program
 *
 * @param prog output program
 * @param src source code
 * @param name name to be used
 * @param numHeaders number of headers
 * @param headers header pointers
 * @param includeNames
 * @return hiprtcResult
 */
hiprtcResult hiprtcCreateProgram(hiprtcProgram *prog, const char *src,
                                 const char *name, int numHeaders,
                                 const char **headers,
                                 const char **includeNames);

/**
 * @brief Destroy the hiprtcProgram
 *
 * @param prog
 * @return hiprtcResult
 */
hiprtcResult hiprtcDestroyProgram(hiprtcProgram *prog);

/**
 * @brief Compile the hiprtcProgram with options
 *
 * @param prog Input Program
 * @param numOptions Number of options
 * @param options Options
 * @return hiprtcResult
 */
hiprtcResult hiprtcCompileProgram(hiprtcProgram prog, int numOptions,
                                  const char **options);

/**
 * @brief Get program log size
 *
 * @param prog
 * @param logSizeRet
 * @return hiprtcResult
 */
hiprtcResult hiprtcGetProgramLogSize(hiprtcProgram prog, size_t *logSizeRet);

/**
 * @brief Get program log
 *
 * @param prog
 * @param dst
 * @return hiprtcResult
 */
hiprtcResult hiprtcGetProgramLog(hiprtcProgram prog, const char *dst);

/**
 * @brief Get code size
 *
 * @param prog
 * @param binarySizeRet
 * @return hiprtcResult
 */
hiprtcResult hiprtcGetCodeSize(hiprtcProgram prog, size_t *binarySizeRet);

/**
 * @brief Get code to be loaded by hipModuleLoad
 *
 * @param prog
 * @param binaryMem
 * @return hiprtcResult
 */
hiprtcResult hiprtcGetCode(hiprtcProgram prog, char *binaryMem);

#ifdef __cplusplus
}
#endif

#endif
