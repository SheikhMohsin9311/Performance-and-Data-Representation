#ifndef PERF_HELPER_H
#define PERF_HELPER_H

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ── Raw sample from a single measured iteration ────────────────────────── */
typedef struct {
    uint64_t cycles;
    uint64_t instructions;
    uint64_t cache_misses;
    uint64_t branches;
    uint64_t branch_misses;
    uint64_t l1_misses;
    uint64_t tlb_misses;
} perf_sample_t;

/* ── Aggregated statistics across all samples ───────────────────────────── */
typedef struct {
    uint64_t median_cycles;
    uint64_t min_cycles;
    uint64_t median_instructions;
    uint64_t median_cache_misses;
    uint64_t median_branches;
    uint64_t median_branch_misses;
    uint64_t median_l1_misses;
    uint64_t median_tlb_misses;
    double   cv_pct;    /* coefficient of variation for cycles (%) */
} perf_stats_t;

/* ── Open a single perf counter, return fd or -1 on failure ─────────────── */
static int open_perf_counter(uint32_t type, uint64_t config) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.type    = type;
    pe.size    = sizeof(pe);
    pe.config  = config;
    pe.disabled    = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv  = 1;

    int fd = syscall(SYS_perf_event_open, &pe, 0, -1, -1, 0);
    return fd; /* -1 on failure; caller must check */
}

/* ── Enable all counters ────────────────────────────────────────────────── */
static inline void start_counters(int* fds, int count) {
    for (int i = 0; i < count; i++) {
        if (fds[i] != -1) {
            ioctl(fds[i], PERF_EVENT_IOC_RESET,  0);
            ioctl(fds[i], PERF_EVENT_IOC_ENABLE, 0);
        }
    }
}

/* ── Disable all counters and read into a raw sample ────────────────────── */
static inline void stop_counters(int* fds, int count, perf_sample_t* s) {
    uint64_t* results = (uint64_t*)s;
    for (int i = 0; i < count; i++) {
        if (fds[i] != -1) {
            ioctl(fds[i], PERF_EVENT_IOC_DISABLE, 0);
            ssize_t r = read(fds[i], &results[i], sizeof(uint64_t));
            (void)r;
        } else {
            results[i] = 0;
        }
    }
    /* Warn once if critical counters are zero (likely paranoid/permission issue) */
    static int warned = 0;
    if (!warned && (s->cycles == 0 || s->instructions == 0)) {
        fprintf(stderr,
            "[WARNING] perf counters (cycles/instrs) returned 0. "
            "Run stabilize.sh or lower /proc/sys/kernel/perf_event_paranoid.\n");
        warned = 1;
    }
}

/* ── qsort comparator for uint64_t ─────────────────────────────────────── */
static int _cmp_u64(const void* a, const void* b) {
    uint64_t x = *(const uint64_t*)a;
    uint64_t y = *(const uint64_t*)b;
    return (x > y) - (x < y);
}

/* ── Compute median / min / CV from an array of uint64_t samples ─────────
   Sorts the array in-place.                                                 */
static inline uint64_t _median_u64(uint64_t* arr, int n) {
    qsort(arr, n, sizeof(uint64_t), _cmp_u64);
    if (n % 2 == 1) return arr[n / 2];
    return (arr[n / 2 - 1] + arr[n / 2]) / 2;
}

/* ── Aggregate an array of perf_sample_t into perf_stats_t ──────────────── */
static inline void compute_perf_stats(perf_sample_t* samples, int n,
                                      perf_stats_t* out) {
    /* Temporary arrays for each metric */
    uint64_t* cyc   = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* ins   = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* cm    = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* br    = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* brm   = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* l1    = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* tlb   = (uint64_t*)malloc(n * sizeof(uint64_t));

    for (int i = 0; i < n; i++) {
        cyc[i] = samples[i].cycles;
        ins[i] = samples[i].instructions;
        cm[i]  = samples[i].cache_misses;
        br[i]  = samples[i].branches;
        brm[i] = samples[i].branch_misses;
        l1[i]  = samples[i].l1_misses;
        tlb[i] = samples[i].tlb_misses;
    }

    /* Sort once to get median and min for cycles */
    qsort(cyc, n, sizeof(uint64_t), _cmp_u64);
    out->min_cycles    = cyc[0];
    out->median_cycles = (n % 2 == 1) ? cyc[n/2] : (cyc[n/2-1] + cyc[n/2]) / 2;

    /* Coefficient of variation = stddev / mean * 100 */
    double sum = 0.0, sum2 = 0.0;
    for (int i = 0; i < n; i++) { sum += cyc[i]; sum2 += (double)cyc[i] * cyc[i]; }
    double mean = sum / n;
    double var  = (sum2 / n) - (mean * mean);
    out->cv_pct = (mean > 0) ? (sqrt(var > 0 ? var : 0) / mean) * 100.0 : 0.0;

    out->median_instructions  = _median_u64(ins, n);
    out->median_cache_misses  = _median_u64(cm,  n);
    out->median_branches      = _median_u64(br,  n);
    out->median_branch_misses = _median_u64(brm, n);
    out->median_l1_misses     = _median_u64(l1,  n);
    out->median_tlb_misses    = _median_u64(tlb, n);

    free(cyc); free(ins); free(cm); free(br); free(brm); free(l1); free(tlb);
}

#endif /* PERF_HELPER_H */
