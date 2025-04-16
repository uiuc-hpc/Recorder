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
sns.set_style("darkgrid")
sns.set_palette('colorblind')
sns.color_palette("muted")
index = 0
access = "Read"
fig, axes = plt.subplots(1, 3, figsize=(22, 6), sharey=True)
processes = 256
for node in nodes:
    bw_base = []
    bw_opt = []
    for mode in modes:
        file_name = f"{node} Nodes/output_4096k_{node}_{mode}.txt"
        with open(file_name) as f:
            lines = f.readlines()
            flag = False
            tmp_bw = []
            for line in lines:
                if not flag:
                    if "---------" in line.strip():
                        flag = True
                else:
                    res = line.split()
                    if res[0] == "read":
                        curr_iter = int(res[10])
                        if curr_iter < timesteps - 1: 
                            tmp_bw.append(float(res[1]))
                        else:
                            flag = False
                            tmp_bw.append(float(res[1]))
                            if mode == "base":
                                bw_base.append(tmp_bw)
                            else:
                                bw_opt.append(tmp_bw)
                            tmp_bw = []

    # print(bw_base)
    # print("\n")
    # print(bw_opt)
    array_data = np.array(bw_base)
    std_devs_base = np.std(array_data, axis=0)

    array_data = np.array(bw_opt)
    std_devs_opt = np.std(array_data, axis=0)

    # print(std_devs_base)
    # print(std_devs_opt)

    avg_bw_base = np.array(bw_base)
    avg_bw_base = np.median(avg_bw_base, axis=0)

    avg_bw_opt = np.array(bw_opt)
    avg_bw_opt = np.median(avg_bw_opt, axis=0)
    
    increase = avg_bw_opt - avg_bw_base
    percentage_increase = (increase / avg_bw_base) * 100
    percentage_increase = np.round(percentage_increase, decimals=2)

    # print(percentage_increase)

    print("\n")
    print(avg_bw_base)
    print("\n")
    print(avg_bw_opt)
    x = np.arange(0, timesteps)
    df_base = pd.DataFrame({'x': x, 'y': avg_bw_base, 'y_upper': avg_bw_base + std_devs_base, 'y_lower': avg_bw_base - std_devs_base})
    df_opt = pd.DataFrame({'x': x, 'y': avg_bw_opt, 'y_upper': avg_bw_opt + std_devs_opt, 'y_lower': avg_bw_opt - std_devs_opt})

    for data, color, label in zip([(df_base, "red", "Baseline Trend"), (df_opt, "blue", "Optimized Trend")],
                               ["red", "blue"],
                               ["Mean Baseline Trend", "Mean Optimized Trend"]):
        sns.lineplot(x=data[0]["x"], y=data[0]["y"], ax=axes[index], label=label, color=color, legend=False)
        axes[index].fill_between(data[0]["x"], data[0]["y_lower"], data[0]["y_upper"], color=color, alpha=0.3)
    
    if index == 0:
        axes[index].set_ylabel(f"{access} BW (MB/s)", fontsize=20)

    axes[index].set_xlabel("Timesteps", fontsize=20)
    axes[index].set_title(f"{node} Nodes, {processes} Processes", fontsize=20)
    # axes[index].legend(loc="upper right")
    index += 1
    processes = processes * 2

plt.subplots_adjust(wspace=-0.5)  # Reduces space between subplots
plt.legend(loc="upper center", bbox_to_anchor=(0.5, -0.15), ncol=2, fontsize=15)
plt.tight_layout()
plt.savefig("Bandwidths.png", dpi=300, bbox_inches="tight")
# plt.savefig("Bandwidths")
