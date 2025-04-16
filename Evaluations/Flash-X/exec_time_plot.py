from datetime import datetime
import matplotlib.pyplot as plt
import numpy as np
import itertools
import pandas as pd
import seaborn as sns

sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5)
sns.set_style("whitegrid")
sns.set_palette('colorblind')
sns.color_palette("muted")

runs = 10
no_checkpoints = 7
folder = ["8", "16", "32"]
fig, axes = plt.subplots(1, 3, figsize=(10, 4),  sharey=True)
index_x = 0
index_y = 0

mean_base_exec = []
mean_opt_exec = []
std_base_exec = []
std_opt_exec = []

for node in folder:
    baseline_time = []
    optimized_time = []

    baseline_execution_time = []
    optimized_execution_time = []

    time_start = 0
    time_end = 0
    current_run = 0
    if node == "8":
        chk_size = [14*1024] * 3 + [15*1024] * 4
    elif node == "16":
        chk_size = [41*1024] * 3 + [42*1024] * 4
    elif node == "32":
        chk_size = [215*1024] * 3 + [217*1024] * 1 + [220*1024] * 1 + [223*1024] * 2
    elif node == "64":
        chk_size = [1.4*1024*1024] * 5 + [1.5*1024*1024] * 2
    file_name = f'{node} Nodes/sedov.log'

    with open(file_name) as f:
        lines = f.readlines()
        for line in lines:
            if "Run number:" in line:
                current_run += 1
            if "IO_writeCheckpoint" in line:
            
                line = line.replace("[", "")
                line = line.replace("]", "")
                
                line = line.split()
                if "sedov_hdf5_chk_0008" not in line[len(line) - 1]:
                    if line[3] == "open:":
                        time_start = line[1]
                    elif line[3] == "close:":
                        time_end = line[1]

                        fmt = '%H:%M:%S.%f'
                        tstamp1 = datetime.strptime(time_start, fmt)
                        tstamp2 = datetime.strptime(time_end, fmt)

                        if tstamp1 > tstamp2:
                            td = tstamp1 - tstamp2
                        else:
                            td = tstamp2 - tstamp1

                        if current_run > runs/2:
                            baseline_time.append(td.total_seconds())
                        else:
                            optimized_time.append(td.total_seconds())
            # if "IO_output" in line:
            #     line = line.split()
            #     if current_run > runs/2:
            #         baseline_execution_time.append(float(line[1]))
            #     else:
            #         optimized_execution_time.append(float(line[1]))


    optimized_time = [optimized_time[i:i+no_checkpoints] for i in range(0, len(optimized_time), no_checkpoints)] 
    optimized_execution_time = [sum(lst) for lst in optimized_time]
    array_data = np.array(optimized_execution_time)
    mean_opt_exec.append(np.mean(array_data))
    std_opt_exec.append(np.std(array_data))
    # array_data = np.array(optimized_time)
    # optimized_execution_time = np.mean(array_data, axis=0)
    optimized_time = np.array(chk_size)/np.array(optimized_time)
    optimized_time = optimized_time.tolist()

    avg_optimized_time = [round(float(sum(col))/len(col), 2) for col in zip(*optimized_time)]
    array_data = np.array(optimized_time)
    std_devs_opt = np.std(array_data, axis=0)  # Use ddof=1 for sample std deviation

    baseline_time = [baseline_time[i:i+no_checkpoints] for i in range(0, len(baseline_time), no_checkpoints)]
    baseline_execution_time = [sum(lst) for lst in baseline_time]
    array_data = np.array(baseline_execution_time)
    mean_base_exec.append(np.mean(array_data))
    std_base_exec.append(np.std(array_data))
    # array_data = np.array(baseline_time)
    # baseline_execution_time = np.mean(array_data, axis=0)
    # print(baseline_execution_time)
    baseline_time = np.array(chk_size)/np.array(baseline_time)
    baseline_time = baseline_time.tolist()


    avg_baseline_time = [round(float(sum(col))/len(col), 2) for col in zip(*baseline_time)]
    # print(avg_baseline_time)
    array_data = np.array(baseline_time)
    std_devs_base = np.std(array_data, axis=0)

    # print(baseline_execution_time)
    # mean_base_exec.append(np.sum(baseline_execution_time))
  

    # # print(optimized_execution_time)
    # mean_opt_exec.append(np.sum(optimized_execution_time))
    # std_opt_exec.append(np.std(optimized_execution_time))

    # print(avg_baseline_time)
    # print(avg_optimized_time)
    x = np.arange(0, no_checkpoints)
    print(avg_baseline_time)
    print(avg_optimized_time)
    df_base = pd.DataFrame({'x': x, 'y': avg_baseline_time, 'y_upper': avg_baseline_time + std_devs_base, 'y_lower': avg_baseline_time - std_devs_base})
    df_opt = pd.DataFrame({'x': x, 'y': avg_optimized_time, 'y_upper': avg_optimized_time + std_devs_opt, 'y_lower': avg_optimized_time - std_devs_opt})

    for data, color, label in zip([(df_base, "red", "Baseline Trend"), (df_opt, "blue", "Optimized Trend")],
                                ["red", "blue"],
                                ["Mean Baseline Trend", "Mean Optimized Trend (SmartIO)"]):
        sns.lineplot(x=data[0]["x"], y=data[0]["y"], label=label, ax=axes[index_y], color=color, legend=False)
        axes[index_y].fill_between(data[0]["x"], data[0]["y_lower"], data[0]["y_upper"], color=color, alpha=0.3)

    if index_y == 0:
        axes[index_y].set_ylabel("BW (MB/s)", fontsize=24) 
    axes[index_y].set_xlabel("Checkpoints", fontsize=24)
    axes[index_y].tick_params(axis='y', labelsize=22) 
    axes[index_y].tick_params(axis='x', labelsize=22) 
    processes = 448 * (index_y + 1)
    axes[index_y].set_title(f"{node} Nodes, {processes} Processes", fontsize=24)
    index_y += 1
       
