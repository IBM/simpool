#include "FixedPoolAllocator.hpp"
#include "DynamicPoolAllocator.hpp"
#include "StdAllocator.hpp"

typedef FixedPoolAllocator<int, StdAllocator> FPA;
typedef DynamicPoolAllocator<StdAllocator, StdAllocator> DPA;

int main() {
  FPA& fpa = FPA::getInstance();
  DPA& dpa = DPA::getInstance();
  return 0;
}
