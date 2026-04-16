#!/usr/bin/env python
# coding: utf-8

# # Hardware-Level Performance Analysis of Data Structures
#
# **Research Question**: How do different data structure *representations* affect
# CPU performance across the memory hierarchy as working-set size grows?
#
# **Methodology**: 16 data structures measured across 17 canonical sizes (N = 1 K – 10 M).
# Each data point is the **median** of ≥ 20 independent traversals (hardware counters
# via `perf_event_open`), with the coefficient of variation (CV%) reported as a
# data-quality signal.  Rows with CV% > 40 % or fewer than 50 K median cycles are
# automatically excluded as unreliable.

# ---
# ## Setup

# In[ ]:

import glob
import os
import time
import warnings
warnings.filterwarnings("ignore")

import numpy as np
import pandas as pd
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import matplotlib.gridspec as gridspec
import matplotlib.patches as mpatches
from matplotlib.lines import Line2D
import seaborn as sns
from scipy.stats import gmean

# ── Publication-quality style ────────────────────────────────────────────────
plt.rcParams.update({
    "font.family":        "DejaVu Sans",
    "font.size":          11,
    "axes.titlesize":     13,
    "axes.labelsize":     11,
    "xtick.labelsize":    9,
    "ytick.labelsize":    9,
    "legend.fontsize":    9,
    "legend.framealpha":  0.92,
    "axes.grid":          True,
    "grid.alpha":         0.35,
    "grid.linestyle":     "--",
    "axes.spines.top":    False,
    "axes.spines.right":  False,
    "figure.dpi":         150,
    "savefig.dpi":        300,
    "savefig.bbox":       "tight",
})

# Colour palette: 16 distinct, perceptually uniform colours
DS_PALETTE = [
    "#1f77b4","#ff7f0e","#2ca02c","#d62728","#9467bd",
    "#8c564b","#e377c2","#7f7f7f","#bcbd22","#17becf",
    "#aec7e8","#ffbb78","#98df8a","#ff9896","#c5b0d5",
    "#c49c94",
]

# ── Data-structure category families for grouped colouring ───────────────────
FAMILIES = {
    "Contiguous":  ["Array","Vector","CircularBuffer","Heap","AoS","SoA"],
    "Tree":        ["BST","RBTree","BTree","vEBTree"],
    "Linked":      ["LinkedList","SlabList","SkipList"],
    "Hash / Trie": ["HashMap","Trie"],
    "Deque":       ["Deque"],
}
FAMILY_COLOR = {
    "Contiguous":  "#1f77b4",
    "Tree":        "#d62728",
    "Linked":      "#2ca02c",
    "Hash / Trie": "#9467bd",
    "Deque":       "#ff7f0e",
}

def family_of(ds):
    for fam, members in FAMILIES.items():
        if ds in members:
            return fam
    return "Other"


# ---
# ## 1 – Data Loading

# In[ ]:

# ── Schema versions ───────────────────────────────────────────────────────────
# "METRICS2" schema (current): median_cycles, median_instructions, ipc,
#   median_cache_misses, median_branches, median_branch_misses,
#   median_l1_misses, median_tlb_misses, cv_pct
# Legacy schema (old runperf.sh): cycles, instructions, ipc,
#   cache_misses, branches, branch_misses, L1_misses, TLB_misses

METRICS2_COLS = {
    "median_cycles", "median_instructions", "median_cache_misses",
    "median_l1_misses", "median_tlb_misses", "cv_pct",
}

def _normalise(df: pd.DataFrame) -> pd.DataFrame | None:
    """Rename legacy column names to the METRICS2 schema. Return None to discard."""
    cols = set(df.columns)
    if METRICS2_COLS.issubset(cols):
        return df  # already METRICS2
    # Legacy schema – rename if recognisable
    renames = {}
    if "cycles"       in cols: renames["cycles"]       = "median_cycles"
    if "instructions" in cols: renames["instructions"] = "median_instructions"
    if "ipc"          in cols: pass  # ipc is same
    if "cache_misses" in cols: renames["cache_misses"] = "median_cache_misses"
    if "branches"     in cols: renames["branches"]     = "median_branches"
    if "branch_misses"in cols: renames["branch_misses"]= "median_branch_misses"
    if "L1_misses"    in cols: renames["L1_misses"]    = "median_l1_misses"
    if "TLB_misses"   in cols: renames["TLB_misses"]   = "median_tlb_misses"
    if not renames:
        return None  # unrecognisable schema – discard
    df = df.rename(columns=renames)
    if "cv_pct" not in df.columns:
        df["cv_pct"] = np.nan
    return df


