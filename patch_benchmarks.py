import os
import re

c_files = [f for f in os.listdir('.') if f.endswith('.c')]

patch_code = """
    int fds[7];
    fds[0] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    fds[1] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    fds[2] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    fds[3] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS);
    fds[4] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES);
    fds[5] = open_perf_counter(PERF_TYPE_HW_CACHE, (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)));
    fds[6] = open_perf_counter(PERF_TYPE_HW_CACHE, (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)));

    __asm__ __volatile__("" ::: "memory");
"""

for f in c_files:
    with open(f, 'r') as file:
        content = file.read()
    
    # 1. Update the counter opening block
    content = re.sub(r'int fd_c = open_perf_counter\(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES\);\s+int fd_i = open_perf_counter\(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS\);\s+int fd_m = open_perf_counter\(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES\);', patch_code, content)
    
    # 2. Update start_counters call
    content = content.replace('start_counters(fd_c, fd_i, fd_m);', 'start_counters(fds, 7);')
    
    # 3. Update stop_counters call
    content = content.replace('stop_counters(fd_c, fd_i, fd_m, &m);', 'stop_counters(fds, 7, &m);')
    
    # 4. Update the printf line
    content = content.replace('printf("METRICS,%lu,%lu,%lu\\n", m.cycles, m.instructions, m.cache_misses);', 
                               'printf("METRICS,%lu,%lu,%lu,%lu,%lu,%lu,%lu\\n", m.cycles, m.instructions, m.cache_misses, m.branches, m.branch_misses, m.l1_misses, m.tlb_misses);')
    
    # 5. Update close calls
    content = re.sub(r'close\(fd_c\);\s+close\(fd_i\);\s+close\(fd_m\);', 
                     'for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);', content)

    with open(f, 'w') as file:
        file.write(content)
