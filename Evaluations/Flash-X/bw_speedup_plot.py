import matplotlib.pyplot as plt
import numpy as np

# Checkpoints
checkpoints = np.arange(0, 7)

# Bandwidth data
baseline_8 = [1961.11, 1950.38, 1936.47, 1964.7, 1929.51, 1946.15, 1954.1]
smartio_8 = [1781.35, 1973.18, 7111.06, 7275.94, 7258.72, 7299.32, 7278.71]

baseline_16 = [5476.42, 5600.22, 5632.57, 5548.95, 5551.32, 5610.71, 5646.06]
smartio_16 = [5004.26, 5662.24, 11598.22, 11632.86, 11593.89, 11473.13, 11577.98]

baseline_32 = [6317.31, 6295.57, 6304.4, 6278.32, 6247.32, 6306.4, 6319.75]
smartio_32 = [5679.9, 6333.33, 20188.1, 20378.61, 20301.85, 19983.2, 19651.99]

# Calculate speedup
speedup_8 = np.array(smartio_8) / np.array(baseline_8)
speedup_16 = np.array(smartio_16) / np.array(baseline_16)
speedup_32 = np.array(smartio_32) / np.array(baseline_32)

# Plotting
plt.figure(figsize=(10, 4))
plt.plot(checkpoints, speedup_8, marker='o', label='8 Nodes')
plt.plot(checkpoints, speedup_16, marker='s', label='16 Nodes')
plt.plot(checkpoints, speedup_32, marker='^', label='32 Nodes')
plt.axhline(y=1, color='black', linestyle='--', linewidth=2, label='Baseline (1×)')

# Labels and formatting
plt.xlabel("Checkpoint", fontsize=18)
plt.ylabel("Speedup (×)", fontsize=18)
plt.ylim(bottom=0)
plt.tick_params(axis='y', labelsize=18) 
plt.tick_params(axis='x', labelsize=18) 
# plt.title("SmartIO Real-Time Speedup at Each Checkpoint Across Node Counts")
plt.xticks(checkpoints)
plt.legend(loc="upper left", fontsize=18)
plt.grid(axis='y', linestyle='solid', alpha=0.7)

plt.tight_layout()

# Save as SVG
output_path = "FlashX_BW_Speedup.png"
plt.savefig(output_path, format="png")
plt.savefig("FlashX_BW_Speedup.pdf", format="pdf", bbox_inches="tight")
plt.close()

output_path
