import json
import os

NOTEBOOK_PATH = "analysis.ipynb"
DATA_PATH = "high_fidelity_standardized_results.csv"

if not os.path.exists(NOTEBOOK_PATH):
    print(f"Notebook {NOTEBOOK_PATH} not found.")
    exit(1)

with open(NOTEBOOK_PATH, 'r') as f:
    nb = json.load(f)

# The first code cell is usually the one with the import and loading logic
for cell in nb['cells']:
    if cell['cell_type'] == 'code':
        # Check if it has load_data or similar
        found = False
        for line in cell['source']:
            if 'def load_data' in line:
                found = True
                break
        
        if found:
            print("Found loading cell. Patching...")
            new_source = [
                "import pandas as pd\n",
                "import numpy as np\n",
                "import matplotlib.pyplot as plt\n",
                "import seaborn as sns\n",
                "import glob\n",
                "import time\n",
                "import os\n",
                "import warnings\n",
                "warnings.filterwarnings('ignore')\n",
                "\n",
                "# --- Modern GitHub Dark Aesthetics ---\n",
                "sns.set_theme(style=\"darkgrid\", palette=\"muted\")\n",
                "plt.rcParams.update({\n",
                "    'figure.facecolor': '#0d1117', 'axes.facecolor': '#161b22', \n",
                "    'axes.edgecolor': '#30363d', 'axes.labelcolor': '#e6edf3',\n",
                "    'xtick.color': '#8b949e', 'ytick.color': '#8b949e', \n",
                "    'text.color': '#e6edf3', 'grid.color': '#21262d',\n",
                "    'font.family': 'monospace', 'axes.titlesize': 15,\n",
                "    'legend.frameon': True, 'legend.facecolor': '#161b22', 'legend.edgecolor': '#30363d'\n",
                "})\n",
                "\n",
                "COLORS = {\n",
                "    'Vector': '#79c0ff', 'Array': '#1f6feb', 'Deque': '#a5d6ff', 'Sequential': '#79c0ff',\n",
                "    'BST': '#ff7b72', 'RBTree': '#ff9e44', 'BTree': '#f0883e', 'SkipList': '#d2a8ff', 'vEBTree': '#bc8cff',\n",
                "    'LinkedList': '#f78166', 'SlabList': '#ffa198',\n",
                "    'AoS': '#3fb950', 'SoA': '#56d364',\n",
                "    'HashMap': '#e3b341', 'Trie': '#f8e3a1', 'CircularBuffer': '#388bfd'\n",
                "}\n",
                "\n",
                "def load_standardized(path):\n",
                "    print(f\"Loading standardized data from {path}...\")\n",
                "    df = pd.read_csv(path)\n",
                "    if 'median_cycles' in df.columns:\n",
                "        df['cycles_per_elem'] = df['median_cycles'] / df['N']\n",
                "    elif 'cycles' in df.columns:\n",
                "        df['cycles_per_elem'] = df['cycles'] / df['N']\n",
                "    if 'median_instructions' in df.columns and 'median_cycles' in df.columns:\n",
                "        df['ipc'] = df['median_instructions'] / df['median_cycles']\n",
                "    return df\n",
                "\n",
                "DATA_PATH = \"high_fidelity_standardized_results.csv\"\n",
                "if os.path.exists(DATA_PATH):\n",
                "    df = load_standardized(DATA_PATH)\n",
                "    print(f\"Successfully loaded {len(df)} records.\")\n",
                "else:\n",
                "    print(f\"Error: {DATA_PATH} not found. Ensure consolidate_results.py is run.\")\n"
            ]
            cell['source'] = new_source
            break

with open(NOTEBOOK_PATH, 'w') as f:
    json.dump(nb, f, indent=1)
print("Notebook patched successfully.")
