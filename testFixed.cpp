#include <iostream>
#include "FixedPoolAllocator.hpp"
#include "Allocator.hpp"
#include <chrono>
using namespace std::chrono;

const int numOuter = 1 << 20;
const int numInner = 1 << 10;

const int NP       = 1 << 6;

struct A { double a[numInner]; };

typedef FixedPoolAllocator<A, CPUAllocator, NP> AllocType;

int main()
{
  AllocType& f = AllocType::getInstance();

  std::cout << "Test: Allocating and deleting " << numOuter << " objects of size " << numInner << std::endl;

  {
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    for (int i=0; i<numOuter; i++) {
      A *t = new A;
      for (int i=0; i<numInner; i++) t->a[i] = 1.0;
      delete t;
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> timeSpan = duration_cast< duration<double> >(t2 - t1);
    std::cout << "Time with system allocator = " << timeSpan.count() << std::endl;
  }

  {
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    for (int i=0; i<numOuter; i++) {
      A* t = f.allocate();
      for (int i=0; i<numInner; i++) t->a[i] = 1.0;
      f.deallocate(t);
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> timeSpan = duration_cast< duration<double> >(t2 - t1);
    std::cout << "Time with FixedPoolAllocator = " << timeSpan.count() << std::endl;
  }

  return 0;
}
