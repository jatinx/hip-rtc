R"(
#define __device__ __attribute__((device))
#define __host__ __attribute__((host))
#define __global__ __attribute__((global))
#define __constant__ __attribute__((constant))
#define __shared__ __attribute__((shared))

#define launch_bounds_impl0(requiredMaxThreadsPerBlock)                        \
  __attribute__((amdgpu_flat_work_group_size(1, requiredMaxThreadsPerBlock)))
#define launch_bounds_impl1(requiredMaxThreadsPerBlock,                        \
                            minBlocksPerMultiprocessor)                        \
  __attribute__((amdgpu_flat_work_group_size(1, requiredMaxThreadsPerBlock),   \
                 amdgpu_waves_per_eu(minBlocksPerMultiprocessor)))
#define select_impl_(_1, _2, impl_, ...) impl_
#define __launch_bounds__(...)                                                 \
  select_impl_(__VA_ARGS__, launch_bounds_impl1,                               \
               launch_bounds_impl0)(__VA_ARGS__)
constexpr int warpSize = 32;
)"