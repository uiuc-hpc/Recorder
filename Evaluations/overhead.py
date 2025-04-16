import numpy as np
import matplotlib.pyplot as plt

# Use LaTeX-style fonts for better readability
plt.rcParams.update({
    "font.size": 14,
    "axes.labelsize": 14,
    "axes.titlesize": 16,
    "legend.fontsize": 12,
    "xtick.labelsize": 12,
    "ytick.labelsize": 12
})

# Dummy data (in seconds)
labels = ['IOR', 'Flash-X']

# Baseline values (only execution time, no overhead)
baseline_execution = [227.244, 248.5534]
baseline_std = [0.864, 7.354]  # Standard deviation for baseline

# Optimized values (execution + overhead)
optimized_execution = [69.2902503, 125.28]  # Reduced execution time
optimized_prediction = [0.88, 4.00]  # Overhead for prediction
optimized_optimization = [0.0068954, 0.02]  # Overhead for optimization

# Standard deviations (error bars)
optimized_execution_std = [18.78, 13.58]
optimized_prediction_std = [0.02, 9.70]
optimized_optimization_std = [0.00069, 0.00399]

x = np.arange(len(labels))  # Group positions
width = 0.3  # Width of the bars

fig, ax = plt.subplots(figsize=(10, 4), dpi=400)  # High-resolution figure

# **Baseline bars with error bars**
ax.bar(x - 0.2, baseline_execution, width=width, label="Baseline", 
       color='#4C72B0', capsize=4, alpha=0.85)

# **Optimized bars with stacking and error bars**
bar1 = ax.bar(x+0.1, optimized_execution, width=width, label="SmartIO", 
              color='#92C5DE', capsize=4, alpha=0.85)

bar2 = ax.bar(x+0.1, optimized_prediction, width=width, bottom=optimized_execution, 
              label="Prediction & Extraction", color='#55A868', capsize=4, alpha=0.85)

bar3 = ax.bar(x+0.1, optimized_optimization, width=width, 
              bottom=np.array(optimized_execution) + np.array(optimized_prediction), 
              label="Optimization", color='#C44E52', capsize=4, alpha=0.85)

# **Compute and annotate percentage overhead**
for i, (exec_time, pred_overhead, opt_overhead) in enumerate(zip(optimized_execution, optimized_prediction, optimized_optimization)):
    total_time = exec_time + pred_overhead + opt_overhead
    overhead_percent = ((pred_overhead + opt_overhead) / total_time) * 100

    ax.text(i + 0.11, exec_time + pred_overhead + opt_overhead + 30, 
            f"{pred_overhead + opt_overhead:.2f}s\n({overhead_percent:.2f}%)", 
            ha='center', va='center', fontsize=18, color='black', weight='bold')

# **Labels and title**
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_ylabel("Execution Time (s)", fontsize=18)
# ax.set_title("Execution Time Breakdown: Baseline vs Optimized", fontsize=16)

# **Legend**
ax.legend(loc='upper center', bbox_to_anchor=(0.5, -0.10), ncol=2, fontsize=18)

# **Grid for readability**
ax.yaxis.grid(True, linestyle='solid', alpha=0.7)
ax.set_axisbelow(True)
plt.tick_params(axis='y', labelsize=18) 
plt.tick_params(axis='x', labelsize=18) 
# **Adjust layout to prevent overlapping**
plt.tight_layout()

# **Save figure as high-quality PNG for publication**
plt.savefig("Overhead.png", dpi=600, bbox_inches='tight')
plt.savefig("Overhead.pdf", format="pdf", bbox_inches="tight")

# Show plot
plt.show()
