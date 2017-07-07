#ifndef STLALLOC_HPP
#define STLALLOC_HPP

#include <limits>
#include <cstddef>
#include "Allocator.hpp"
#include "DynamicPoolAllocator.hpp"

template <class T>
struct STLAllocator {
  typedef T value_type;
  typedef std::size_t size_type;

#ifdef USE_UVM_ALLOC
  typedef UVMAllocator AllocatorType;
#else
  typedef CPUAllocator AllocatorType;
#endif

  DynamicPoolAllocator<AllocatorType> &m;

  STLAllocator() : m(DynamicPoolAllocator<AllocatorType>::getInstance()) { }
  STLAllocator(const STLAllocator&) { }
  T* allocate(std::size_t n) {
    return static_cast<T*>( m.allocate( n * sizeof(T) ) );
  }
  void deallocate(T* p, std::size_t n) { m.deallocate(p); }

  size_type max_size() const { return std::numeric_limits<unsigned int>::max(); }
};
template <class T, class U>
bool operator==(const STLAllocator<T>&, const STLAllocator<U>&) { return true; }
template <class T, class U>
bool operator!=(const STLAllocator<T>&, const STLAllocator<U>&) { return false; }

#endif
