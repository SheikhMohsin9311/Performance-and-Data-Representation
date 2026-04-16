#!/usr/bin/env python3
"""
simplified_plots.py — Generates 3 student-friendly figures for the presentation.
Outputs:
  slides/fig_fastest.png    — bar chart: who's fastest at large N?
  slides/fig_scaling.png    — line chart: how speed grows with N
  slides/fig_why_arrays.png — L1 miss rate: contiguous vs pointer-based
"""

import os, glob, time, warnings
warnings.filterwarnings("ignore")

import numpy as np
import pandas as pd
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import matplotlib.patches as mpatches
import matplotlib.patheffects as path_effects

os.makedirs("slides", exist_ok=True)

# ── Style: The Manuscript (Elite Academic) ───────────────────────────────────
plt.rcParams.update({
    "font.family":        "EB Garamond",
    "font.size":          16,
    "axes.titlesize":     24,
    "axes.labelsize":     20,
    "xtick.labelsize":    16,
    "ytick.labelsize":    16,
    "legend.fontsize":    14,
    "axes.facecolor":     "#0a0a0a",
    "figure.facecolor":   "#0a0a0a",
    "axes.edgecolor":     "#555555",
    "axes.labelcolor":    "#ffffff",
    "xtick.color":        "#ffffff",
    "ytick.color":        "#ffffff",
    "grid.color":         "#222222",
    "grid.alpha":         0.5,
    "grid.linestyle":     "-",
    "text.color":         "#f8f8f8",
    "axes.spines.top":    False,
    "axes.spines.right":  False,
    "figure.dpi":         200,
    "savefig.dpi":        300,
    "savefig.transparent":False, # Match background exactly
    "savefig.facecolor":  "#0a0a0a",
})

# ── Classical Academic Palette ──────────────────────────────────────────────
# Muted, sophisticated colors (Silver, Slate Blue, Sage, Dusty Rose, Sand)
FAMILY_COLOR = {
    "Contiguous":  "#94a3b8",   # Slate Blue/Silver (High Fidelity)
    "Tree":        "#5eead4",   # Tealg/Sage
    "Linked":      "#e2e8f0",   # Off-white/Ghost (The "old" structures)
    "Hash / Trie": "#cbd5e1",   # Light Slate
    "Deque":       "#475569",   # Dark Slate
}
FAMILIES = {
    "Contiguous":  {"Array","Vector","CircularBuffer","Heap","AoS","SoA"},
    "Tree":        {"BST","RBTree","BTree","vEBTree"},
    "Linked":      {"LinkedList","SlabList","SkipList"},
    "Hash / Trie": {"HashMap","Trie"},
    "Deque":       {"Deque"},
}
def family_of(ds):
    for f, m in FAMILIES.items():
        if ds in m: return f
    return "Other"

# ── Load data (Strict: Version 2 only) ────────────────────────────────────────
METRICS2_COLS = {"median_cycles","median_instructions","median_cache_misses",
                 "median_l1_misses","median_tlb_misses","cv_pct"}

def _is_valid_metrics2(df):
    """Ensure the file has the new-style statistical columns."""
    return METRICS2_COLS.issubset(set(df.columns))

frames = []
for f in sorted(glob.glob("deep_scaling_results*.csv")):
    # Skip files being actively written (checked via mtime vs current)
    if time.time() - os.path.getmtime(f) < 2: continue
    try:
        raw = pd.read_csv(f)
        if _is_valid_metrics2(raw):
            frames.append(raw)
    except Exception: pass

if not frames:
    print("❌ Error: No valid METRICS2 data found. Run benchmarks first.")
    exit(1)

df = pd.concat(frames, ignore_index=True)

# Data Quality Filters
df = df[df["median_cycles"] >= 50_000] # Drop noise-only runs
df = df[df["cv_pct"].isna() | (df["cv_pct"] <= 40)] # Drop jittery runs
df["_cv"] = df["cv_pct"].fillna(100)
df = (df.sort_values("_cv")
        .drop_duplicates(subset=["data_structure","N"])
        .drop(columns=["_cv"]).reset_index(drop=True))

# Inferred Metrics
df["cycles_per_n"]    = df["median_cycles"]    / df["N"]
df["l1_misses_per_n"] = df["median_l1_misses"] / df["N"]
df["family"]          = df["data_structure"].map(family_of)

