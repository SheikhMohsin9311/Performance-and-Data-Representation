#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "perf_helper.h"

static void assert_u64_eq(uint64_t actual, uint64_t expected, const char* label) {
    if (actual != expected) {
        fprintf(stderr, "Assertion failed for %s: expected %lu, got %lu\n",
                label, (unsigned long)expected, (unsigned long)actual);
        exit(1);
    }
}

static void assert_double_close(double actual, double expected, double tol, const char* label) {
    if (fabs(actual - expected) > tol) {
        fprintf(stderr, "Assertion failed for %s: expected %.6f, got %.6f\n",
                label, expected, actual);
        exit(1);
    }
}

static void test_median_even_odd(void) {
    uint64_t odd[] = {5, 1, 9};
    uint64_t even[] = {10, 2, 6, 4};

    assert_u64_eq(_median_u64(odd, 3), 5, "median odd");
    assert_u64_eq(_median_u64(even, 4), 5, "median even");
}

static void test_compute_perf_stats_single(void) {
    perf_sample_t samples[1] = {
        {.cycles = 100, .instructions = 250, .cache_misses = 3, .branches = 7,
         .branch_misses = 2, .l1_misses = 5, .tlb_misses = 11}
    };
    perf_stats_t stats;

    compute_perf_stats(samples, 1, &stats);

    assert_u64_eq(stats.avg_cycles, 100, "avg cycles");
    assert_u64_eq(stats.median_cycles, 100, "median cycles");
    assert_u64_eq(stats.min_cycles, 100, "min cycles");
    assert_u64_eq(stats.avg_instructions, 250, "avg instructions");
    assert_u64_eq(stats.median_instructions, 250, "median instructions");
    assert_u64_eq(stats.avg_cache_misses, 3, "avg cache misses");
    assert_u64_eq(stats.median_cache_misses, 3, "median cache misses");
    assert_u64_eq(stats.avg_branches, 7, "avg branches");
    assert_u64_eq(stats.median_branches, 7, "median branches");
    assert_u64_eq(stats.avg_branch_misses, 2, "avg branch misses");
    assert_u64_eq(stats.median_branch_misses, 2, "median branch misses");
    assert_u64_eq(stats.avg_l1_misses, 5, "avg l1 misses");
    assert_u64_eq(stats.median_l1_misses, 5, "median l1 misses");
    assert_u64_eq(stats.avg_tlb_misses, 11, "avg tlb misses");
    assert_u64_eq(stats.median_tlb_misses, 11, "median tlb misses");
    assert_double_close(stats.stddev_cycles, 0.0, 1e-9, "stddev cycles");
    assert_double_close(stats.ci95_cycles, 0.0, 1e-9, "ci95 cycles");
    assert_double_close(stats.cv_pct, 0.0, 1e-9, "cv pct");
}

static void test_compute_perf_stats_multiple(void) {
    perf_sample_t samples[3] = {
        {.cycles = 10, .instructions = 100, .cache_misses = 1, .branches = 50,
         .branch_misses = 5, .l1_misses = 2, .tlb_misses = 3},
        {.cycles = 20, .instructions = 200, .cache_misses = 2, .branches = 60,
         .branch_misses = 4, .l1_misses = 3, .tlb_misses = 4},
        {.cycles = 30, .instructions = 300, .cache_misses = 3, .branches = 70,
         .branch_misses = 6, .l1_misses = 4, .tlb_misses = 5}
    };
    perf_stats_t stats;

    compute_perf_stats(samples, 3, &stats);

    assert_u64_eq(stats.avg_cycles, 20, "avg cycles multi");
    assert_u64_eq(stats.median_cycles, 20, "median cycles multi");
    assert_u64_eq(stats.min_cycles, 10, "min cycles multi");
    assert_u64_eq(stats.avg_instructions, 200, "avg instructions multi");
    assert_u64_eq(stats.median_instructions, 200, "median instructions multi");
    assert_u64_eq(stats.avg_cache_misses, 2, "avg cache misses multi");
    assert_u64_eq(stats.median_cache_misses, 2, "median cache misses multi");
    assert_u64_eq(stats.avg_branches, 60, "avg branches multi");
    assert_u64_eq(stats.median_branches, 60, "median branches multi");
    assert_u64_eq(stats.avg_branch_misses, 5, "avg branch misses multi");
    assert_u64_eq(stats.median_branch_misses, 5, "median branch misses multi");
    assert_u64_eq(stats.avg_l1_misses, 3, "avg l1 misses multi");
    assert_u64_eq(stats.median_l1_misses, 3, "median l1 misses multi");
    assert_u64_eq(stats.avg_tlb_misses, 4, "avg tlb misses multi");
    assert_u64_eq(stats.median_tlb_misses, 4, "median tlb misses multi");

    double sum = 10.0 + 20.0 + 30.0;
    double sum2 = 10.0 * 10.0 + 20.0 * 20.0 + 30.0 * 30.0;
    double mean = sum / 3.0;
    double var = (sum2 - sum * sum / 3.0) / 2.0;
    double sd = sqrt(var);
    double ci95 = 1.96 * sd / sqrt(3.0);
    double cv = (sd / mean) * 100.0;

    assert_double_close(stats.stddev_cycles, sd, 1e-9, "stddev cycles multi");
    assert_double_close(stats.ci95_cycles, ci95, 1e-9, "ci95 cycles multi");
    assert_double_close(stats.cv_pct, cv, 1e-9, "cv pct multi");
}

static void test_stop_counters_invalid_fds(void) {
    int fds[2] = {-1, -1};
    perf_sample_t sample = {.cycles = 123, .instructions = 456};

    stop_counters(fds, 2, &sample);

    assert_u64_eq(sample.cycles, 0, "stop counters cycles");
    assert_u64_eq(sample.instructions, 0, "stop counters instructions");
}

static void test_open_perf_counter_invocation(void) {
    int fd = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    if (fd != -1) {
        close(fd);
    }
}

int main(void) {
    test_median_even_odd();
    test_compute_perf_stats_single();
    test_compute_perf_stats_multiple();
    test_stop_counters_invalid_fds();
    test_open_perf_counter_invocation();

    printf("perf_helper tests passed\n");
    return 0;
}
