# import numpy as np
# import pandas as pd
# import seaborn as sns
# import itertools
# import matplotlib.pyplot as plt
# from scipy import stats

# nodes = [8, 32]
# modes = ["base", "opt"]
# timesteps = 5
# folder = "32 Nodes/"
# sns.set_theme(style="whitegrid")
# sns.set_context("paper", font_scale=1.5)


# for node in nodes:
#     bw_base = []
#     bw_opt = []
#     for mode in modes:
#         file_name = f"{node} Nodes/output_4096k_{node}_{mode}.txt"
#         with open(file_name) as f:
#             lines = f.readlines()
#             flag = False
#             tmp_bw = []
#             for line in lines:
#                 if not flag:
#                     if "---------" in line.strip():
#                         flag = True
#                 else:
#                     res = line.split()
#                     if res[0] == "write":
#                         curr_iter = int(res[10])
#                         if curr_iter < timesteps - 1: 
#                             tmp_bw.append(float(res[1]))
#                         else:
#                             flag = False
#                             tmp_bw.append(float(res[1]))
#                             if mode == "base":
#                                 bw_base.append(tmp_bw)
#                             else:
#                                 bw_opt.append(tmp_bw)
#                             tmp_bw = []

#     print(bw_base)
#     print(bw_opt)

#     array_data = np.array(bw_base)
#     std_devs_base = np.std(array_data, axis=0)

#     array_data = np.array(bw_opt)
#     std_devs_opt = np.std(array_data, axis=0)

#     print(std_devs_base)
#     print(std_devs_opt)

#     avg_bw_base = np.array(bw_base)
#     avg_bw_base = np.mean(avg_bw_base, axis=0)

#     avg_bw_opt = np.array(bw_opt)
#     avg_bw_opt = np.mean(avg_bw_opt, axis=0)

#     # Generate sample data
#     x = np.arange(0, timesteps)


#     # Create DataFrame
#     df_base = pd.DataFrame({'x': x, 'y': avg_bw_base, 'y_upper': avg_bw_base + std_devs_base, 'y_lower': avg_bw_base - std_devs_base})
#     df_opt = pd.DataFrame({'x': x, 'y': avg_bw_opt, 'y_upper': avg_bw_opt + std_devs_opt, 'y_lower': avg_bw_opt - std_devs_opt})

#     # Create the plot
#     plt.figure(figsize=(12, 7))
#     sns.lineplot(x=df_base["x"], y=df_base["y"], label=f"Baseline Trend", color="red")
#     plt.fill_between(df_base["x"], df_base["y_lower"], df_base["y_upper"], color="red", alpha=0.2, label=f"Baseline ± Std Dev")

#     sns.lineplot(x=df_opt["x"], y=df_opt["y"], label=f"Optimized Trend", color="blue")
#     plt.fill_between(df_opt["x"], df_opt["y_lower"], df_opt["y_upper"], color="blue", alpha=0.2, label=f"Optimized ± Std Dev")
#     plt.xticks(x)
#     plt.ylim(0, 35000)

#     plt.xlabel("Timesteps", fontsize=14)
#     plt.ylabel("BW (MB/s)", fontsize=14)
#     plt.legend(loc='upper right', fontsize='x-small')
#     plt.savefig(f"{node} Nodes/Bandwidths")

# import numpy as np
# import pandas as pd
# import seaborn as sns
# import matplotlib.pyplot as plt

# # Set up the figure with two subplots sharing the Y-axis


# nodes = [8, 16, 32]
# modes = ["base", "opt"]
# timesteps = 8
# # sns.set_theme(style="whitegrid")
# sns.set_context("paper", font_scale=1.5)
# sns.set_style("darkgrid")
# sns.set_palette('colorblind')
# sns.color_palette("muted")
# index = 0
# access = "Write"
# fig, axes = plt.subplots(1, 3, figsize=(22, 6), sharey=True)
# processes = 256
# for node in nodes:
#     bw_base = []
#     bw_opt = []
#     percentage_increases = []
#     for mode in modes:
#         file_name = f"GPFS/{node} Nodes/output_4096k_{node}_{mode}.txt"
#         with open(file_name) as f:
#             lines = f.readlines()
#             flag = False
#             tmp_bw = []
#             for line in lines:
#                 if not flag:
#                     if "---------" in line.strip():
#                         flag = True
#                 else:
#                     res = line.split()
#                     if res[0] == "read":
#                         curr_iter = int(res[10])
#                         if curr_iter < timesteps - 1: 
#                             tmp_bw.append(float(res[1]))
#                         else:
#                             flag = False
#                             tmp_bw.append(float(res[1]))
#                             if mode == "base":
#                                 bw_base.append(tmp_bw)
#                             else:
#                                 bw_opt.append(tmp_bw)
#                             tmp_bw = []

#     print(bw_base)
#     print(bw_opt)
#     array_data = np.array(bw_base)
#     std_devs_base = np.std(array_data, axis=0)

#     array_data = np.array(bw_opt)
#     std_devs_opt = np.std(array_data, axis=0)

#     # print(std_devs_base)
#     # print(std_devs_opt)

