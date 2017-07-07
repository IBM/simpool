#include <iostream>
#include "DynamicPoolAllocator.hpp"
#include "Allocator.hpp"
#include <chrono>
using namespace std::chrono;

typedef DynamicPoolAllocator<CPUAllocator> AllocType;

const int numOuter = 1 << 20;
const int numInner = 1 << 10;
struct A { double *a; };

int main()
{
  AllocType& f = AllocType::getInstance();

  std::cout << "Test: Allocating and deleting " << numOuter << " objects of size " << numInner << std::endl;

  {
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    for (int i=0; i<numOuter; i++) {
      A *t = new A;
      t->a = new double[numInner];
      for (int j=0; j<numInner; j++) t->a[j] = 1.0;
      delete t->a;
      delete t;
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> timeSpan = duration_cast< duration<double> >(t2 - t1);
    std::cout << "Time with system allocator = " << timeSpan.count() << std::endl;
  }

  {
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    for (int i=0; i<numOuter; i++) {
      A* t = (A *) f.allocate(sizeof(A));
      t->a = (double *) f.allocate(numInner * sizeof(double));
      for (int j=0; j<numInner; j++) t->a[j] = 1.0;
      f.deallocate(t->a);
      f.deallocate(t);
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> timeSpan = duration_cast< duration<double> >(t2 - t1);
    std::cout << "Time with DynamicPoolAllocator = " << timeSpan.count() << std::endl;
  }

  std::cout << "Test: Allocating with occasional deletion " << numInner << " objects of size " << numInner << std::endl;

  {
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    A *t = new A[numInner];
    for (int i=0; i<numInner; i++) {
      t[i].a = new double[numInner];
      for (int j=0; j<numInner; j++) t[i].a[j] = 1.0;
      // Intentionally leak
      if (!(i%5)) {
        delete t[i].a;
      }
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> timeSpan = duration_cast< duration<double> >(t2 - t1);
    std::cout << "Time with system allocator = " << timeSpan.count() << std::endl;
  }

  {
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    A *t = (A *) f.allocate(numInner * sizeof(A));
    for (int i=0; i<numInner; i++) {
      t[i].a = (double *) f.allocate(numInner * sizeof(double));
      for (int j=0; j<numInner; j++) t[i].a[j] = 1.0;
      // Intentionally leak
      if (!(i%5)) {
        f.deallocate(t[i].a);
      }
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> timeSpan = duration_cast< duration<double> >(t2 - t1);
    std::cout << "Time with DynamicPoolAllocator = " << timeSpan.count() << std::endl;
  }

  return 0;
}