# ── Global Statistics for Presentation ───────────────────────────────────────
total_data_points = len(df)
total_structures  = df["data_structure"].nunique()
max_n             = df["N"].max()
min_n             = df["N"].min()
# Estimate total workload processed (approx N * runs for all data points)
# If runs isn't available, we fallback.
total_elements = (df["N"] * df.get("runs", 1)).sum()

with open("slides/stats_research.txt", "w") as f_out:
    f_out.write(f"Datapoints: {total_data_points:,}\n")
    f_out.write(f"Structures: {total_structures}\n")
    f_out.write(f"N Range: {min_n:,} to {max_n:,}\n")
    f_out.write(f"Processed: {total_elements/1e9:.1f} Billion Elements\n")

print(f"✅ Loaded {len(df)} research-grade samples.")

MAX_N = df["N"].max()
print(f"Loaded {len(df)} rows, largest N = {MAX_N:,}")


# ═══════════════════════════════════════════════════════════════════════════════
# FIGURE A — "Who's Fastest?" (horizontal bar chart at large N)
# ═══════════════════════════════════════════════════════════════════════════════
at_max = (df[df["N"] == MAX_N]
           .groupby("data_structure")["cycles_per_n"]
           .median().sort_values())

fig, ax = plt.subplots(figsize=(11, 7))

bar_colors = [FAMILY_COLOR.get(family_of(ds), "#aaa") for ds in at_max.index]
bars = ax.barh(range(len(at_max)), at_max.values, color=bar_colors,
               edgecolor="white", linewidth=0.5, height=0.7)

# Annotate value at end of each bar
import matplotlib.patheffects as path_effects
pe = [path_effects.withStroke(linewidth=2, foreground='#0a0a0a')]

for i, (val, bar) in enumerate(zip(at_max.values, bars)):
    label = f"{val:.1f}" if val < 100 else f"{val:.0f}"
    ax.text(val + at_max.max() * 0.01, i, label + " cycles/elem",
            va="center", fontsize=15, fontweight="bold", color="#ffffff",
            path_effects=pe)

ax.set_yticks(range(len(at_max)))
ax.set_yticklabels(at_max.index, fontsize=13)
ax.set_xlabel(f"CPU Cycles per Element  (lower = faster,  N = {MAX_N:,})")
ax.set_title("Which data structure is fastest?", fontweight="bold", pad=14)
ax.set_xlim(0, at_max.max() * 1.25)
ax.grid(axis="y", alpha=0)

# Legend
patches = [mpatches.Patch(color=c, label=f) for f, c in FAMILY_COLOR.items()]
ax.legend(handles=patches, title="Type", loc="lower right", frameon=True)

plt.tight_layout()
plt.savefig("slides/fig_fastest.png")
plt.close()
print("✓  slides/fig_fastest.png")


# ═══════════════════════════════════════════════════════════════════════════════
# FIGURE B — "Does speed change as data grows?" (line chart, 5 structures)
# ═══════════════════════════════════════════════════════════════════════════════
SPOTLIGHT = ["Array", "LinkedList", "HashMap", "BST", "BTree"]
SPOT_COLORS= {
    "Array":      "#2196F3",
    "LinkedList": "#4CAF50",
    "HashMap":    "#9C27B0",
    "BST":        "#F44336",
    "BTree":      "#FF9800",
}
available_spot = [s for s in SPOTLIGHT if s in df["data_structure"].unique()]

fig, ax = plt.subplots(figsize=(12, 6))

for ds in available_spot:
    sub = df[df["data_structure"] == ds].sort_values("N")
    ax.plot(sub["N"], sub["cycles_per_n"],
            marker="o", markersize=6, linewidth=2.5,
            color=SPOT_COLORS[ds], label=ds)

