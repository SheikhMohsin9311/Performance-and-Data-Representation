CXX      = g++
CXXFLAGS = -O2

TARGETS = second third linked_list heap hash_map vector_ds deque_ds rb_tree

all: $(TARGETS)

second: second.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

third: third.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

linked_list: linked_list.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

heap: heap.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

hash_map: hash_map.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

vector_ds: vector_ds.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

deque_ds: deque_ds.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

rb_tree: rb_tree.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
	
run: all
	@for target in $(TARGETS); do \
		echo "\n=== Running $$target ==="; \
		./$$target; \
	done
