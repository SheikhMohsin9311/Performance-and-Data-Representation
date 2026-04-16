#!/usr/bin/env python3
"""
patch_benchmarks.py — Transforms all 16 benchmark .c files in-place.

The transformation per file:
  1. Expand the single warmup pass to 3 passes.
  2. Replace `perf_metrics_t m;` with per-sample collection declarations.
  3. Replace:
       start_counters(fds, 7);
       for (int r = 0; r < runs; r++) { <body> }
       stop_counters(fds, 7, &m);
     with:
       for (int _r = 0; _r < runs; _r++) {
           start_counters(fds, 7);
           <body>
           stop_counters(fds, 7, &_samples[_r]);
       }
  4. Replace `printf("METRICS,...` with stats computation + METRICS2 print.
  5. Inject `free(_samples);` before the fd cleanup loop.

Usage: python3 patch_benchmarks.py [--dry-run]
"""

import re, sys, os, shutil, textwrap

DRY_RUN = "--dry-run" in sys.argv

TARGETS = [
    "aos.c", "array.c", "bst.c", "btree.c", "circular_buffer.c",
    "deque_ds.c", "hash_map.c", "heap.c", "linked_list.c", "rb_tree.c",
    "skip_list.c", "slab_list.c", "soa.c", "trie.c", "veb_tree.c", "vector_ds.c",
]

METRICS2_BLOCK = """\
    compute_perf_stats(_samples, runs, &_stats);
    double _ipc = (_stats.median_cycles > 0)
        ? (double)_stats.median_instructions / _stats.median_cycles : 0.0;
    printf("METRICS2,%lu,%lu,%.2f,%lu,%lu,%lu,%lu,%lu,%.1f\\n",
           _stats.median_cycles, _stats.median_instructions, _ipc,
           _stats.median_cache_misses, _stats.median_branches,
           _stats.median_branch_misses, _stats.median_l1_misses,
           _stats.median_tlb_misses, _stats.cv_pct);
"""

def find_matching_brace(lines, start_idx):
    """Given the index of the line containing '{', find the matching '}'."""
    depth = 0
    for i in range(start_idx, len(lines)):
        depth += lines[i].count('{') - lines[i].count('}')
        if depth == 0:
            return i
    return len(lines) - 1

def patch_source(src: str) -> str:
    lines = src.splitlines(keepends=True)
    out = []
    i = 0

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        # ── 1. Expand warmup ─────────────────────────────────────────────
        if stripped == "// Warm up":
            out.append(line)  # keep the comment
            out.append("    // 3 warmup passes to prime cache and branch predictor\n")
            out.append("    for (int _wu = 0; _wu < 3; _wu++) {\n")
            i += 1
            # collect warmup body until the closing asm fence
            while i < len(lines):
                wl = lines[i]
                if '__asm__' in wl and 'memory' in wl:
                    out.append("    } /* end warmup */\n")
                    out.append(wl)
                    i += 1
                    break
                out.append("    " + wl.lstrip())
                i += 1
            continue

        # ── 2. Replace `perf_metrics_t m;` ──────────────────────────────
        if re.match(r'\s*perf_metrics_t\s+m\s*;', line):
            out.append("    /* Per-iteration sample collection */\n")
            out.append("    perf_sample_t* _samples = (perf_sample_t*)malloc(runs * sizeof(perf_sample_t));\n")
            out.append("    perf_stats_t   _stats;\n")
            i += 1
            continue

        # ── 3. Replace start_counters → for-loop → stop_counters ─────────
        if re.match(r'\s*start_counters\(fds,\s*7\)\s*;', line):
            # peek ahead to find the drowning `for (int r …)` loop
            j = i + 1
            # skip blank / comment lines
            while j < len(lines) and not re.match(r'\s*for\s*\(\s*int\s+r\s*=\s*0', lines[j]):
                j += 1

            if j < len(lines):
                # find the opening brace of the for-loop body
                k = j
                while k < len(lines) and '{' not in lines[k]:
                    k += 1
                close = find_matching_brace(lines, k)

                # extract body between opening and closing brace (exclusive)
                body_lines = lines[k+1:close]

                out.append("    for (int _r = 0; _r < runs; _r++) {\n")
                out.append("        start_counters(fds, 7);\n")
                for bl in body_lines:
                    # re-indent: strip one level (4 spaces) if present, add 8
                    stripped_bl = bl[4:] if bl.startswith("    ") else bl.lstrip()
                    out.append("        " + stripped_bl)
                out.append("        stop_counters(fds, 7, &_samples[_r]);\n")
                out.append("    } /* end per-iteration measurement */\n")

                # advance past everything consumed: start_counters … stop_counters
                i = close + 1
                # skip the original stop_counters line
                while i < len(lines) and re.match(r'\s*stop_counters\(fds,\s*7,\s*&m\)\s*;', lines[i]):
                    i += 1
                continue
            else:
                # fallback: emit as-is
                out.append(line)
                i += 1
                continue

        # ── 4. Replace printf("METRICS, ───────────────────────────────────
        if re.match(r'\s*printf\("METRICS,', line):
            out.append(METRICS2_BLOCK)
            i += 1
            continue

        # ── 5. Inject free(_samples) before fd cleanup ───────────────────
        if re.match(r'\s*for\s*\(int\s+i\s*=\s*0\s*;.*fds.*close', line):
            out.append("    free(_samples);\n")
            out.append(line)
            i += 1
            continue

        out.append(line)
        i += 1

    return "".join(out)

def patch_file(path: str) -> bool:
    with open(path) as f:
        original = f.read()

    patched = patch_source(original)

    if patched == original:
        print(f"  SKIP (no changes): {path}")
        return False

    if DRY_RUN:
        print(f"  DRY-RUN: would patch {path}")
        return True

    shutil.copy2(path, path + ".bak")
    with open(path, "w") as f:
        f.write(patched)
    print(f"  PATCHED: {path}")
    return True


def main():
    base = os.path.dirname(os.path.abspath(__file__))
    n_ok = 0
    for fname in TARGETS:
        fpath = os.path.join(base, fname)
        if not os.path.exists(fpath):
            print(f"  MISSING: {fpath}")
            continue
        if patch_file(fpath):
            n_ok += 1
    print(f"\nDone: {n_ok}/{len(TARGETS)} files patched.")


if __name__ == "__main__":
    main()