def load_results(
    pattern: str = "deep_scaling_results*.csv",
    min_cycles: int = 50_000,
    max_cv_pct: float = 40.0,
) -> pd.DataFrame:
    """
    Load and aggregate all benchmark CSV files.

    Quality filters applied per row:
      • median_cycles >= min_cycles  (eliminates sub-microsecond noise)
      • cv_pct        <= max_cv_pct  (eliminates high-variance samples)
    After filtering, keep only the row with the smallest cv_pct per
    (data_structure, N) pair so that multiple runs of the same experiment
    converge to the cleanest observation.
    """
    files = sorted(glob.glob(pattern))
    if not files:
        print("❌  No CSV files found. Run `bash runperf.sh` first.")
        return pd.DataFrame()

    frames = []
    for f in files:
        # Skip files actively being written
        if time.time() - os.path.getmtime(f) < 2:
            print(f"  ⏭  Skipping (active): {f}")
            continue
        try:
            raw = pd.read_csv(f)
            for c in raw.columns:
                if c != "data_structure":
                    raw[c] = pd.to_numeric(raw[c], errors="coerce")
            normed = _normalise(raw)
            if normed is None:
                print(f"  ⚠️   Unknown schema, skipping: {f}")
                continue
            frames.append(normed)
        except Exception as ex:
            print(f"  ❌  Error loading {f}: {ex}")

    if not frames:
        print("❌  No valid data frames loaded.")
        return pd.DataFrame()

    df = pd.concat(frames, ignore_index=True)

    # ── Quality filters ───────────────────────────────────────────────────────
    n_raw = len(df)
    df = df[df["median_cycles"] >= min_cycles]
    if "cv_pct" in df.columns and df["cv_pct"].notna().any():
        df = df[df["cv_pct"].isna() | (df["cv_pct"] <= max_cv_pct)]
    n_after = len(df)
    print(f"  Loaded {n_raw} rows → {n_after} after quality filtering "
          f"(min_cycles={min_cycles:,}, max_cv={max_cv_pct}%)")

    # ── Deduplicate: prefer lowest cv_pct per (structure, N) ─────────────────
    df["_sort_key"] = df["cv_pct"].fillna(100)
    df = (df.sort_values("_sort_key")
            .drop_duplicates(subset=["data_structure", "N"])
            .drop(columns=["_sort_key"])
            .reset_index(drop=True))

    # ── Derived metrics ───────────────────────────────────────────────────────
    df["cycles_per_n"]      = df["median_cycles"]      / df["N"]
    df["l1_misses_per_n"]   = df["median_l1_misses"]   / df["N"]
    df["tlb_misses_per_n"]  = df["median_tlb_misses"]  / df["N"]
    df["cache_misses_per_n"]= df["median_cache_misses"]/ df["N"]
    df["bmiss_rate"]        = (df["median_branch_misses"] /
                                df["median_branches"].replace(0, np.nan))
    df["family"]            = df["data_structure"].map(family_of)

    return df


df = load_results()

if df.empty:
    raise SystemExit("No data to plot.")

STRUCTURES = sorted(df["data_structure"].unique())
DS_COLOR   = {ds: DS_PALETTE[i % len(DS_PALETTE)] for i, ds in enumerate(STRUCTURES)}
N_VALS     = sorted(df["N"].unique())
MAX_N      = df["N"].max()

print(f"\n  Data structures : {len(STRUCTURES)}")
print(f"  N range         : {N_VALS[0]:,} – {MAX_N:,}  ({len(N_VALS)} sizes)")
print(f"  Total data rows : {len(df)}")
print(f"\n  Structures:\n    " + ", ".join(STRUCTURES))


# ---
# ## 2 – Cycles/N Scaling  (log–log)
#
# **Interpretation**: slope ≈ 0 → constant cost per element (cache-resident);
# slope > 0 → cost grows with N (memory-bound).  A slope breakpoint marks the
# transition between cache levels.

# In[ ]:

fig, ax = plt.subplots(figsize=(13, 7))

for ds in STRUCTURES:
    sub = df[df["data_structure"] == ds].sort_values("N")
    if sub.empty: continue
    ax.plot(sub["N"], sub["cycles_per_n"],
            marker="o", markersize=4, linewidth=1.7,
            color=DS_COLOR[ds], label=ds)

