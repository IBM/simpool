#include "STLAlloc.hpp"
#include <vector>

int main()
{
  std::vector< int, STLAllocator<int> > v(10);

  return 0;
}
