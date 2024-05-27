add_library(hip_rtc SHARED
  hiprtc.cpp
  hiprtc_internal.cpp
  rocm_smi.cpp)

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

target_link_libraries(hip_rtc amd_comgr rocm_smi64)