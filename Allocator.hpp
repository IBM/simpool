#ifndef _ALLOCATOR_HPP
#define _ALLOCATOR_HPP

#include <cstdlib>

#ifdef HAS_CUDA

#include "cuda_runtime.h"

struct UVMAllocator
{
  static inline void *allocate(std::size_t size) { void *ptr; cudaMallocManaged(&ptr, size); return ptr; }
  static inline void deallocate(void *ptr) { cudaFree(ptr); }
};

#endif

struct CPUAllocator
{
  static inline void *allocate(std::size_t size) { return std::malloc(size); }
  static inline void deallocate(void *ptr) { std::free(ptr); }
};

#endif // _ALLOCATOR_HPP
