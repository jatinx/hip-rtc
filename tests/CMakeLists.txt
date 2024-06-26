add_executable(version version.cpp)
target_link_libraries(version PUBLIC hip_rtc)

add_executable(result result.cpp)
target_link_libraries(result PUBLIC hip_rtc)

add_executable(prog prog.cpp)
target_link_libraries(prog PUBLIC hip_rtc)

add_executable(log log.cpp)
target_link_libraries(log PUBLIC hip_rtc)

add_library(amdhip64 SHARED IMPORTED)
set_target_properties(amdhip64 PROPERTIES
  IMPORTED_LOCATION "${ROCM_PATH}/lib/libamdhip64.so"
  INTERFACE_INCLUDE_DIRECTORIES "${ROCM_PATH}/include"
)

add_executable(load_code load_code.cpp)
target_link_libraries(load_code PUBLIC hip_rtc amdhip64)
target_compile_definitions(load_code PUBLIC __HIP_PLATFORM_AMD__)

add_executable(mangled_names mangled_names.cpp)
target_link_libraries(mangled_names PUBLIC hip_rtc amdhip64)
target_compile_definitions(mangled_names PUBLIC __HIP_PLATFORM_AMD__)

add_executable(include_header include_header.cpp)
target_link_libraries(include_header PUBLIC hip_rtc amdhip64)
target_compile_definitions(include_header PUBLIC __HIP_PLATFORM_AMD__)

add_test(NAME version COMMAND version)
add_test(NAME result COMMAND result)
add_test(NAME prog COMMAND prog)
add_test(NAME log COMMAND log)
add_test(NAME load_code COMMAND load_code)
add_test(NAME mangled_names COMMAND mangled_names)
add_test(NAME include_header COMMAND include_header)
