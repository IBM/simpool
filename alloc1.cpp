#include <cstdlib>
#include <cstddef>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <vector>

struct Block
{
  char *data;
  std::size_t size;
  Block *next;
  bool isHead;

  // Construct a block
  Block(char *data_, std::size_t size_, Block *next_, bool isHead_)
    : data(data_), size(size_), next(next_), isHead(isHead_) { }
};

struct CPUAllocator
{
  static void *allocate(std::size_t size) { return std::malloc(size); }
  static void deallocate(void *ptr) { std::free(ptr); }
};

template <class MA>
class Manager
{
  // Head nodes of used and free block lists
  Block *usedBlocks;
  Block *freeBlocks;

  // Total size allocated in bytes
  std::size_t totalSize;
  std::size_t usedSize;

  const std::size_t blockSize;

  static const std::size_t minAllocSize;

  // Search the list of free blocks and return a usable one if that exists, else NULL
  void findUsableBlock(Block *&best, Block *&prev, std::size_t size) {
    best = prev = NULL;
    for ( Block *temp = freeBlocks, *tempPrev = NULL ; temp ; temp = temp->next ) {
      if ( temp->size >= size && (!best || temp->size < best->size) ) {
        best = temp;
        prev = tempPrev;
      }
      tempPrev = temp;
    }
  }

  // Allocate a new block and add it to the list of free blocks
  void allocateBlock(Block *&curr, Block *&prev, std::size_t size) {
    std::size_t allocSize = std::max(size, minAllocSize);
    curr = prev = NULL;
    void *data = NULL;

    // Allocate data
    data = MA::allocate(size);
    assert(data);

    // Find next and prev such that next->data is still smaller than data (keep ordered)
    Block *next;
    for ( next = freeBlocks; next && next->data < data ; next = next->next ) {
      prev = next;
    }

    // Allocate the block
    curr = (Block *) MA::allocate(blockSize);
    if (!curr) return;
    (*curr) = Block((char *) data, allocSize, next, true);
    totalSize += allocSize;

    // Insert
    if (prev) prev->next = curr;
    else freeBlocks = curr;
  }

  void splitBlock(Block *&curr, Block *&prev, std::size_t size) {
    Block *next;
    if ( curr->size == size ) {
      // Keep it
      next = curr->next;
    }
    else {
      // Split the block
      std::size_t remaining = curr->size - size;
      Block *newBlock = (Block *) MA::allocate(blockSize);
      if (!newBlock) return;
      (*newBlock) = Block(curr->data + size, remaining, curr->next, false);
      next = newBlock;
      curr->size = size;
    }

    usedSize += size;
    if (prev) prev->next = next;
    else freeBlocks = next;
  }

  void releaseBlock(Block *curr, Block *prev) {
    assert(curr != NULL);
    usedSize -= curr->size;

    if (prev) prev->next = curr->next;
    else usedBlocks = curr->next;

    prev = NULL;
    Block *temp = freeBlocks;
    for ( ; temp && temp->data < curr->data ; temp = temp->next ) {
      prev = temp;
    }

    // Keep track of the successor
    Block *next = prev ? prev->next : freeBlocks;

    // Check if prev and curr can be merged
    if ( prev && prev->data + prev->size == curr->data && !curr->isHead ) {
      prev->size = prev->size + curr->size;
      MA::deallocate(curr); // keep data
      curr = prev;
    }
    else if (prev) {
      prev->next = curr;
    }
    else {
      freeBlocks = curr;
    }

    // Check if curr and next can be merged
    if ( next && curr->data + curr->size == next->data && !next->isHead ) {
      curr->size = curr->size + next->size;
      curr->next = next->next;
      MA::deallocate(next);
    }
    else {
      curr->next = next;
    }
  }

