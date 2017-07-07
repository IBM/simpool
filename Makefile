CXX	 = clang++
# CXXFLAGS = -O0 -fsanitize=address -g
CXXFLAGS = -O3 -std=c++11
LDFLAGS	 =

HAS_CUDA = NO

ifeq ($(HAS_CUDA),YES)
CXXFLAGS += -I/usr/local/cuda/include -DHAS_CUDA
LDFLAGS += -L/usr/local/cuda/lib64 -lcudart
endif

TESTS=testDynamic testFixed

all: $(TESTS)

$(TESTS) : % : %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(TESTS)
