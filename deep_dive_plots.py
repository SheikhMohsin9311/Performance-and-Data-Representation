import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import matplotlib.ticker as mticker
import warnings

warnings.filterwarnings("ignore")

# --- Technical Styles ---
sns.set_theme(style="whitegrid")
plt.rcParams.update({
    'font.family': 'serif',
    'font.size': 11,
    'axes.titlesize': 14,
    'axes.labelsize': 12,
    'legend.fontsize': 10,
    'figure.dpi': 300,
    'savefig.format': 'pdf',
    'axes.grid': True,
    'grid.alpha': 0.3,
})
# Add PNG support
DPI = 300
FORMATS = ['pdf', 'png']

COLORS = {
    'Vector': '#005f73', 'Array': '#0a9396', 'Deque': '#94d2bd', 
    'BST': '#ae2012', 'RBTree': '#bb3e03', 'BTree': '#ee9b00', 'SkipList': '#9b2226', 'vEBTree': '#ca6702',
    'LinkedList': '#9b2226', 'SlabList': '#d00000', 'Heap': '#001219',
    'AoS': '#3fb950', 'SoA': '#56d364',
    'HashMap': '#1b4332', 'Trie': '#2d6a4f', 'CircularBuffer': '#388bfd'
}

os.makedirs("deep_dive_figures", exist_ok=True)

def load_and_sanitize(path="master.csv"):
    df = pd.read_csv(path)
    # Global Numeric Coercion
    metric_cols = [
        "N", "runs", "median_cycles", "median_instructions", "ipc",
        "median_cache_misses", "median_branches", "median_branch_misses",
        "median_l1_misses", "median_tlb_misses", "cv_pct", "stddev_cycles", "ci95_cycles"
    ]
    for col in metric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')
    
    # Filter: CV < 15% and N >= 1000
    df = df[(df["cv_pct"] < 15) & (df["N"] >= 1000)].dropna(subset=["N", "median_cycles"])
    
    # Inferred metrics
    df["cycles_per_n"] = df["median_cycles"] / df["N"]
    df["ipc"] = df["median_instructions"] / df["median_cycles"]
    df["l1_miss_rate"] = df["median_l1_misses"] / df["N"]
    df["llc_miss_rate"] = df["median_cache_misses"] / df["N"]
    df["tlb_miss_rate"] = df["median_tlb_misses"] / df["N"]
    df["branch_miss_rate"] = df["median_branch_misses"] / df["median_branches"]
    
    # Families
    contiguous = {"Array", "Vector", "CircularBuffer", "Heap", "AoS", "SoA"}
    df["is_contiguous"] = df["data_structure"].apply(lambda x: "Contiguous" if x in contiguous else "Pointer-based")
    
    return df

def plot_insight_a(df):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df, x="N", y="cycles_per_n", hue="data_structure", palette=COLORS, lw=2)
    plt.xscale("log")
    plt.yscale("log")
    plt.title("Scaling Efficiency: The Absolute Latency Frontier")
    plt.ylabel("Cycles per Element (Normalized)")
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_a_scaling.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

def plot_insight_b(df):
    plt.figure(figsize=(10, 6))
    subset = df[df["data_structure"].isin(["Vector", "LinkedList", "BTree", "vEBTree", "HashMap", "BST"])]
    sns.scatterplot(data=subset, x="llc_miss_rate", y="cycles_per_n", hue="data_structure", palette=COLORS, alpha=0.6)
    plt.title("The Cost of a Miss: Latency vs. LLC Pressure")
    plt.xlabel("LLC Misses per Element")
    plt.ylabel("Cycles per Element")
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_b_cache_wall.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

def plot_insight_c(df):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df, x="N", y="ipc", hue="data_structure", palette=COLORS, alpha=0.5)
    plt.xscale("log")
    plt.title("Instruction Throughput (IPC) Collapse across Scales")
    plt.ylabel("Instructions per Cycle")
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_c_ipc_collapse.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

def plot_insight_d(df):
    plt.figure(figsize=(8, 6))
    sns.boxplot(data=df, x="is_contiguous", y="cycles_per_n", palette="Set2")
    plt.yscale("log")
    plt.title("The Structural Tax: Contiguous vs. Pointer-based Latency")
    plt.ylabel("Cycles per Element (Log Scale)")
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_d_structural_tax.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

def plot_insight_e(df):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df, x="N", y="tlb_miss_rate", hue="data_structure", palette=COLORS, alpha=0.7)
    plt.xscale("log")
    plt.title("TLB Exhaustion: Translation Overhead at Massive Scales")
    plt.ylabel("TLB Misses per Element")
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_e_tlb_wall.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

def plot_insight_f(df):
    high_n_df = df[df["N"] >= 10000000]
    if high_n_df.empty:
        print("Skipping Insight F: No data for N >= 10M")
        return
    high_n = high_n_df.groupby("data_structure")["cycles_per_n"].mean().sort_values()
    plt.figure(figsize=(10, 8))
    high_n.plot(kind='barh', color=[COLORS.get(x, '#8b949e') for x in high_n.index])
    plt.title("High-N Efficiency Leaderboard (N >= 10M)")
    plt.xlabel("Average Cycles per Element")
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_f_ranking.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

# --- Advanced Figures for 10-page Paper ---

def plot_insight_g(df):
    """Branch Prediction Landscape"""
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df, x="N", y="branch_miss_rate", hue="data_structure", palette=COLORS, alpha=0.5)
    plt.xscale("log")
    plt.title("Predictability Matrix: Branch Misprediction Trends")
    plt.ylabel("Misprediction Rate")
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_g_branch_mispredict.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

def plot_insight_h(df):
    """Instruction Efficiency Pareto"""
    plt.figure(figsize=(10, 6))
    sns.scatterplot(data=df, x="cycles_per_n", y="ipc", hue="is_contiguous", alpha=0.4)
    plt.xscale("log")
    plt.title("IPC vs Latency: The Efficiency Pareto Frontier")
    plt.xlabel("Cycles per Element")
    plt.ylabel("Instructions Per Cycle")
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_h_pareto.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

def plot_insight_i(df):
    """Measurement Stability Distribution"""
    plt.figure(figsize=(10, 6))
    sns.violinplot(data=df, x="data_structure", y="cv_pct", inner="quart", palette=COLORS)
    plt.xticks(rotation=45)
    plt.title("Experimental Stability: Variance (CV%) Distribution")
    plt.ylabel("Coefficient of Variation (%)")
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_i_stability.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

def plot_insight_j(df):
    """Architectural Correlation Heatmap"""
    corr = df[["N", "cycles_per_n", "ipc", "l1_miss_rate", "llc_miss_rate", "tlb_miss_rate", "branch_miss_rate"]].corr()
    plt.figure(figsize=(10, 8))
    sns.heatmap(corr, annot=True, cmap='coolwarm', fmt=".2f")
    plt.title("Hardware Performance Correlation Matrix")
    plt.tight_layout()
    for fmt in FORMATS:
        plt.savefig(f"deep_dive_figures/insight_j_correlation.{fmt}", dpi=DPI if fmt == 'png' else None)
    plt.close()

if __name__ == "__main__":
    df = load_and_sanitize()
    print(f"Analyzing {len(df)} sanitized samples...")
    plot_insight_a(df)
    plot_insight_b(df)
    plot_insight_c(df)
    plot_insight_d(df)
    plot_insight_e(df)
    plot_insight_f(df)
    plot_insight_g(df)
    plot_insight_h(df)
    plot_insight_i(df)
    plot_insight_j(df)
    print("PDF figures generated in deep_dive_figures/")
