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
    uint64_t avg_cycles;
    uint64_t min_cycles;
    double   stddev_cycles;   /* sample standard deviation of cycles */
    double   ci95_cycles;     /* 95% CI half-width: 1.96*stddev/sqrt(n) */
    uint64_t median_instructions;
    uint64_t avg_instructions;
    uint64_t median_cache_misses;
    uint64_t avg_cache_misses;
    uint64_t median_branches;
    uint64_t avg_branches;
    uint64_t median_branch_misses;
    uint64_t avg_branch_misses;
    uint64_t median_l1_misses;
    uint64_t avg_l1_misses;
    uint64_t median_tlb_misses;
    uint64_t avg_tlb_misses;
    double   cv_pct;    /* coefficient of variation for cycles (%) */
} perf_stats_t;

/* ── METRICS3 output helper macro ────────────────────────────────────────── */
/* Columns: tag,N,
            med_cyc,avg_cyc,med_ins,avg_ins,ipc,
            med_cm,avg_cm,med_br,avg_br,
            med_brm,avg_brm,med_l1,avg_l1,med_tlb,avg_tlb,
            cv_pct,stddev_cycles,ci95_cycles */
#define PRINT_METRICS3(N, stats, ipc)                                              \
    printf("METRICS3,%d,%lu,%lu,%lu,%lu,%.2f,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.1f,%.0f,%.0f\n", \
           (int)(N),                                                               \
           (stats).median_cycles, (stats).avg_cycles,                              \
           (stats).median_instructions, (stats).avg_instructions, (ipc),           \
           (stats).median_cache_misses, (stats).avg_cache_misses,                  \
           (stats).median_branches, (stats).avg_branches,                          \
           (stats).median_branch_misses, (stats).avg_branch_misses,                \
           (stats).median_l1_misses, (stats).avg_l1_misses,                        \
           (stats).median_tlb_misses, (stats).avg_tlb_misses,                       \
           (stats).cv_pct,                                                         \
           (stats).stddev_cycles, (stats).ci95_cycles)

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
    /* Warn once if critical counters are zero (paranoid / permission issue) */
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

/* ── Compute median from a sorted / unsorted uint64_t array ─────────────── */
static inline uint64_t _median_u64(uint64_t* arr, int n) {
    qsort(arr, n, sizeof(uint64_t), _cmp_u64);
    if (n % 2 == 1) return arr[n / 2];
    return (arr[n / 2 - 1] + arr[n / 2]) / 2;
}

/* ── Aggregate an array of perf_sample_t into perf_stats_t ──────────────── */
static inline void compute_perf_stats(perf_sample_t* samples, int n,
                                      perf_stats_t* out) {
    uint64_t* cyc = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* ins = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* cm  = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* br  = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* brm = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* l1  = (uint64_t*)malloc(n * sizeof(uint64_t));
    uint64_t* tlb = (uint64_t*)malloc(n * sizeof(uint64_t));

    uint64_t sum_cyc = 0, sum_ins = 0, sum_cm = 0, sum_br = 0, sum_brm = 0, sum_l1 = 0, sum_tlb = 0;

    for (int i = 0; i < n; i++) {
        cyc[i] = samples[i].cycles;
        ins[i] = samples[i].instructions;
        cm[i]  = samples[i].cache_misses;
        br[i]  = samples[i].branches;
        brm[i] = samples[i].branch_misses;
        l1[i]  = samples[i].l1_misses;
        tlb[i] = samples[i].tlb_misses;

        sum_cyc += cyc[i];
        sum_ins += ins[i];
        sum_cm  += cm[i];
        sum_br  += br[i];
        sum_brm += brm[i];
        sum_l1  += l1[i];
        sum_tlb += tlb[i];
    }

    out->avg_cycles       = sum_cyc / n;
    out->avg_instructions = sum_ins / n;
    out->avg_cache_misses = sum_cm / n;
    out->avg_branches     = sum_br / n;
    out->avg_branch_misses= sum_brm / n;
    out->avg_l1_misses    = sum_l1 / n;
    out->avg_tlb_misses   = sum_tlb / n;

    /* Sort cycles to compute median and min */
    qsort(cyc, n, sizeof(uint64_t), _cmp_u64);
    out->min_cycles    = cyc[0];
    out->median_cycles = (n % 2 == 1) ? cyc[n/2] : (cyc[n/2-1] + cyc[n/2]) / 2;

    /* Bessel-corrected sample stddev, CV, and 95% CI half-width */
    double sum = 0.0, sum2 = 0.0;
    for (int i = 0; i < n; i++) { sum += cyc[i]; sum2 += (double)cyc[i]*cyc[i]; }
    double mean = sum / n;
    double var  = (n > 1) ? (sum2 - sum*sum/n) / (n-1) : 0.0;
    double sd   = sqrt(var > 0 ? var : 0);
    out->stddev_cycles = sd;
    out->ci95_cycles   = (n > 1) ? 1.96 * sd / sqrt((double)n) : 0.0;
    out->cv_pct        = (mean > 0) ? (sd / mean) * 100.0 : 0.0;

    out->median_instructions  = _median_u64(ins, n);
    out->median_cache_misses  = _median_u64(cm,  n);
    out->median_branches      = _median_u64(br,  n);
    out->median_branch_misses = _median_u64(brm, n);
    out->median_l1_misses     = _median_u64(l1,  n);
    out->median_tlb_misses    = _median_u64(tlb, n);

    free(cyc); free(ins); free(cm); free(br); free(brm); free(l1); free(tlb);
}

#endif /* PERF_HELPER_H */
