import pandas as pd
import numpy as np

def identify_problems(path="master.csv"):
    df = pd.read_csv(path)
    metric_cols = [
        "N", "median_cycles", "median_instructions", "median_cache_misses", "median_tlb_misses", "cv_pct"
    ]
    for col in metric_cols:
        df[col] = pd.to_numeric(df[col], errors='coerce')
    
    df = df[(df["cv_pct"] < 15) & (df["N"] >= 1000)].dropna()
    
    df["cycles_per_n"] = df["median_cycles"] / df["N"]
    df["llc_miss_rate"] = df["median_cache_misses"] / df["N"]
    df["tlb_miss_rate"] = df["median_tlb_misses"] / df["N"]
    df["ipc"] = df["median_instructions"] / df["median_cycles"]
    
    print("--- 1. Hardware Bottleneck Correlations ---")
    corr = df[["cycles_per_n", "llc_miss_rate", "tlb_miss_rate", "ipc"]].corr()["cycles_per_n"]
    print(corr.sort_values(ascending=False))
    
    print("\n--- 2. The Structural Tax (Pointer vs. Contiguous) ---")
    contiguous = {"Array", "Vector", "CircularBuffer", "Heap", "AoS", "SoA"}
    df["is_contiguous"] = df["data_structure"].apply(lambda x: "Contiguous" if x in contiguous else "Pointer-based")
    avg_perf = df.groupby("is_contiguous")["cycles_per_n"].median()
    print(avg_perf)
    print(f"Multiplier: {avg_perf['Pointer-based'] / avg_perf['Contiguous']:.2f}x")
    
    print("\n--- 3. Top Performers vs. Worst Performers (at High N) ---")
    high_n = df[df["N"] >= 1000000].groupby("data_structure")["cycles_per_n"].median().sort_values()
    print("Top 3 (Fastest):")
    print(high_n.head(3))
    print("\nBottom 3 (Slowest):")
    print(high_n.tail(3))
    
    print("\n--- 4. TLB Wall Threshold ---")
    tlb_wall = df[df["tlb_miss_rate"] > 0.1].groupby("data_structure")["N"].min().sort_values()
    if not tlb_wall.empty:
        print("N where TLB misses/elem > 0.1:")
        print(tlb_wall.head(5))
    else:
        print("No TLB Wall detected with 0.1 threshold.")

if __name__ == "__main__":
    identify_problems()
