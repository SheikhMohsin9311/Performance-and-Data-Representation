#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>

// ==========================================
// Hardware Measurement Tools
// ==========================================
static inline uint64_t bench_start(void) {
    unsigned cycles_low, cycles_high;
    asm volatile("CPUID\n\t" "RDTSCP\n\t" "mov %%edx, %0\n\t" "mov %%eax, %1\n\t"
                 : "=r" (cycles_high), "=r" (cycles_low) :: "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)cycles_high << 32) | cycles_low;
}

static inline uint64_t bench_end(void) {
    unsigned cycles_low, cycles_high;
    asm volatile("RDTSCP\n\t" "mov %%edx, %0\n\t" "mov %%eax, %1\n\t" "CPUID\n\t"
                 : "=r" (cycles_high), "=r" (cycles_low) :: "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)cycles_high << 32) | cycles_low;
}

long long get_time_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
}

// ==========================================
// Data Structures
// ==========================================
struct EmployeeAoS { uint64_t id; uint64_t salary; char name[1]; };
struct EmployeeSoA { uint64_t* id; uint64_t* salary; char (*name)[1]; };
struct Node { uint64_t data; struct Node* next; };
struct BSTNode { uint64_t key; struct BSTNode* left; struct BSTNode* right; };

// Helper to free BST
void free_bst(struct BSTNode* root) {
    if (!root) return;
    free_bst(root->left);
    free_bst(root->right);
    free(root);
}

int main(void) {
    const char* filename = "hardware_metricsPC.csv";
    FILE *csv = fopen(filename, "a+");
    if (!csv) {
        printf("Error: Could not open %s\n", filename);
        return 1;
    }

    fseek(csv, 0, SEEK_END);
    if (ftell(csv) == 0) {
        fprintf(csv, "Run_Timestamp,Data_Structure,Elements,Time_ns,Clock_Cycles\n");
    }

    long run_timestamp = (long)time(NULL);
    srand((unsigned)time(NULL)); 
    printf("Starting master benchmark run %ld...\n", run_timestamp);

    int sizes[] = {1000, 10000, 100000, 1000000, 5000000}; // Capped at 5M to keep runtimes reasonable
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < num_sizes; s++) {
        int N = sizes[s];
        struct timespec start_t, end_t;
        uint64_t start_c, end_c;
        volatile uint64_t dummy_accumulator = 0;

        printf("\n--- Benchmarking N=%d ---\n", N);

        // Pre-generate random keys for search benchmarks (1 to N)
        uint64_t* search_keys = malloc(N * sizeof(uint64_t));
        for(int i = 0; i < N; i++) search_keys[i] = i + 1;
        
        // Fisher-Yates Shuffle to randomize memory access & balance trees
        for (int i = N - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            uint64_t temp = search_keys[i]; search_keys[i] = search_keys[j]; search_keys[j] = temp;
        }

        // ---------------------------------------------------------
        // 1. AoS vs SoA (Prefetcher & Memory Bandwidth)
        // ---------------------------------------------------------
        struct EmployeeAoS* aos = malloc(N * sizeof(struct EmployeeAoS));
        for(int i = 0; i < N; i++) aos[i].salary = 1;
        
        clock_gettime(CLOCK_MONOTONIC, &start_t);
        start_c = bench_start();
        uint64_t sum_aos = 0;
        for(int i = 0; i < N; i++) sum_aos += aos[i].salary;
        end_c = bench_end();
        clock_gettime(CLOCK_MONOTONIC, &end_t);
        dummy_accumulator += sum_aos;
        fprintf(csv, "%ld,AoS_Sum,%d,%lld,%" PRIu64 "\n", run_timestamp, N, get_time_ns(start_t, end_t), end_c - start_c);
        free(aos);

        struct EmployeeSoA soa;
        soa.id = malloc(N * sizeof(uint64_t)); soa.salary = malloc(N * sizeof(uint64_t)); soa.name = malloc(N * 16 * sizeof(char));
        for(int i = 0; i < N; i++) soa.salary[i] = 1;

        clock_gettime(CLOCK_MONOTONIC, &start_t);
        start_c = bench_start();
        uint64_t sum_soa = 0;
        for(int i = 0; i < N; i++) sum_soa += soa.salary[i];
        end_c = bench_end();
        clock_gettime(CLOCK_MONOTONIC, &end_t);
        dummy_accumulator += sum_soa;
        fprintf(csv, "%ld,SoA_Sum,%d,%lld,%" PRIu64 "\n", run_timestamp, N, get_time_ns(start_t, end_t), end_c - start_c);
        free(soa.id); free(soa.salary); free(soa.name);

        // ---------------------------------------------------------
        // 2. Linear Array Queue vs Linked List (Cache Associativity)
        // ---------------------------------------------------------
        uint64_t* arr_queue = malloc(N * sizeof(uint64_t));
        clock_gettime(CLOCK_MONOTONIC, &start_t);
        start_c = bench_start();
        for(int i = 0; i < N; i++) arr_queue[i] = i;
        uint64_t sum_arr_queue = 0;
        for(int i = 0; i < N; i++) sum_arr_queue += arr_queue[i];
        end_c = bench_end();
        clock_gettime(CLOCK_MONOTONIC, &end_t);
        dummy_accumulator += sum_arr_queue;
        fprintf(csv, "%ld,Array_Queue,%d,%lld,%" PRIu64 "\n", run_timestamp, N, get_time_ns(start_t, end_t), end_c - start_c);
        free(arr_queue);

        struct Node* head = NULL; struct Node* tail = NULL;
        clock_gettime(CLOCK_MONOTONIC, &start_t);
        start_c = bench_start();
        for(int i = 0; i < N; i++) {
            struct Node* n = malloc(sizeof(struct Node));
            n->data = i; n->next = NULL;
            if (!head) { head = n; tail = n; } else { tail->next = n; tail = n; }
        }
        uint64_t sum_list_queue = 0;
        while(head != NULL) {
            sum_list_queue += head->data;
            struct Node* temp = head; head = head->next; free(temp);
        }
        end_c = bench_end();
        clock_gettime(CLOCK_MONOTONIC, &end_t);
        dummy_accumulator += sum_list_queue;
        fprintf(csv, "%ld,LinkedList_Queue,%d,%lld,%" PRIu64 "\n", run_timestamp, N, get_time_ns(start_t, end_t), end_c - start_c);

        // ---------------------------------------------------------
        // 3. Binary Search Tree (Branch Predictor Thrashing)
        // ---------------------------------------------------------
        struct BSTNode* bst_root = NULL;
        for (int i = 0; i < N; i++) {
            struct BSTNode* n = malloc(sizeof(struct BSTNode));
            n->key = search_keys[i]; n->left = n->right = NULL;
            if (!bst_root) { bst_root = n; continue; }
            struct BSTNode* curr = bst_root;
            while (1) {
                if (search_keys[i] < curr->key) {
                    if (!curr->left) { curr->left = n; break; }
                    curr = curr->left;
                } else {
                    if (!curr->right) { curr->right = n; break; }
                    curr = curr->right;
                }
            }
        }

        // Benchmark purely the LOOKUP phase (Pointer Chasing)
        clock_gettime(CLOCK_MONOTONIC, &start_t);
        start_c = bench_start();
        uint64_t bst_found = 0;
        for (int i = 0; i < N; i++) {
            struct BSTNode* curr = bst_root;
            uint64_t target = search_keys[i];
            while (curr) {
                if (target == curr->key) { bst_found++; break; }
                curr = (target < curr->key) ? curr->left : curr->right;
            }
        }
        end_c = bench_end();
        clock_gettime(CLOCK_MONOTONIC, &end_t);
        dummy_accumulator += bst_found;
        fprintf(csv, "%ld,BST_Random_Search,%d,%lld,%" PRIu64 "\n", run_timestamp, N, get_time_ns(start_t, end_t), end_c - start_c);
        free_bst(bst_root);

        // ---------------------------------------------------------
        // 4. Hash Table with Linear Probing (Cache Line Spatial Locality)
        // ---------------------------------------------------------
        uint64_t ht_size = N * 2; // 50% load factor
        uint64_t* hash_table = calloc(ht_size, sizeof(uint64_t));
        
        for (int i = 0; i < N; i++) {
            uint64_t key = search_keys[i];
            uint64_t idx = key % ht_size;
            while (hash_table[idx] != 0) { idx = (idx + 1) % ht_size; }
            hash_table[idx] = key;
        }

        // Benchmark purely the LOOKUP phase (Cache hit rate)
        clock_gettime(CLOCK_MONOTONIC, &start_t);
        start_c = bench_start();
        uint64_t ht_found = 0;
        for (int i = 0; i < N; i++) {
            uint64_t target = search_keys[i];
            uint64_t idx = target % ht_size;
            while (hash_table[idx] != 0) {
                if (hash_table[idx] == target) { ht_found++; break; }
                idx = (idx + 1) % ht_size;
            }
        }
        end_c = bench_end();
        clock_gettime(CLOCK_MONOTONIC, &end_t);
        dummy_accumulator += ht_found;
        fprintf(csv, "%ld,HashTable_Linear_Search,%d,%lld,%" PRIu64 "\n", run_timestamp, N, get_time_ns(start_t, end_t), end_c - start_c);
        free(hash_table);
        free(search_keys);

        // ---------------------------------------------------------
        // 5. Matrix Traversal (Row-Major vs Col-Major TLB Misses)
        // ---------------------------------------------------------
        uint64_t dim = 1; 
        while (dim * dim < (uint64_t)N) dim++; 
        uint64_t mat_size = dim * dim; // Force a perfect square close to N
        uint64_t* matrix = malloc(mat_size * sizeof(uint64_t));
        for(uint64_t i = 0; i < mat_size; i++) matrix[i] = 1;

        // Row Major (Fast, hardware prefetcher friendly)
        clock_gettime(CLOCK_MONOTONIC, &start_t);
        start_c = bench_start();
        uint64_t sum_row = 0;
        for (uint64_t r = 0; r < dim; r++) {
            for (uint64_t c = 0; c < dim; c++) {
                sum_row += matrix[r * dim + c];
            }
        }
        end_c = bench_end();
        clock_gettime(CLOCK_MONOTONIC, &end_t);
        dummy_accumulator += sum_row;
        fprintf(csv, "%ld,Matrix_Row_Major,%d,%lld,%" PRIu64 "\n", run_timestamp, (int)mat_size, get_time_ns(start_t, end_t), end_c - start_c);

        // Column Major (Slow, defeats prefetcher, causes TLB thrashing)
        clock_gettime(CLOCK_MONOTONIC, &start_t);
        start_c = bench_start();
        uint64_t sum_col = 0;
        for (uint64_t c = 0; c < dim; c++) {
            for (uint64_t r = 0; r < dim; r++) {
                sum_col += matrix[r * dim + c];
            }
        }
        end_c = bench_end();
        clock_gettime(CLOCK_MONOTONIC, &end_t);
        dummy_accumulator += sum_col;
        fprintf(csv, "%ld,Matrix_Col_Major,%d,%lld,%" PRIu64 "\n", run_timestamp, (int)mat_size, get_time_ns(start_t, end_t), end_c - start_c);

        free(matrix);
    }

    fclose(csv);
    printf("\nRun complete! Total metrics appended to %s\n", filename);
    return 0;
}