#     avg_bw_base = np.array(bw_base)
#     avg_bw_base = np.mean(avg_bw_base, axis=0)

#     avg_bw_opt = np.array(bw_opt)
#     avg_bw_opt = np.mean(avg_bw_opt, axis=0)

#     increase = avg_bw_opt - avg_bw_base
#     percentage_increase = (increase / avg_bw_base) * 100
#     print(percentage_increase)

#     x = np.arange(0, timesteps)
#     df_base = pd.DataFrame({'x': x, 'y': avg_bw_base, 'y_upper': avg_bw_base + std_devs_base, 'y_lower': avg_bw_base - std_devs_base})
#     df_opt = pd.DataFrame({'x': x, 'y': avg_bw_opt, 'y_upper': avg_bw_opt + std_devs_opt, 'y_lower': avg_bw_opt - std_devs_opt})

#     for data, color, label in zip([(df_base, "red", "Baseline Trend"), (df_opt, "blue", "Optimized Trend")],
#                                ["red", "blue"],
#                                ["Mean Baseline Trend", "Mean Optimized Trend"]):
#         sns.lineplot(x=data[0]["x"], y=data[0]["y"], ax=axes[index], label=label, color=color, legend=False)
#         axes[index].fill_between(data[0]["x"], data[0]["y_lower"], data[0]["y_upper"], color=color, alpha=0.3)
    
#     if index == 0:
#         axes[index].set_ylabel(f"{access} BW (MB/s)", fontsize=20)

#     axes[index].set_xlabel("Timesteps", fontsize=20)
#     axes[index].set_title(f"{node} Nodes, {processes} Processes", fontsize=20)
#     # axes[index].legend(loc="upper right")
#     index += 1
#     processes = processes * 2

# plt.subplots_adjust(wspace=-0.5)  # Reduces space between subplots
# plt.legend(loc="upper center", bbox_to_anchor=(0.5, -0.15), ncol=2, fontsize=15)
# plt.tight_layout()
# plt.savefig("Bandwidths.png", dpi=300, bbox_inches="tight")
# # plt.savefig("Bandwidths")


import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# Set up the figure with two subplots sharing the Y-axis


nodes = [8, 16, 32]
modes = ["base", "opt"]

timesteps = 7
# sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5)
sns.set_style("whitegrid")
sns.set_palette('colorblind')
sns.color_palette("muted")
index = 0
access = "Write"
fig, axes = plt.subplots(1, 3, figsize=(10, 4), sharey=True)
processes = 256
file_system = ["GPFS", "Lustre"]

time_base_gpfs = []
time_opt_gpfs = []
percentage_increases_gpfs = []

time_base_lustre = []
time_opt_lustre = []
percentage_increases_lustre = []

for node in nodes:
    for fs in file_system:
        for mode in modes:
            file_name = f"{fs}/{node} Nodes/output_4096k_{node}_{mode}.txt"
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
                            tmp_time.append(float(res[9]))
                        elif res[0] == "read":
                            curr_iter = int(res[10])
                            if curr_iter < timesteps - 1: 
                                tmp_time.append(float(res[9]))
                            else:
                                flag = False
                                tmp_time.append(float(res[9]))
                                if mode == "base":
                                    if fs == "Lustre":
                                        time_base_lustre.append(sum(tmp_time))
                                    elif fs == "GPFS":
                                        time_base_gpfs.append(sum(tmp_time))
                                elif mode == "opt":
                                    if fs == "Lustre":
                                        time_opt_lustre.append(sum(tmp_time))
                                    elif fs == "GPFS":
                                        time_opt_gpfs.append(sum(tmp_time))
                                tmp_time = []

time_base_gpfs = [time_base_gpfs[i:i+10] for i in range(0, len(time_base_gpfs), 10)] 
time_opt_gpfs = [time_opt_gpfs[i:i+10] for i in range(0, len(time_opt_gpfs), 10)] 

time_base_lustre = [time_base_lustre[i:i+10] for i in range(0, len(time_base_lustre), 10)] 
time_opt_lustre = [time_opt_lustre[i:i+10] for i in range(0, len(time_opt_lustre), 10)] 
print(time_base_lustre[0])
print(time_opt_lustre[0])

array_data = np.array(time_base_lustre[0])
avg_time_base_lustre_8 = np.mean(array_data, axis=0)

std_time_base_lustre_8 = np.std(array_data, axis=0)

array_data = np.array(time_base_lustre[1])
avg_time_base_lustre_16 = np.mean(array_data, axis=0)
std_time_base_lustre_16 = np.std(array_data, axis=0)

array_data = np.array(time_base_lustre[2])
avg_time_base_lustre_32 = np.mean(array_data, axis=0)
print(avg_time_base_lustre_32)
std_time_base_lustre_32 = np.std(array_data, axis=0)

array_data = np.array(time_opt_lustre[0])
avg_time_opt_lustre_8 = np.mean(array_data, axis=0)
std_time_opt_lustre_8 = np.std(array_data, axis=0)

array_data = np.array(time_opt_lustre[1])
avg_time_opt_lustre_16 = np.mean(array_data, axis=0)
std_time_opt_lustre_16 = np.std(array_data, axis=0)

