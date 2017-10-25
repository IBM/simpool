#ifndef DYNAMICPOOLALLOCATOR_HPP
#define DYNAMICPOOLALLOCATOR_HPP

#include <cstddef>
#include <cassert>
#include "Allocator.hpp"
#include "FixedPoolAllocator.hpp"

struct Block
{
  char *data;
  std::size_t size;
  bool isHead;
  Block *next;
};

template <class MA, class IA = CPUAllocator, std::size_t MINSIZE = (1 << 6)>
class DynamicPoolAllocator
{
  // Allocator for the underlying data
  typedef FixedPoolAllocator<struct Block, IA, 1 << 6> BlockAlloc;
  BlockAlloc blockAllocator;

  // Start of the nodes of used and free block lists
  struct Block *usedBlocks;
  struct Block *freeBlocks;

  // Total size allocated in bytes
  std::size_t loanedSize;

  // Search the list of free blocks and return a usable one if that exists, else NULL
  void findUsableBlock(struct Block *&best, struct Block *&prev, std::size_t size) {
    best = prev = NULL;
    for ( struct Block *iter = freeBlocks, *iterPrev = NULL ; iter ; iter = iter->next ) {
      if ( iter->size >= size && (!best || iter->size < best->size) ) {
        best = iter;
        prev = iterPrev;
      }
      iterPrev = iter;
    }
  }

  // Allocate a new block and add it to the list of free blocks
  void allocateBlock(struct Block *&curr, struct Block *&prev, std::size_t size) {
    std::size_t allocSize = std::max(size, MINSIZE);
    curr = prev = NULL;
    void *data = NULL;

    // Allocate data
    data = MA::allocate(size);
    assert(data);

    // Find next and prev such that next->data is still smaller than data (keep ordered)
    struct Block *next;
    for ( next = freeBlocks; next && next->data < data ; next = next->next ) {
      prev = next;
    }

    // Allocate the block
    curr = (struct Block *) blockAllocator.allocate();
    if (!curr) return;
    curr->data = reinterpret_cast<char *>(data);
    curr->size = allocSize;
    curr->isHead = true;
    curr->next = next;
    loanedSize += allocSize;

    // Insert
    if (prev) prev->next = curr;
    else freeBlocks = curr;
  }

  void splitBlock(struct Block *&curr, struct Block *&prev, std::size_t size) {
    struct Block *next;
    if ( curr->size == size ) {
      // Keep it
      next = curr->next;
    }
    else {
      // Split the block
      std::size_t remaining = curr->size - size;
      struct Block *newBlock = (struct Block *) blockAllocator.allocate();
      if (!newBlock) return;
      newBlock->data = curr->data + size;
      newBlock->size = remaining;
      newBlock->isHead = false;
      newBlock->next = curr->next;
      next = newBlock;
      curr->size = size;
    }

    loanedSize += size;
    if (prev) prev->next = next;
    else freeBlocks = next;
  }

  void releaseBlock(struct Block *curr, struct Block *prev) {
    assert(curr != NULL);
    loanedSize -= curr->size;

    if (prev) prev->next = curr->next;
    else usedBlocks = curr->next;

    // Find location to put this block in the freeBlocks list
    prev = NULL;
    for ( struct Block *temp = freeBlocks ; temp && temp->data < curr->data ; temp = temp->next ) {
      prev = temp;
    }

    // Keep track of the successor
    struct Block *next = prev ? prev->next : freeBlocks;

    // Check if prev and curr can be merged
    if ( prev && prev->data + prev->size == curr->data && !curr->isHead ) {
      prev->size = prev->size + curr->size;
      blockAllocator.deallocate(curr); // keep data
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
      blockAllocator.deallocate(next); // keep data
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
      assert(freeBlocks->isHead);
      loanedSize -= freeBlocks->size;
      MA::deallocate(freeBlocks->data);
      struct Block *curr = freeBlocks;
      freeBlocks = freeBlocks->next;
      blockAllocator.deallocate(curr);
    }
  }

public:

  DynamicPoolAllocator()
    : blockAllocator(), usedBlocks(NULL), freeBlocks(NULL), loanedSize(0) { }

  ~DynamicPoolAllocator() { freeAllBlocks(); }

  static inline DynamicPoolAllocator &getInstance() {
    static DynamicPoolAllocator instance;
    return instance;
  }

  void *allocate(std::size_t size) {
    struct Block *best, *prev;
    findUsableBlock(best, prev, size);

    // Allocate a block if needed
    if (!best) allocateBlock(best, prev, size);
    assert(best);

    // Split the free block
    splitBlock(best, prev, size);

    // Push node to the list of used nodes
    best->next = usedBlocks;
    usedBlocks = best;

    // Return the new pointer
    return usedBlocks->data;
  }

  void deallocate(void *ptr) {
    assert(ptr);

    struct Block *curr = usedBlocks, *prev = NULL;
    for ( ; curr && curr->data != ptr ; curr = curr->next ) {
      prev = curr;
    }

    if (!curr) return;

    releaseBlock(curr, prev);
  }

  std::size_t allocatedSize() const { return loanedSize; }

  std::size_t totalSize() const {
    return sizeof(*this) + allocatedSize() + blockAllocator.totalSize();
  }

  std::size_t numFreeBlocks() const {
    std::size_t nb = 0;
    for (struct Block *temp = freeBlocks; temp; temp = temp->next) nb++;
    return nb;
  }

  std::size_t numUsedBlocks() const {
    std::size_t nb = 0;
    for (struct Block *temp = usedBlocks; temp; temp = temp->next) nb++;
    return nb;
  }

};

#endif