  void freeAllBlocks() {
    while(usedBlocks) {
      releaseBlock(usedBlocks, NULL);
    }

    while(freeBlocks) {
#ifndef NDEBUG
      if (!freeBlocks->isHead) {
        std::cerr << "Left with a block that is not a head!" << std::endl;
      }
#endif
      totalSize -= freeBlocks->size;
      MA::deallocate(freeBlocks->data);
      Block *curr = freeBlocks;
      freeBlocks = freeBlocks->next;
      MA::deallocate(curr);
    }
  }

public:
  Manager()
    : usedBlocks(NULL), freeBlocks(NULL), totalSize(0), usedSize(0), blockSize(sizeof(Block)) { }

  ~Manager() { freeAllBlocks(); }

  static inline Manager &getInstance() {
    static Manager instance;
    return instance;
  }

  void *allocate(std::size_t size) {
    Block *best, *prev;
    findUsableBlock(best, prev, size);

    // Allocate a block if needed
    if (!best) allocateBlock(best, prev, size);
    if (!best) return best;

    // Split the free block
    splitBlock(best, prev, size);

    // Push node to the list of used nodes
    best->next = usedBlocks;
    usedBlocks = best;

    // Return the new pointer
    return usedBlocks->data;
  }

  void free(void *ptr) {
    if (!ptr) return;

    Block *curr = usedBlocks, *prev = NULL;
    for ( ; curr && curr->data != ptr ; curr = curr->next ) {
      prev = curr;
    }

    if (!curr) return;

    releaseBlock(curr, prev);
  }

  std::size_t allocatedSize() const { return totalSize; }

  std::size_t activeSize() const { return usedSize; }

  std::size_t numFreeBlocks() const {
    std::size_t nb = 0;
    for (Block *temp = freeBlocks; temp; temp = temp->next) nb++;
    return nb;
  }

  std::size_t numUsedBlocks() const {
    std::size_t nb = 0;
    for (Block *temp = usedBlocks; temp; temp = temp->next) nb++;
    return nb;
  }

};

template<> const std::size_t Manager<CPUAllocator>::minAllocSize = 1 << 20;

void *operator new (std::size_t size) throw (std::bad_alloc)
{
  return Manager<CPUAllocator>::getInstance().allocate(size);
}

void *operator new[] (std::size_t size) throw (std::bad_alloc)
{
  return Manager<CPUAllocator>::getInstance().allocate(size);
}

void operator delete (void *ptr) throw()
{
  return Manager<CPUAllocator>::getInstance().free(ptr);
}

void operator delete[] (void *ptr) throw()
{
  return Manager<CPUAllocator>::getInstance().free(ptr);
}

template <class MA, class T>
struct STLMallocator {
  typedef T value_type;
  typedef std::size_t size_type;
  STLMallocator() { }
  STLMallocator(const STLMallocator&) { }
  T* allocate(std::size_t n) {
    return static_cast<T*>(Manager<MA>::getInstance().allocate(n*sizeof(T)));
  }
  void deallocate(T* p, std::size_t) { Manager<MA>::getInstance().free(p); }

  size_type max_size() const { return 1 << 24; }
};
template <class MA, class T, class U>
bool operator==(const STLMallocator<MA,T>&, const STLMallocator<MA,U>&) { return true; }
template <class MA, class T, class U>
bool operator!=(const STLMallocator<MA,T>&, const STLMallocator<MA,U>&) { return false; }

void printMemoryUsage()
{
  Manager<CPUAllocator>& m = Manager<CPUAllocator>::getInstance();
  std::cout << "allocated size = " << m.allocatedSize() << std::endl;
  std::cout << "active size = " << m.activeSize() << std::endl;
  std::cout << "number of active/unused blocks = " << m.numUsedBlocks() << "/" << m.numFreeBlocks() << std::endl;
}

struct A
{
  int a;
  int b;
  double c;
};

int main()
{
  for (int i=0; i<500000; i++)
    A *a = new A[10];
  printMemoryUsage();

  return 0;
}