array_data = np.array(time_opt_lustre[2])
avg_time_opt_lustre_32 = np.mean(array_data, axis=0)
print(avg_time_opt_lustre_32)
std_time_opt_lustre_32 = np.std(array_data, axis=0)

#######

array_data = np.array(time_base_gpfs[0])
avg_time_base_gpfs_8 = np.mean(array_data, axis=0)
std_time_base_gpfs_8 = np.std(array_data, axis=0)

array_data = np.array(time_base_gpfs[1])
avg_time_base_gpfs_16 = np.mean(array_data, axis=0)
std_time_base_gpfs_16 = np.std(array_data, axis=0)

array_data = np.array(time_base_gpfs[2])
avg_time_base_gpfs_32 = np.mean(array_data, axis=0)
std_time_base_gpfs_32 = np.std(array_data, axis=0)

array_data = np.array(time_opt_gpfs[0])
avg_time_opt_gpfs_8 = np.mean(array_data, axis=0)
std_time_opt_gpfs_8 = np.std(array_data, axis=0)

array_data = np.array(time_opt_gpfs[1])
avg_time_opt_gpfs_16 = np.mean(array_data, axis=0)
std_time_opt_gpfs_16 = np.std(array_data, axis=0)

array_data = np.array(time_opt_gpfs[2])
avg_time_opt_gpfs_32 = np.mean(array_data, axis=0)
std_time_opt_gpfs_32 = np.std(array_data, axis=0)


data = {
    "Machine": ["C1"] * 6 + ["C2"] * 6,
    "Nodes": [8, 8, 16, 16, 32, 32, 8, 8, 16, 16, 32, 32],
    "Case": ["Baseline", "Optimized (SmartIO)"] * 6,
    "Execution Time": [avg_time_base_lustre_8, avg_time_opt_lustre_8, avg_time_base_lustre_16, avg_time_opt_lustre_16, avg_time_base_lustre_32, avg_time_opt_lustre_32, avg_time_base_gpfs_8, avg_time_opt_gpfs_8, avg_time_base_gpfs_16, avg_time_opt_gpfs_16, avg_time_base_gpfs_32, avg_time_opt_gpfs_32],  # Example times
    "Error": [std_time_base_lustre_8, std_time_opt_lustre_8, std_time_base_lustre_16, std_time_opt_lustre_16, std_time_base_lustre_32, std_time_opt_lustre_32, std_time_base_gpfs_8, std_time_opt_gpfs_8, std_time_base_gpfs_16, std_time_opt_gpfs_16, std_time_base_gpfs_32, std_time_opt_gpfs_32]  # Example times

}

# Create DataFrame
df = pd.DataFrame(data)

# Set the style and figure size
sns.set_theme(style="whitegrid")

# Create separate bar plots for each machine
g = sns.FacetGrid(df, col="Machine", height=4, aspect=1.2, sharey=True)

# Define the bar plot function with data labels
def barplot_with_error_caps(data, **kwargs):
    ax = sns.barplot(
        x="Nodes", y="Execution Time", hue="Case", data=data, palette="muted",
        ci=None, **kwargs
    )
        # Enable horizontal grid lines
    ax.spines['top'].set_visible(True)
    ax.spines['right'].set_visible(True)
    ax.spines['left'].set_visible(True)
    ax.spines['bottom'].set_visible(True)

    # Optional: make sure spines are styled consistently
    for spine in ax.spines.values():
        spine.set_linewidth(0.5)
        spine.set_color("black")

    # Add manual error bars with horizontal caps
    for i, bar in enumerate(ax.patches):
        node_group = data["Nodes"].unique()
        x_positions = np.arange(len(node_group))  # Get x positions for each node category

        case_labels = data["Case"].unique()
        case_offset = [-0.2, 0.2]  # Offset for Baseline and Optimized bars
        case_index = i % len(case_labels)  # Get case index for offset
        
        x_loc = x_positions[i // len(case_labels)] + case_offset[case_index]  # Adjust x location
        y_val = data.iloc[i]["Execution Time"]
        y_err = data.iloc[i]["Error"]

        # Plot error bars with caps
        ax.errorbar(
            x=x_loc, y=y_val, yerr=y_err, fmt='o', color='black', capsize=2,markersize=2, linestyle=''
        )

        ax.tick_params(axis='y', labelsize=18) 
        ax.tick_params(axis='x', labelsize=18) 

# Apply the function to each subplot
g.map_dataframe(barplot_with_error_caps)

# Improve labels and titles
g.set_axis_labels("Nodes", "Execution Time (s)", fontsize=18)
g.set_titles("{col_name}", size=18)

index = 0
for ax in g.axes.flat:
    if index == 1:
        handles, labels = ax.get_legend_handles_labels()
        ax.legend(handles, labels, loc='upper right', frameon=True, fontsize=18)
    index = index + 1


# Adjust layout for better appearance
plt.subplots_adjust(top=0.85)
plt.savefig("IOR_Exec_Time.png", dpi=300, bbox_inches="tight")
plt.savefig("IOR_Exec_Time.pdf", format="pdf", bbox_inches="tight")