axes[0].legend(loc="upper left", ncol=1, fontsize=24)

plt.subplots_adjust(wspace=0.1)
plt.tight_layout()

# plt.savefig(f"Bandwidths.png", dpi=300, bbox_inches="tight")

print(mean_base_exec)

print(mean_opt_exec)


import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import pandas as pd

# Data
nodes = ['8 Nodes', '16 Nodes', '32 Nodes']
baseline = mean_base_exec
optimized = mean_opt_exec
std_baseline = std_base_exec
std_optimized = std_opt_exec

sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5)
sns.set_style("white")
sns.set_palette('colorblind')
sns.color_palette("muted")
# Create DataFrame
df = pd.DataFrame({
    "Nodes": nodes * 2,
    "Execution Time (s)": baseline + optimized,
    "Configuration": ["Baseline"] * 3 + ["Optimized (SmartIO)"] * 3,
    "Std Dev": std_baseline + std_optimized
})

# Colors from the uploaded image
colors = ["#4C72B0", "#DD8452"]  # Blue for Baseline, Orange for Optimized

# Plot
plt.figure(figsize=(10, 4))
sns.barplot(data=df, x="Nodes", y="Execution Time (s)", hue="Configuration", 
            palette="muted", capsize=0.1, ci=None)

# Add error bars
for i, (b, o, sb, so) in enumerate(zip(baseline, optimized, std_baseline, std_optimized)):
    plt.errorbar(i-0.2, b, yerr=sb, fmt='o', color='black', linestyle='', capsize=5)
    plt.errorbar(i+0.2, o, yerr=so, fmt='o', color='black', linestyle='', capsize=5)

# Labels and styling
plt.ylabel("I/O Time (s)", fontsize=18)
plt.xlabel("", fontsize=18)
plt.tick_params(axis='y', labelsize=18) 
plt.tick_params(axis='x', labelsize=18) 
plt.xticks(ticks=range(len(nodes)), labels=nodes)
plt.legend(loc="upper left", fontsize=18)
plt.grid(axis='y', linestyle="solid", alpha=0.7)

# Show the plot
plt.tight_layout()
plt.savefig(f"FlashX_Exec_Time.png", dpi=300, bbox_inches="tight")
plt.savefig("FlashX_Exec_Time.pdf", format="pdf", bbox_inches="tight")

