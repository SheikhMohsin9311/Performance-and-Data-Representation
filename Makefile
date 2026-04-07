CXX      = g++

# Build variant: O2 (default) | O3 | native
# Usage: make              → -O2  (production baseline)
#        make VARIANT=O3   → -O3  (aggressive optimisation)
#        make VARIANT=native → -O3 -march=native  (SIMD, AVX2/AVX512)
VARIANT  ?= O2

ifeq ($(VARIANT),native)
    CXXFLAGS = -O3 -march=native
else
    CXXFLAGS = -$(VARIANT)
endif

TARGETS = second third linked_list heap hash_map vector_ds deque_ds rb_tree \
          aos soa skip_list btree veb_tree trie slab_list circular_buffer

all: $(TARGETS)

second:          second.cpp          ; $(CXX) $(CXXFLAGS) -o $@ $<
third:           third.cpp           ; $(CXX) $(CXXFLAGS) -o $@ $<
linked_list:     linked_list.cpp     ; $(CXX) $(CXXFLAGS) -o $@ $<
heap:            heap.cpp            ; $(CXX) $(CXXFLAGS) -o $@ $<
hash_map:        hash_map.cpp        ; $(CXX) $(CXXFLAGS) -o $@ $<
vector_ds:       vector_ds.cpp       ; $(CXX) $(CXXFLAGS) -o $@ $<
deque_ds:        deque_ds.cpp        ; $(CXX) $(CXXFLAGS) -o $@ $<
rb_tree:         rb_tree.cpp         ; $(CXX) $(CXXFLAGS) -o $@ $<
aos:             aos.cpp             ; $(CXX) $(CXXFLAGS) -o $@ $<
soa:             soa.cpp             ; $(CXX) $(CXXFLAGS) -o $@ $<
skip_list:       skip_list.cpp       ; $(CXX) $(CXXFLAGS) -o $@ $<
btree:           btree.cpp           ; $(CXX) $(CXXFLAGS) -o $@ $<
veb_tree:        veb_tree.cpp        ; $(CXX) $(CXXFLAGS) -o $@ $<
trie:            trie.cpp            ; $(CXX) $(CXXFLAGS) -o $@ $<
slab_list:       slab_list.cpp       ; $(CXX) $(CXXFLAGS) -o $@ $<
circular_buffer: circular_buffer.cpp ; $(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)