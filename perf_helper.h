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

typedef struct {
    uint64_t cycles;
    uint64_t instructions;
    uint64_t cache_misses;
    uint64_t branches;
    uint64_t branch_misses;
    uint64_t l1_misses;
    uint64_t tlb_misses;
} perf_metrics_t;

static int open_perf_counter(uint32_t type, uint64_t config) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.type = type;
    pe.size = sizeof(pe);
    pe.config = config;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    int fd = syscall(SYS_perf_event_open, &pe, 0, -1, -1, 0);
    if (fd == -1) {
        // Fallback or ignore for non-critical counters if needed
        return -1; 
    }
    return fd;
}

static inline void start_counters(int* fds, int count) {
    for (int i = 0; i < count; i++) {
        if (fds[i] != -1) {
            ioctl(fds[i], PERF_EVENT_IOC_RESET, 0);
            ioctl(fds[i], PERF_EVENT_IOC_ENABLE, 0);
        }
    }
}

static inline void stop_counters(int* fds, int count, perf_metrics_t* m) {
    uint64_t* results = (uint64_t*)m;
    for (int i = 0; i < count; i++) {
        if (fds[i] != -1) {
            ioctl(fds[i], PERF_EVENT_IOC_DISABLE, 0);
            ssize_t r = read(fds[i], &results[i], sizeof(uint64_t));
            (void)r;
        } else {
            results[i] = 0;
        }
    }
}


#endif
