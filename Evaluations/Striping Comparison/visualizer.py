import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# Set up the figure with two subplots sharing the Y-axis


modes = ["ps", "ts"]
block_size = ["32", "64", "128", "256"]
timesteps = 5

sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5)
sns.set_style("whitegrid")
sns.set_palette('colorblind')
palette = sns.color_palette("muted")  # Reverse Blues for contrast

# custom_colors = ["#1f77b4", "#ff7f0e"]  # Blue, Orange, Green
index = 0
fig, axes = plt.subplots(1, 4, figsize=(10, 4), sharey=True)
categories = ["Progressive", "Static"]

for size in block_size:
    time_ps_default = []
    time_ps = []
    time_ts = []
    for mode in modes:
        file_name = f"output_{size}m_16_{mode}.txt"
        with open(file_name) as f:
            lines = f.readlines()
            flag = False
            tmp_time = []
            for line in lines:
                if not flag:
                    if "---------" in line.strip():
                        flag = True
                else:
                    res = line.split()

                    if res[0] == "write":
                        curr_iter = int(res[10])
                        if curr_iter < timesteps - 1: 
                            tmp_time.append(float(res[9]))
                        else:
                            flag = False
                            tmp_time.append(float(res[9]))
                            if mode == "ps_default":
                                time_ps_default.append(sum(tmp_time))
                            if mode == "ps":
                                time_ps.append(sum(tmp_time))
                            elif mode == "ts":
                                time_ts.append(sum(tmp_time))
                            tmp_time = []

    # array_data = np.array(time_ps_default)
    # avg_time_ps_default = np.mean(array_data, axis=0)
    # std_time_ps_default = np.std(array_data, axis=0)

    array_data = np.array(time_ps)
    avg_time_ps = np.mean(array_data, axis=0)
    std_time_ps = np.std(array_data, axis=0)

    array_data = np.array(time_ts)
    avg_time_ts = np.mean(array_data, axis=0)
    std_time_ts = np.std(array_data, axis=0)

    bandwidths_1 = [avg_time_ps, avg_time_ts]
    variance_1 = [std_time_ps, std_time_ts]
    print(bandwidths_1)
    print(variance_1)
    # Colors from the uploaded image
    colors = ["#DD8452", "#4C72B0"]  # Blue for Baseline, Orange for Optimized

    df = {'Category': ["Progressive", "Static"],
        'Value': bandwidths_1,
        'Error': variance_1}

    sns.barplot(ax=axes[index], x='Category', y='Value', data=df, capsize=0.1, palette="muted")
    x_positions = np.arange(len(bandwidths_1))  # x positions for error bars

    axes[index].errorbar(x_positions, bandwidths_1, yerr=variance_1, fmt='o', linestyle='', color='black', capsize=5)



    # sns.barplot(ax=axes[index], x=categories, y=bandwidths_1, yerr=variance_1, 
    #                    capsize=5, palette=colors, edgecolor="black", linewidth=1.0)

    # Add error bars
    # for i, (b, o, sb, so) in enumerate(zip(avg_time_ps, avg_time_ts, std_time_ps, std_time_ts)):
    #     plt.errorbar(i-0.2, b, yerr=sb, fmt='none', color='black', capsize=5)
    #     plt.errorbar(i+0.2, o, yerr=so, fmt='none', color='black', capsize=5)
    ####

    # sns.barplot(ax=axes[index], x=categories, y=bandwidths_1, yerr=variance_1, capsize=0.1, palette="muted")
    # for i, (bandwidth, error) in enumerate(zip(bandwidths_1, variance_1)):
    #     axes[index].text(i, bandwidth + 8, f"±{round(error, 3)}", ha='center', fontsize=24, fontweight='bold')
    
    if index == 0:
        axes[index].set_ylabel("Execution Time (s)", fontsize=18)

    axes[index].tick_params(axis='y', labelsize=18) 
    axes[index].tick_params(axis='x', labelsize=18) 
    axes[index].set_xticklabels([]) 
    axes[index].set_xlabel("")
    axes[index].set_title(f"FS = {size}GB", fontsize=18)
    index += 1

# Create a custom legend for categories
handles = [plt.Rectangle((0, 0), 1, 1, color=color, edgecolor="black") for color in colors]
axes[1].legend(handles, categories, loc="upper center", ncol=1, fontsize=18)

plt.subplots_adjust(wspace=-0.5)  # Reduces space between subplots
plt.tight_layout(rect=[0, 0, 1, 0.93])  # Leave space for legend
plt.savefig("Striping.png", dpi=300, bbox_inches="tight")
plt.savefig("Striping.pdf", format="pdf", bbox_inches="tight")
# plt.savefig("Bandwidths")
