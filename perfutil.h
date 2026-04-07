#pragma once
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

struct PerfCounter {
    int fd;

    PerfCounter() {
        perf_event_attr pe;
        memset(&pe, 0, sizeof(pe));
        pe.type           = PERF_TYPE_HARDWARE;
        pe.config         = PERF_COUNT_HW_CPU_CYCLES;
        pe.disabled       = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv     = 1;
        fd = syscall(SYS_perf_event_open, &pe, 0, -1, -1, 0);
    }

    void start() {
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }

    uint64_t stop() {
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        uint64_t count = 0;
        read(fd, &count, sizeof(count));
        return count;
    }

    ~PerfCounter() { close(fd); }
};