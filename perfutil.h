#pragma once
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <iostream>

struct PerfMetrics {
    uint64_t cycles;
    uint64_t instructions;
    uint64_t cache_misses;
};

class PerfCounter {
    int fd_cycles;
    int fd_instructions;
    int fd_misses;

    // Helper to open a specific hardware counter
    int open_counter(uint32_t type, uint64_t config) {
        perf_event_attr pe;
        memset(&pe, 0, sizeof(pe));
        pe.type           = type;
        pe.size           = sizeof(pe);
        pe.config         = config;
        pe.disabled       = 1;
        pe.exclude_kernel = 1; // Exclude OS overhead
        pe.exclude_hv     = 1;
        
        int fd = syscall(SYS_perf_event_open, &pe, 0, -1, -1, 0);
        if (fd == -1) {
            std::cerr << "Error opening perf event. Run: sudo sysctl kernel.perf_event_paranoid=1" << std::endl;
            exit(EXIT_FAILURE);
        }
        return fd;
    }

public:
    PerfCounter() {
        fd_cycles       = open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
        fd_instructions = open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
        fd_misses       = open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    }

    void start() {
        ioctl(fd_cycles,       PERF_EVENT_IOC_RESET, 0);
        ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd_misses,       PERF_EVENT_IOC_RESET, 0);

        ioctl(fd_cycles,       PERF_EVENT_IOC_ENABLE, 0);
        ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0);
        ioctl(fd_misses,       PERF_EVENT_IOC_ENABLE, 0);
    }

    PerfMetrics stop() {
        ioctl(fd_cycles,       PERF_EVENT_IOC_DISABLE, 0);
        ioctl(fd_instructions, PERF_EVENT_IOC_DISABLE, 0);
        ioctl(fd_misses,       PERF_EVENT_IOC_DISABLE, 0);

        PerfMetrics m = {0, 0, 0};
        
        // Capture the result to satisfy the compiler's warn_unused_result
        auto r1 = read(fd_cycles,       &m.cycles,       sizeof(uint64_t));
        auto r2 = read(fd_instructions, &m.instructions, sizeof(uint64_t));
        auto r3 = read(fd_misses,       &m.cache_misses, sizeof(uint64_t));
        
        // Cast to void to silence the warning
        (void)r1; (void)r2; (void)r3;

        return m;
    }

    ~PerfCounter() {
        close(fd_cycles);
        close(fd_instructions);
        close(fd_misses);
    }
};