# Annotate approximate cache boundary lines
for (label, xv, color) in [
    ("L1 ≈ 32 KB",   32_768 // 4,   "#888888"),
    ("L2 ≈ 256 KB",  262_144 // 4,  "#555555"),
    ("L3 ≈ 8 MB",    8_388_608 // 4,"#222222"),
]:
    if xv >= N_VALS[0] and xv <= MAX_N:
        ax.axvline(xv, color=color, linewidth=0.9, linestyle=":", alpha=0.7)
        ax.text(xv * 1.05, ax.get_ylim()[1] if ax.get_ylim()[1] != 1 else 1e3,
                label, fontsize=7.5, color=color, va="top")

ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("Number of Elements N")
ax.set_ylabel("CPU Cycles per Element")
ax.set_title("Figure 1 — Cycles per Element vs. N (log–log)\n"
             "Slope breakpoints reveal cache-level transitions",
             loc="left", fontweight="bold")
ax.legend(ncol=3, bbox_to_anchor=(1.01, 1), loc="upper left", frameon=True)
ax.xaxis.set_major_formatter(mticker.FuncFormatter(
    lambda x, _: f"{int(x):,}" if x < 1e6 else f"{x/1e6:.0f}M"))
plt.tight_layout()
plt.savefig("fig1_cycles_per_n.pdf")
plt.show()


# ---
# ## 3 – IPC Heatmap (all structures × N)
#
# Higher IPC = better pipeline utilisation.  Cache misses stall the pipeline
# and collapse IPC.

# In[ ]:

# Reduce N values to at most 20 log-spaced buckets for a readable heatmap
N_all   = sorted(df["N"].unique())
N_buckets = list(dict.fromkeys(
    [N_all[int(round(i))] for i in np.linspace(0, len(N_all) - 1, min(20, len(N_all)))]
))
df_hm = df[df["N"].isin(N_buckets)].copy()

ipc_pivot = (df_hm.pivot_table(index="data_structure", columns="N",
                               values="ipc", aggfunc="median")
                   .reindex(columns=sorted(N_buckets)))

# Determine whether to annotate (too many cells = skip)
do_annot  = ipc_pivot.shape[0] * ipc_pivot.shape[1] <= 200
cell_h    = max(0.38, 7.0 / max(len(ipc_pivot), 1))
fig, ax   = plt.subplots(figsize=(max(14, len(N_buckets) * 0.75),
                                   max(5, len(ipc_pivot) * cell_h)))

cmap = sns.color_palette("vlag", as_cmap=True)
sns.heatmap(
    ipc_pivot, ax=ax,
    cmap=cmap,
    annot=do_annot, fmt=".2g", annot_kws={"size": 7},
    linewidths=0.4, linecolor="#cccccc",
    cbar_kws={"label": "Instructions Per Cycle (IPC)", "shrink": 0.6},
)
ax.set_xlabel("N (number of elements)")
ax.set_ylabel("")
ax.set_title("Figure 2 — IPC Heatmap (structures × N)\n"
             "Dark red = high IPC (compute-bound)  |  Dark blue = low IPC (memory-stalled)",
             loc="left", fontweight="bold")
col_labels = [f"{int(c):,}" if c < 1e6 else f"{c/1e6:.1f}M" for c in N_buckets]
ax.set_xticks(np.arange(len(col_labels)) + 0.5)
ax.set_xticklabels(col_labels, rotation=45, ha="right")
plt.tight_layout()
plt.savefig("fig2_ipc_heatmap.pdf")
plt.show()


# ---
# ## 4 – L1-Cache Miss Rate vs. N
#
# Shows precisely where each structure's working set escapes L1 (≈ 32 KB).

# In[ ]:

fig, ax = plt.subplots(figsize=(13, 7))

for ds in STRUCTURES:
    sub = df[df["data_structure"] == ds].sort_values("N")
    if sub.empty or sub["l1_misses_per_n"].isna().all(): continue
    ax.plot(sub["N"], sub["l1_misses_per_n"],
            marker="s", markersize=4, linewidth=1.7,
            color=DS_COLOR[ds], label=ds)

ax.axvline(32_768 // 4,  color="#888", linewidth=0.9, linestyle=":", alpha=0.8)
ax.text(32_768 // 4 * 1.05, ax.get_ylim()[1] if ax.get_ylim()[1] != 1 else 0.01,
        "L1 ≈ 32KB", fontsize=7.5, color="#666", va="top")

ax.set_xscale("log")
ax.set_yscale("symlog", linthresh=1e-4)
ax.set_xlabel("Number of Elements N")
ax.set_ylabel("L1-D Cache Misses per Element")
ax.set_title("Figure 3 — L1 Cache-Miss Rate per Element vs. N\n"
             "Sharp rise = working set exceeds L1; contiguous structures stay flat longest",
             loc="left", fontweight="bold")
ax.legend(ncol=3, bbox_to_anchor=(1.01, 1), loc="upper left")
ax.xaxis.set_major_formatter(mticker.FuncFormatter(
    lambda x, _: f"{int(x):,}" if x < 1e6 else f"{x/1e6:.0f}M"))
plt.tight_layout()
plt.savefig("fig3_l1_miss_rate.pdf")
plt.show()


# ---
# ## 5 – TLB Miss Onset
#
# TLB misses appear once the working set exceeds DTLB coverage
# (~32 × 4 KB = 128 KB → N ≈ 32 K for 4-byte elements).

# In[ ]:

tlb_df = df[df["median_tlb_misses"] > 0].copy()

fig, ax = plt.subplots(figsize=(13, 6))

for ds in STRUCTURES:
    sub = df[df["data_structure"] == ds].sort_values("N")
    if sub.empty: continue
    # Draw full line in light grey, then overplot TLB-active portion
    ax.plot(sub["N"], sub["tlb_misses_per_n"],
            linewidth=1.2, alpha=0.25, color=DS_COLOR[ds])
    active = sub[sub["median_tlb_misses"] > 0]
    if not active.empty:
        ax.plot(active["N"], active["tlb_misses_per_n"],
                marker="^", markersize=5, linewidth=1.8,
                color=DS_COLOR[ds], label=ds)

ax.axvline(128_000 // 4, color="#777", linewidth=0.9, linestyle=":",
           label="DTLB limit ≈ 128 KB")

ax.set_xscale("log")
ax.set_yscale("symlog", linthresh=1e-5)
ax.set_xlabel("Number of Elements N")
ax.set_ylabel("TLB Misses per Element")
ax.set_title("Figure 4 — TLB-Miss Rate per Element vs. N\n"
             "Dots appear only once working set overflows DTLB capacity",
             loc="left", fontweight="bold")
ax.legend(ncol=2, bbox_to_anchor=(1.01, 1), loc="upper left")
ax.xaxis.set_major_formatter(mticker.FuncFormatter(
    lambda x, _: f"{int(x):,}" if x < 1e6 else f"{x/1e6:.0f}M"))
plt.tight_layout()
plt.savefig("fig4_tlb_miss_onset.pdf")
plt.show()


# ---
# ## 6 – Memory-Hierarchy Cost Breakdown (at max N)
#
# Stacked bar: cycles attributable to L1 misses, TLB misses, and residual.

# In[ ]:

large = (df[df["N"] == MAX_N]
           .set_index("data_structure")
           .reindex(STRUCTURES)
           .dropna(subset=["median_cycles"]))

# Rough attribution using hardware-counter costs
L1_PENALTY  = 5     # ~5 cycles per L1 miss (L2 hit)
TLB_PENALTY = 20    # ~20 cycles per TLB miss (page-walk)

large["l1_cost"]      = (large["median_l1_misses"]  * L1_PENALTY).clip(lower=0)
large["tlb_cost"]     = (large["median_tlb_misses"] * TLB_PENALTY).clip(lower=0)
large["residual"]     = (large["median_cycles"]
                          - large["l1_cost"]
                          - large["tlb_cost"]).clip(lower=0)

fig, ax = plt.subplots(figsize=(13, 6))
x     = np.arange(len(large))
width = 0.6

p1 = ax.bar(x, large["residual"],   width, color="#4878cf", label="Other / Compute")
p2 = ax.bar(x, large["l1_cost"],    width, bottom=large["residual"],
            color="#d65f5f", label=f"L1-D miss cost (×{L1_PENALTY} cy)")
p3 = ax.bar(x, large["tlb_cost"],   width,
            bottom=large["residual"] + large["l1_cost"],
            color="#ee854a", label=f"TLB miss cost (×{TLB_PENALTY} cy)")

ax.set_xticks(x)
ax.set_xticklabels(large.index, rotation=35, ha="right")
ax.set_ylabel("Estimated Cycles")
ax.set_title(f"Figure 5 — Estimated Cycle-Cost Breakdown at N = {MAX_N:,}\n"
             f"(L1 miss ≈ {L1_PENALTY} cy, TLB miss ≈ {TLB_PENALTY} cy)",
             loc="left", fontweight="bold")
ax.yaxis.set_major_formatter(mticker.FuncFormatter(
    lambda x, _: f"{x/1e6:.0f}M" if x >= 1e6 else f"{x/1e3:.0f}K"))
ax.legend(frameon=True)
plt.tight_layout()
plt.savefig("fig5_cost_breakdown.pdf")
plt.show()


# ---
# ## 7 – Branch Misprediction Rate at Large N

# In[ ]:

large_bm = (df[df["N"] == MAX_N]
              .set_index("data_structure")
              .reindex(STRUCTURES)
              .dropna(subset=["bmiss_rate"]))

fig, ax = plt.subplots(figsize=(11, 5))

colors_bm = [FAMILY_COLOR.get(family_of(ds), "#aaaaaa")
             for ds in large_bm.index]
bars = ax.bar(range(len(large_bm)), large_bm["bmiss_rate"] * 100,
              color=colors_bm, edgecolor="white", linewidth=0.5, width=0.65)

ax.axhline(1.0, color="#333", linewidth=0.8, linestyle="--",
           label="1% misprediction rate")
ax.set_xticks(range(len(large_bm)))
ax.set_xticklabels(large_bm.index, rotation=35, ha="right")
ax.set_ylabel("Branch Misprediction Rate (%)")
ax.set_title(f"Figure 6 — Branch Misprediction Rate at N = {MAX_N:,}\n"
             "Pointer-chasing structures have irregular control flow → higher misprediction",
             loc="left", fontweight="bold")

# Family legend
patches = [mpatches.Patch(color=c, label=f) for f, c in FAMILY_COLOR.items()]
ax.legend(handles=patches, frameon=True, loc="upper right")
plt.tight_layout()
plt.savefig("fig6_branch_mispred.pdf")
plt.show()


# ---
# ## 8 – Radar / Spider Chart: Multi-Metric Comparison at Large N
#
# Normalised (0 = best, 1 = worst) across 5 metrics.  Smaller area = better overall.

# In[ ]:

RADAR_METRICS = {
    "Cycles/N":     "cycles_per_n",
    "L1 Misses/N":  "l1_misses_per_n",
    "TLB Misses/N": "tlb_misses_per_n",
    "Bmiss Rate":   "bmiss_rate",
    "IPC (inv.)":   "ipc",        # inverted: lower IPC = worse
}

at_max = (df[df["N"] == MAX_N]
            .set_index("data_structure")
            [list(RADAR_METRICS.values())]
            .dropna(how="all"))

# Normalise 0-1 (higher = worse for all, so invert IPC)
norm = at_max.copy()
for col in norm.columns:
    rng = norm[col].max() - norm[col].min()
    if col == "ipc":
        norm[col] = 1 - (norm[col] - norm[col].min()) / (rng if rng else 1)
    else:
        norm[col] = (norm[col] - norm[col].min()) / (rng if rng else 1)
norm = norm.fillna(0)

labels  = list(RADAR_METRICS.keys())
N_spoke = len(labels)
angles  = np.linspace(0, 2 * np.pi, N_spoke, endpoint=False).tolist()
angles += angles[:1]

# Only plot up to 16 structures on the radar (all of them)
fig, ax = plt.subplots(subplot_kw={"projection": "polar"}, figsize=(10, 10))

for ds in norm.index:
    values = norm.loc[ds].tolist() + [norm.loc[ds].tolist()[0]]
    ax.plot(angles, values, linewidth=1.4, label=ds, color=DS_COLOR.get(ds, "#555"))
    ax.fill(angles, values, alpha=0.06, color=DS_COLOR.get(ds, "#555"))

ax.set_thetagrids(np.degrees(angles[:-1]), labels, fontsize=10)
ax.set_ylim(0, 1)
ax.set_yticks([0.25, 0.5, 0.75, 1.0])
ax.set_yticklabels(["25%", "50%", "75%", "100%"], fontsize=7)
ax.set_title(f"Figure 7 — Multi-Metric Radar at N = {MAX_N:,}\n"
             "Smaller area = more efficient  |  All axes: 0 = best, 1 = worst",
             fontweight="bold", pad=25)
ax.legend(loc="upper right", bbox_to_anchor=(1.45, 1.15), ncol=1, frameon=True)
plt.tight_layout()
plt.savefig("fig7_radar.pdf")
plt.show()


# ---
# ## 9 – Measurement Quality: CV% per Structure at Large N
#
# Green < 5%  |  Orange 5–15%  |  Red > 15%

# In[ ]:

if "cv_pct" in df.columns and df["cv_pct"].notna().any():
    cv_at_max = (df[df["N"] == MAX_N]
                   .set_index("data_structure")["cv_pct"]
                   .reindex(STRUCTURES)
                   .dropna())

    fig, ax = plt.subplots(figsize=(11, 4.5))
    bar_colors = [
        "seagreen" if v < 5 else ("darkorange" if v < 15 else "crimson")
        for v in cv_at_max
    ]
    ax.bar(range(len(cv_at_max)), cv_at_max, color=bar_colors,
           edgecolor="white", linewidth=0.5, width=0.65)
    ax.axhline(5,  color="seagreen",  linestyle="--", linewidth=0.9,
               label="5% — excellent")
    ax.axhline(15, color="darkorange",linestyle="--", linewidth=0.9,
               label="15% — borderline")
    ax.set_xticks(range(len(cv_at_max)))
    ax.set_xticklabels(cv_at_max.index, rotation=35, ha="right")
    ax.set_ylabel("Coefficient of Variation (CV%)")
    ax.set_title(f"Figure 8 — Measurement Noise (CV%) at N = {MAX_N:,}\n"
                 "Reflects intra-experiment variability; run `sudo bash stabilize.sh` to reduce",
                 loc="left", fontweight="bold")
    ax.legend(frameon=True)
    plt.tight_layout()
    plt.savefig("fig8_cv_pct.pdf")
    plt.show()
else:
    print("cv_pct column unavailable — skipping Figure 8.")


# ---
# ## 10 – Summary Ranking Table
#
# Structures ranked by geometric mean of normalised cost across ALL N values
# (lower total normalised cost = better overall).

# In[ ]:

# Compute per-row normalised score for cycles_per_n and l1_misses_per_n
rank_cols = ["cycles_per_n", "l1_misses_per_n", "tlb_misses_per_n", "bmiss_rate"]
ranked    = df.groupby("data_structure")[rank_cols].median()

# Normalise each column to [0, 1]
for col in rank_cols:
    rng = ranked[col].max() - ranked[col].min()
    ranked[col] = (ranked[col] - ranked[col].min()) / (rng if rng else 1)

ranked["Overall Score (lower = better)"] = ranked[rank_cols].mean(axis=1)
ranked = ranked.sort_values("Overall Score (lower = better)")
ranked.index.name = "Data Structure"

summary = ranked.copy()
summary.columns = ["Cycles/N (norm.)", "L1 Miss/N (norm.)",
                   "TLB Miss/N (norm.)", "Branch Mispred (norm.)",
                   "Overall Score"]

fig, ax = plt.subplots(figsize=(13, max(4, len(summary) * 0.45 + 1)))
ax.axis("off")

table_data  = [[ds] + [f"{v:.3f}" for v in row]
               for ds, row in summary.iterrows()]
col_labels  = ["Data Structure"] + list(summary.columns)
tbl = ax.table(cellText=table_data, colLabels=col_labels,
               loc="center", cellLoc="center")
tbl.auto_set_font_size(False)
tbl.set_fontsize(9)
tbl.auto_set_column_width(col=list(range(len(col_labels))))

# Colour header row
for j in range(len(col_labels)):
    tbl[(0, j)].set_facecolor("#2c3e50")
    tbl[(0, j)].set_text_props(color="white", fontweight="bold")

# Colour data rows by rank (green → red)
n_rows = len(table_data)
cmap_rank = plt.get_cmap("RdYlGn_r")
for i, row_data in enumerate(table_data):
    norm_rank = i / max(n_rows - 1, 1)
    rgba      = cmap_rank(norm_rank)
    bg        = mpl.colors.to_hex((rgba[0], rgba[1], rgba[2], 0.18))
    for j in range(len(col_labels)):
        tbl[(i + 1, j)].set_facecolor(bg)

ax.set_title("Figure 9 — Overall Performance Ranking\n"
             "(Normalised median score across all N; lower = better)",
             fontweight="bold", pad=12, fontsize=12)
plt.tight_layout()
plt.savefig("fig9_ranking_table.pdf")
plt.show()

print("\n✅  All figures rendered.")
print("   PDF exports saved alongside this notebook.")
