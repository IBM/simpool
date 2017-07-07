#include <new>
#include <cstddef>
#include "Allocator.hpp"
#include "DynamicPoolAllocator.hpp"

namespace {
#ifdef USE_UVM_ALLOC
typedef UVMAllocator AllocatorType;
#else
typedef CPUAllocator AllocatorType;
#endif
}

void *operator new (std::size_t size) throw (std::bad_alloc)
{
  return DynamicPoolAllocator<AllocatorType>::getInstance().allocate(size);
}

void *operator new[] (std::size_t size) throw (std::bad_alloc)
{
  return DynamicPoolAllocator<AllocatorType>::getInstance().allocate(size);
}

void operator delete (void *ptr) throw()
{
  return DynamicPoolAllocator<AllocatorType>::getInstance().free(ptr);
}

void operator delete[] (void *ptr) throw()
{
  return DynamicPoolAllocator<AllocatorType>::getInstance().free(ptr);
}
