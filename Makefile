CXX	 = clang++
# CXXFLAGS = -O0 -fsanitize=address -g
CXXFLAGS = -O3 -std=c++11
LDFLAGS	 =

HAS_CUDA = NO
USE_UVM_ALLOC = NO

ifeq ($(HAS_CUDA),YES)

CXXFLAGS += -I/usr/local/cuda/include -DHAS_CUDA
LDFLAGS += -L/usr/local/cuda/lib64 -lcudart

ifeq ($(USE_UVM_ALLOC),YES)
CXXFLAGS += -DUSE_UVM_ALLOC
endif

endif

TESTS=testDynamic testFixed testAlloc
HEADERS=Allocator.hpp \
	FixedPoolAllocator.hpp \
	DynamicPoolAllocator.hpp \
	STLAlloc.hpp

all: $(TESTS)

$(TESTS) : % : %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(TESTS)
