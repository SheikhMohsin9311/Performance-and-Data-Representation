#!/usr/bin/env python
# coding: utf-8

# # Performance Analysis of Data Structures
# 
# This notebook analyzes the performance characteristics of various data structures based on hardware counters captured using `perf`.

# In[ ]:


import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

# Set style
sns.set_theme(style="whitegrid")
plt.rcParams["figure.figsize"] = (12, 6)


# ## 1. Data Loading and Preprocessing
# 
# The data in `deep_scaling_results.csv` appears to have some line wrapping (rows split across two lines). We'll use a custom loader to join these lines correctly.

# In[ ]:


def load_wrapped_csv(filepath):
    with open(filepath, 'r') as f:
        lines = [line.strip() for line in f if line.strip()]

    header = lines[0]
    data_lines = lines[1:]

    joined_rows = []
    # Each row is split into 2 lines
    for i in range(0, len(data_lines), 2):
        if i + 1 < len(data_lines):
            joined_rows.append(data_lines[i] + "," + data_lines[i+1])
        else:
            joined_rows.append(data_lines[i])

    # Create a single string and use pandas to read it
    csv_content = "\n".join([header] + joined_rows)
    from io import StringIO
    df = pd.read_csv(StringIO(csv_content))

    # Remove redundant columns if they were duplicated during joining
    expected_cols = header.split(",")
    df = df.iloc[:, :len(expected_cols)]
    df.columns = expected_cols

    # Convert all columns except 'data_structure' to numeric
    for col in df.columns:
        if col != 'data_structure':
            df[col] = pd.to_numeric(df[col], errors='coerce')

    return df

df = load_wrapped_csv('deep_scaling_results.csv')
print(f"Loaded {len(df)} rows and {len(df.columns)} columns.")
df.head()


# ## 2. Scaling Analysis: Cycles per Operation
# 
# We calculate `Cycles per N` to see how each data structure scales with size.

# In[ ]:


df['cycles_per_n'] = df['cycles'] / df['N']

plt.figure(figsize=(14, 8))
sns.lineplot(data=df, x='N', y='cycles_per_n', hue='data_structure', marker='o')
plt.xscale('log')
plt.yscale('log')
plt.title('Scaling: CPU Cycles per Operation vs N')
plt.ylabel('Cycles / N')
plt.xlabel('Number of Elements (N)')
plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
plt.grid(True, which="both", ls="--")
plt.tight_layout()
plt.show()


# ## 3. Instruction Throughput (IPC)
# 
# Instructions Per Cycle (IPC) indicates how well the CPU pipeline is utilized. Higher IPC usually means fewer stalls (e.g., from cache misses).

# In[ ]:


max_n = df['N'].max()
plt.figure(figsize=(14, 8))
sns.barplot(data=df[df['N'] == max_n], x='data_structure', y='ipc')
plt.title(f'Instruction Throughput (IPC) at N={max_n:,}')
plt.xticks(rotation=45)
plt.ylabel('IPC')
plt.show()


# ## 4. Cache Efficiency
# 
# How many cache misses occur per operation? This is a key metric for memory-bound data structures.

# In[ ]:


df['cache_misses_per_n'] = df['cache_misses'] / df['N']

plt.figure(figsize=(14, 8))
sns.lineplot(data=df, x='N', y='cache_misses_per_n', hue='data_structure', marker='o')
plt.xscale('log')
plt.yscale('log')
plt.title('Cache Misses per Operation vs N')
plt.ylabel('Cache Misses / N')
plt.xlabel('N')
plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
plt.show()


# ## 5. TLB Misses
# 
# Translation Lookaside Buffer (TLB) misses indicate page table walk overhead, which becomes significant when data exceeds memory page locality.

# In[ ]:


df['tlb_misses_per_n'] = df['TLB_misses'] / df['N']

# Filter out structures with no TLB data (recorded as NaN)
tlb_df = df.dropna(subset=['tlb_misses_per_n'])

if not tlb_df.empty:
    plt.figure(figsize=(14, 8))
    sns.lineplot(data=tlb_df, x='N', y='tlb_misses_per_n', hue='data_structure', marker='o')
    plt.xscale('log')
    plt.title('TLB Misses per Operation vs N')
    plt.ylabel('TLB Misses / N')
    plt.show()
else:
    print("No valid TLB data found to plot.")


# ## 6. Summary Heatmap
# 
# Comparing all metrics normalized at the largest N.

# In[ ]:


large_n_df = df[df['N'] == max_n].copy()
metrics = ['cycles_per_n', 'ipc', 'cache_misses_per_n', 'tlb_misses_per_n']
# Only include metrics that have at least some non-null values
available_metrics = [m for m in metrics if large_n_df[m].notnull().any()]

if available_metrics:
    summary = large_n_df.set_index('data_structure')[available_metrics]
    # Normalize for heatmap (0 to 1 scaling for each metric)
    summary_norm = (summary - summary.min()) / (summary.max() - summary.min())

    plt.figure(figsize=(12, 10))
    sns.heatmap(summary_norm, annot=summary, fmt=".2f", cmap="YlGnBu")
    plt.title(f'Normalized Performance Metrics (at N={max_n:,})')
    plt.show()
else:
    print("No numeric metrics available for heatmap.")

