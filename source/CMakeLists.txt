add_library(amd_comgr SHARED IMPORTED)
set_target_properties(amd_comgr PROPERTIES
  IMPORTED_LOCATION "${ROCM_PATH}/lib/libamd_comgr.so"
  INTERFACE_INCLUDE_DIRECTORIES "${ROCM_PATH}/include"
)

add_library(rocm_smi64 SHARED IMPORTED)
set_target_properties(rocm_smi64 PROPERTIES
  IMPORTED_LOCATION "${ROCM_PATH}/lib/librocm_smi64.so"
  INTERFACE_INCLUDE_DIRECTORIES "${ROCM_PATH}/include"
)

add_executable(gen_hiprtc_header 
  generate_hiprtc_header.cpp
  comgr_wrapper.cpp
  rocm_smi.cpp)

target_link_libraries(gen_hiprtc_header amd_comgr rocm_smi64)

add_custom_command(TARGET gen_hiprtc_header
  POST_BUILD
  COMMAND gen_hiprtc_header ${PROJECT_SOURCE_DIR}/include)

add_library(hip_rtc SHARED
  hiprtc.cpp
  comgr_wrapper.cpp
  hiprtc_internal.cpp
  rocm_smi.cpp)

target_link_libraries(hip_rtc amd_comgr rocm_smi64)
add_dependencies(hip_rtc gen_hiprtc_header)