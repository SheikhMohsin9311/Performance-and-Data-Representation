CC      = gcc
CFLAGS  = -O2 -std=c11 -Wall
LDFLAGS = -lm

TARGETS = second third linked_list heap hash_map vector_ds deque_ds rb_tree \
          aos soa skip_list btree veb_tree trie slab_list circular_buffer

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)