# Subtle cache boundary annotations
for label, val, col in [("L1 hits\n(≤32 KB)", 32_768//4, "#aaa"),
                         ("L2 range", 262_144//4, "#aaa"),
                         ("RAM needed", 8_388_608//4, "#aaa")]:
    xv = val
    if df["N"].min() < xv < df["N"].max():
        ax.axvline(xv, color=col, linewidth=1.2, linestyle=":")
        ax.text(xv * 1.06, ax.get_ylim()[1] * 0.9 if ax.get_ylim()[1] > 1 else 1,
                label, fontsize=9, color="#888", va="top", ha="left")

ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("Number of elements stored  (N)")
ax.set_ylabel("CPU cycles per element\n(lower = faster)")
ax.set_title("How does speed change as data grows?", fontweight="bold", pad=14)
ax.legend(frameon=True, loc="upper left")
ax.xaxis.set_major_formatter(mticker.FuncFormatter(
    lambda x, _: f"{int(x):,}" if x < 1e6 else f"{x/1e6:.0f}M"))
plt.tight_layout()
plt.savefig("slides/fig_scaling.png")
plt.close()
print("✓  slides/fig_scaling.png")


# ═══════════════════════════════════════════════════════════════════════════════
# FIGURE C — "Why are arrays faster? The Memory Fast Lane"
# Average L1 miss rate: contiguous vs pointer-chasing groups, at large-ish N
# ═══════════════════════════════════════════════════════════════════════════════
CONTIGUOUS = {"Array","Vector","CircularBuffer","Heap","AoS","SoA"}
POINTER    = {"LinkedList","BST","RBTree","SkipList","SlabList","HashMap"}

# Use the largest N that all structures have data for
shared_n = (df.groupby("data_structure")["N"].max()
              .reindex(list(CONTIGUOUS | POINTER))
              .dropna())
# Pick a large N present in the data — last decile
target_n = df[df["N"] >= df["N"].quantile(0.80)]["N"].min()

sub_c = df[(df["data_structure"].isin(CONTIGUOUS)) & (df["N"] == target_n)]["l1_misses_per_n"]
sub_p = df[(df["data_structure"].isin(POINTER))    & (df["N"] == target_n)]["l1_misses_per_n"]

mean_c = sub_c.mean() if not sub_c.empty else 0
mean_p = sub_p.mean() if not sub_p.empty else 1

fig, ax = plt.subplots(figsize=(8, 6))

groups = ["Arrays\n(contiguous)", "Linked / Tree\n(pointer-based)"]
values = [mean_c, mean_p]
colors = [FAMILY_COLOR["Contiguous"], FAMILY_COLOR["Linked"]]

bar_objs = ax.bar(groups, values, color=colors, width=0.45,
                  edgecolor="white", linewidth=0.5)

# Annotate bars
for bar, val in zip(bar_objs, values):
    ax.text(bar.get_x() + bar.get_width() / 2,
            val + max(values) * 0.02,
            f"{val:.3f} misses/elem", ha="center", va="bottom",
            fontsize=16, fontweight="bold", color='#ffffff',
            path_effects=pe)

# Ratio callout
if mean_c > 0:
    ratio = mean_p / mean_c
    ax.annotate(
        f"{ratio:.0f}× more\ncache misses",
        xy=(1, mean_p), xytext=(0.55, mean_p * 0.7),
        fontsize=14, color="#c0392b", fontweight="bold",
        arrowprops=dict(arrowstyle="->", color="#c0392b", lw=1.5),
    )

ax.set_ylabel(f"L1 Cache Misses per Element  (N = {target_n:,})")
ax.set_title("Why are arrays faster?\nThey use the CPU's memory cache better",
             fontweight="bold", pad=14)
ax.set_ylim(0, max(values) * 1.35)
ax.yaxis.set_major_formatter(mticker.FormatStrFormatter("%.3f"))
plt.tight_layout()
plt.savefig("slides/fig_why_arrays.png")
plt.close()
print("✓  slides/fig_why_arrays.png")


# ═══════════════════════════════════════════════════════════════════════════════
# FIGURE D — Scatter: "More cache misses = slower" (at large N)
# X = L1 misses per element, Y = cycles per element, one dot per structure
# Size of dot = proportional to TLB misses (an extra dimension for free)
# ═══════════════════════════════════════════════════════════════════════════════
scat = (df[df["N"] == MAX_N]
          .groupby("data_structure")[["cycles_per_n","l1_misses_per_n","median_tlb_misses"]]
          .median().dropna())
scat["family"] = scat.index.map(family_of)
scat["tlb_size"] = (scat["median_tlb_misses"].fillna(0)
                      .clip(lower=1).apply(np.log1p) * 60 + 120)

fig, ax = plt.subplots(figsize=(10, 7))

for fam, grp in scat.groupby("family"):
    sc = ax.scatter(grp["l1_misses_per_n"], grp["cycles_per_n"],
                    s=grp["tlb_size"],
                    color=FAMILY_COLOR.get(fam, "#aaa"),
                    alpha=0.88, edgecolors="white", linewidth=1.2,
                    label=fam, zorder=3)

# Label each point
for ds, row in scat.iterrows():
    ax.annotate(ds,
                xy=(row["l1_misses_per_n"], row["cycles_per_n"]),
                xytext=(6, 3), textcoords="offset points",
                fontsize=9.5, color="#333")

# Trend line (all points)
xv, yv = scat["l1_misses_per_n"], scat["cycles_per_n"]
if len(xv) >= 3:
    coef = np.polyfit(np.log1p(xv), np.log1p(yv), 1)
    x_line = np.linspace(xv.min(), xv.max(), 200)
    y_line = np.expm1(np.poly1d(coef)(np.log1p(x_line)))
    ax.plot(x_line, y_line, color="#999", linewidth=1.5,
            linestyle="--", label="Trend", zorder=2)

ax.set_xlabel("L1 Cache Misses per Element  (more = worse memory use)")
ax.set_ylabel(f"CPU Cycles per Element  (lower = faster,  N = {MAX_N:,})")
ax.set_title("More cache misses = more cycles\n"
             "Dot size reflects TLB misses (page-table pressure)",
             fontweight="bold", pad=14)
ax.set_xscale("symlog", linthresh=1e-3)
ax.set_yscale("log")
ax.legend(frameon=True, loc="upper left")
ax.xaxis.set_major_formatter(mticker.FuncFormatter(
    lambda x, _: f"{x:.3f}" if abs(x) < 0.1 else f"{x:.1f}"))
plt.tight_layout()
plt.savefig("slides/fig_scatter_misses_vs_speed.png")
plt.close()
print("✓  slides/fig_scatter_misses_vs_speed.png")


# ═══════════════════════════════════════════════════════════════════════════════
# FIGURE E — Scatter: "Speed across ALL sizes" (bubble scatter, all N)
# X = N, Y = cycles/N, colour by family, size by L1-miss rate
# Shows the full performance landscape at a glance
# ═══════════════════════════════════════════════════════════════════════════════
# Thin to every-other point per structure to keep it readable
df_s = df.sort_values("N")
df_thin = (df_s.groupby("data_structure", group_keys=False)
               .apply(lambda g: g.iloc[::2]))

fig, ax = plt.subplots(figsize=(12, 7))

for fam, grp in df_thin.groupby("family"):
    sizes = np.clip(grp["l1_misses_per_n"].fillna(0) * 4000 + 20, 15, 250)
    ax.scatter(grp["N"], grp["cycles_per_n"],
               s=sizes,
               color=FAMILY_COLOR.get(fam, "#aaa"),
               alpha=0.55, edgecolors="none",
               label=fam, zorder=3)

# Cache boundary lines
for (lbl, xv) in [("L1\n32KB", 32_768//4), ("L2\n256KB", 262_144//4),
                   ("L3\n8MB", 8_388_608//4)]:
    if df["N"].min() < xv < df["N"].max():
        ax.axvline(xv, color="#bbb", linewidth=1.1, linestyle=":")
        ax.text(xv * 1.04, ax.get_ylim()[1] * 0.98 if ax.get_ylim()[1] > 1 else 1,
                lbl, fontsize=8.5, color="#999", va="top")

ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("Number of elements  N")
ax.set_ylabel("CPU cycles per element  (lower = faster)")
ax.set_title("Performance landscape across all sizes\n"
             "Bubble size = L1 cache-miss rate (bigger = worse cache use)",
             fontweight="bold", pad=14)
ax.legend(frameon=True, loc="upper left", markerscale=1.4)
ax.xaxis.set_major_formatter(mticker.FuncFormatter(
    lambda x, _: f"{int(x):,}" if x < 1e6 else f"{x/1e6:.0f}M"))
plt.tight_layout()
plt.savefig("slides/fig_scatter_landscape.png")
plt.close()
print("✓  slides/fig_scatter_landscape.png")


print("\n✅  All 5 figures saved to slides/")
