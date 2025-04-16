""" import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# Set up the figure with two subplots sharing the Y-axis



timesteps = 5
sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5)
sns.set_style("darkgrid")
sns.set_palette('colorblind')
sns.color_palette("muted")



plt.figure(figsize=(10,6))
 #, "romio_ds_write", "ind_wr_buffer_size"
parameters = ["romio_ds_read", "ind_rd_buffer_size"]
fig, axes = plt.subplots(1, 2, figsize=(15, 5), sharey=True)


index = 0

for parameter in parameters:
    
    bw_base = []
    bw_8M = []
    bw_4M = []
    bw_2M = []
    bw_1M = []
    if parameter=="ind_wr_buffer_size":
        access = 'write'
        modes = ["base", "first", "second", "third", "fourth"]
    elif parameter=="ind_rd_buffer_size":
        access = 'read'
        modes = ["base", "first", "second", "third", "fourth"]
    elif parameter=="romio_ds_write":
        access = 'write'
        modes = ["base", "first", "second"]
    elif parameter=="romio_ds_read":
        access = 'read'
        modes = ["base", "first", "second"]
    for mode in modes:
        file_name = f"output_4096k_8_{mode}_{parameter}.txt"
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
                    if res[0] == f"{access}":
                        curr_iter = int(res[10])
                        if curr_iter < timesteps - 1: 
                            tmp_bw.append(float(res[9]))
                        else:
                            flag = False
                            tmp_bw.append(float(res[9]))
                            if mode == "base":
                                bw_base.append(sum(tmp_bw))
                            elif mode == "first":
                                bw_8M.append(sum(tmp_bw))
                            elif mode == "second":
                                bw_4M.append(sum(tmp_bw))
                            elif mode == "third":
                                bw_2M.append(sum(tmp_bw))
                            elif mode == "fourth":
                                bw_1M.append(sum(tmp_bw))
                            tmp_bw = []
    print(bw_base)
    array_data = np.array(bw_base)
    avg_bw_base = np.mean(array_data, axis=0)
    std_devs_base = np.std(array_data, axis=0)
    print(bw_8M)
    array_data = np.array(bw_8M)
    std_devs_8M = np.std(array_data, axis=0)
    avg_bw_8M = np.mean(array_data, axis=0)
    print(bw_4M)
    array_data = np.array(bw_4M)
    std_devs_4M = np.std(array_data, axis=0)
    avg_bw_4M = np.mean(array_data, axis=0)
    print(bw_2M)
    array_data = np.array(bw_2M)
    std_devs_2M = np.std(array_data, axis=0)
    avg_bw_2M = np.mean(array_data, axis=0)
    print(bw_1M)
    array_data = np.array(bw_1M)
    std_devs_1M = np.std(array_data, axis=0)
    avg_bw_1M = np.mean(array_data, axis=0)

    x = np.arange(0, timesteps)
    df_base = pd.DataFrame({'x': x, 'y': avg_bw_base, 'y_upper': avg_bw_base + std_devs_base, 'y_lower': avg_bw_base - std_devs_base})
    df_8M = pd.DataFrame({'x': x, 'y': avg_bw_8M, 'y_upper': avg_bw_8M + std_devs_8M, 'y_lower': avg_bw_8M - std_devs_8M})
    df_4M = pd.DataFrame({'x': x, 'y': avg_bw_4M, 'y_upper': avg_bw_4M + std_devs_4M, 'y_lower': avg_bw_4M - std_devs_4M})
    df_2M = pd.DataFrame({'x': x, 'y': avg_bw_2M, 'y_upper': avg_bw_2M + std_devs_2M, 'y_lower': avg_bw_2M - std_devs_2M})
    df_1M = pd.DataFrame({'x': x, 'y': avg_bw_1M, 'y_upper': avg_bw_1M + std_devs_1M, 'y_lower': avg_bw_1M - std_devs_1M})

    if parameter == "cb_config_list":
        for data, color, label in zip([(df_base, "red", "Default - cb_config_list *:1"), (df_8M, "blue", "cb_config_list *:2"), (df_4M, "brown", "cb_config_list *:4"), (df_2M, "green", "cb_config_list *:8"), (df_1M, "purple", "cb_config_list *:16")],
                                    ["red", "blue", "brown", "green", "purple"],
                                    ["Default - cb_config_list *:1", "cb_config_list *:2", "cb_config_list *:4", "cb_config_list *:8", "cb_config_list *:16"]):
            sns.lineplot(x=data[0]["x"], y=data[0]["y"], ax=axes[index], label=label, color=color, legend=False)
    elif parameter == "cb_buffer_size":
        for data, color, label in zip([(df_base, "red", "Default - cb_buffer_size=16M"), (df_8M, "blue", "cb_buffer_size=8M"), (df_4M, "brown", "cb_buffer_size=4M"), (df_2M, "green", "cb_buffer_size=2M")],
                            ["red", "blue", "brown", "green"],
                            ["Default - cb_buffer_size=16M", "cb_buffer_size=8M", "cb_buffer_size=4M", "cb_buffer_size=2M"]):
            sns.lineplot(x=data[0]["x"], y=data[0]["y"], ax=axes[index], label=label, color=color, legend=False)
    elif parameter == "romio_cb_write":
        for data, color, label in zip([(df_base, "red", "Default - romio_cb_write=automatic"), (df_8M, "blue", "romio_cb_write=enable"), (df_4M, "brown", "romio_cb_write=disable")],
                            ["red", "blue", "brown"],
                            ["Default - romio_cb_write=automatic", "romio_cb_write=enable", "romio_cb_write=disable"]):
            sns.lineplot(x=data[0]["x"], y=data[0]["y"], ax=axes[index], label=label, color=color, legend=False)
        # plt.fill_between(data[0]["x"], data[0]["y_lower"], data[0]["y_upper"], color=color, alpha=0.3)
    elif parameter == "romio_ds_write":
        for data, color, label in zip([(df_base, "red", "Default - romio_ds_write=automatic"), (df_8M, "blue", "romio_ds_write=enable"), (df_4M, "brown", "romio_ds_write=disable")],
                            ["red", "blue", "brown"],
                            ["Default - romio_ds_write=automatic", "romio_ds_write=enable", "romio_ds_write=disable"]):
            sns.lineplot(x=data[0]["x"], y=data[0]["y"], ax=axes[index], label=label, color=color, legend=False)
    elif parameter == "ind_wr_buffer_size":
        for data, color, label in zip([(df_base, "red", "Default - ind_wr_buffer_size=512K"), (df_8M, "blue", "ind_wr_buffer_size=256K"), (df_4M, "brown", "ind_wr_buffer_size=1M"), (df_2M, "green", "ind_wr_buffer_size=2M"), (df_1M, "purple", "ind_wr_buffer_size=4M")],
                            ["red", "blue", "brown", "green", "purple"],
                            ["Default - ind_wr_buffer_size=512K", "ind_wr_buffer_size=256K", "ind_wr_buffer_size=1M", "ind_wr_buffer_size=2M", "ind_wr_buffer_size=4M"]):
            sns.lineplot(x=data[0]["x"], y=data[0]["y"], ax=axes[index], label=label, color=color, legend=False)
    elif parameter == "romio_ds_read":
        for data, color, label in zip([(df_base, "red", "Default - romio_ds_read=automatic"), (df_8M, "blue", "romio_ds_read=enable"), (df_4M, "brown", "romio_ds_read=disable")],
                            ["red", "blue", "brown"],
                            ["Default - romio_ds_read=automatic", "romio_ds_read=enable", "romio_ds_read=disable"]):
            sns.lineplot(x=data[0]["x"], y=data[0]["y"], ax=axes[index], label=label, color=color, legend=False)
    elif parameter == "ind_rd_buffer_size":
        for data, color, label in zip([(df_base, "red", "Default - ind_rd_buffer_size=4M"), (df_8M, "blue", "ind_rd_buffer_size=2M"), (df_4M, "brown", "ind_rd_buffer_size=1M"), (df_2M, "green", "ind_rd_buffer_size=8M"), (df_1M, "purple", "ind_rd_buffer_size=16M")],
                            ["red", "blue", "brown", "green", "purple"],
                            ["Default - ind_rd_buffer_size=4M", "ind_rd_buffer_size=2M", "ind_rd_buffer_size=1M", "ind_rd_buffer_size=8M", "ind_rd_buffer_size=16M"]):
            sns.lineplot(x=data[0]["x"], y=data[0]["y"], ax=axes[index], label=label, color=color, legend=False)


    axes[index].set_xlabel("Timesteps", fontsize=14)
    if index == 0:
        axes[index].set_ylabel("BW (MB/s)", fontsize=14)

    axes[index].legend(loc="upper center", bbox_to_anchor=(0.5, -0.16), ncol=2, fontsize=8)
    specific_x_values = [0, 1, 2, 3, 4]  # Modify based on your dataset
    axes[index].set_xticks(specific_x_values)
    # axes[index].legend(loc="upper right")
    index += 1



# # axes[index].set_xlabel("Timesteps", fontsize=14)
# axes[index].set_title(f"{node} Nodes, {node*40} Processes", fontsize=16)
# index += 1

plt.subplots_adjust(wspace=-0.5)  # Reduces space between subplotsplt.tight_layout()
plt.tight_layout()
plt.savefig("results.png", dpi=300, bbox_inches="tight")
# plt.savefig("Bandwidths")
 """

import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

## romio_ds_read
timesteps = 5
sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5)
sns.set_style("darkgrid")
sns.set_palette('colorblind')
sns.color_palette("muted")

plt.figure(figsize=(8,5))
parameter = "romio_ds_read"
access = 'read'
modes = ["base", "first", "second"]
categories = ["Default - romio_ds_read=automatic", "romio_ds_read=enable", "romio_ds_read=disable"]
custom_colors = ["#1f77b4", "#ff7f0e", "#008000"]  # Blue, Orange, Green

time_base = []
time_enable = []
time_disable = []

for mode in modes:
    file_name = f"output_4096k_8_{mode}_{parameter}.txt"
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
                if res[0] == f"{access}":
                    curr_iter = int(res[10])
                    if curr_iter < timesteps - 1: 
                        tmp_bw.append(float(res[9]))
                    else:
                        flag = False
                        tmp_bw.append(float(res[9]))
                        if mode == "base":
                            time_base.append(sum(tmp_bw))
                        elif mode == "first":
                            time_enable.append(sum(tmp_bw))
                        elif mode == "second":
                            time_disable.append(sum(tmp_bw))
                        tmp_bw = []
print(time_base)
array_data = np.array(time_base)
std_time_base = np.std(array_data, axis=0)
avg_time_base = np.mean(array_data, axis=0)

print(time_enable)
array_data = np.array(time_enable)
std_time_enable = np.std(array_data, axis=0)
avg_time_enable = np.mean(array_data, axis=0)
print(time_disable)
array_data = np.array(time_disable)
std_time_disable = np.std(array_data, axis=0)
avg_time_disable = np.mean(array_data, axis=0)

times_1 = [avg_time_base, avg_time_enable, avg_time_disable]
variance_1 = [std_time_base, std_time_enable, std_time_disable]
sns.barplot(x=categories, y=times_1, yerr=variance_1, 
                    capsize=5, palette=custom_colors, edgecolor="black", linewidth=1.0)

for i, (bandwidth, error) in enumerate(zip(times_1, variance_1)):
    plt.text(i, bandwidth + 0.3, f"±{round(error, 3)}", ha='center', fontsize=11, fontweight='bold')

plt.ylabel("Execution Time (s)")

plt.xticks([])
plt.xlabel("")
handles = [plt.Rectangle((0, 0), 1, 1, color=color, edgecolor="black") for color in custom_colors]
plt.legend(handles, categories, loc="upper center", ncol=2, fontsize=9, frameon=True, bbox_to_anchor=(0.5, -0.05))
plt.tight_layout(rect=[0, 0, 1, 0.93])  # Leave space for legend
plt.savefig(f"{parameter}.png", dpi=300, bbox_inches="tight")

## romio_ds_write
timesteps = 5
sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5)
sns.set_style("darkgrid")
sns.set_palette('colorblind')
sns.color_palette("muted")

plt.figure(figsize=(8,5))
parameter = "romio_ds_write"
access = 'write'
modes = ["base", "first", "second"]
categories = ["Default - romio_ds_write=automatic", "romio_ds_write=enable", "romio_ds_write=disable"]
custom_colors = ["#1f77b4", "#ff7f0e", "#008000"]  # Blue, Orange, Green

time_base = []
time_enable = []
time_disable = []

for mode in modes:
    file_name = f"output_4096k_8_{mode}_{parameter}.txt"
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
                if res[0] == f"{access}":
                    curr_iter = int(res[10])
                    if curr_iter < timesteps - 1: 
                        tmp_bw.append(float(res[9]))
                    else:
                        flag = False
                        tmp_bw.append(float(res[9]))
                        if mode == "base":
                            time_base.append(sum(tmp_bw))
                        elif mode == "first":
                            time_enable.append(sum(tmp_bw))
                        elif mode == "second":
                            time_disable.append(sum(tmp_bw))
                        tmp_bw = []
print(time_base)
array_data = np.array(time_base)
std_time_base = np.std(array_data, axis=0)
avg_time_base = np.mean(array_data, axis=0)

print(time_enable)
array_data = np.array(time_enable)
std_time_enable = np.std(array_data, axis=0)
avg_time_enable = np.mean(array_data, axis=0)
print(time_disable)
array_data = np.array(time_disable)
std_time_disable = np.std(array_data, axis=0)
avg_time_disable = np.mean(array_data, axis=0)

times_1 = [avg_time_base, avg_time_enable, avg_time_disable]
variance_1 = [std_time_base, std_time_enable, std_time_disable]
sns.barplot(x=categories, y=times_1, yerr=variance_1, 
                    capsize=5, palette=custom_colors, edgecolor="black", linewidth=1.0)

for i, (bandwidth, error) in enumerate(zip(times_1, variance_1)):
    plt.text(i, bandwidth + 0.3, f"±{round(error, 3)}", ha='center', fontsize=11, fontweight='bold')

plt.ylabel("Execution Time (s)")

plt.xticks([])
plt.xlabel("")
handles = [plt.Rectangle((0, 0), 1, 1, color=color, edgecolor="black") for color in custom_colors]
plt.legend(handles, categories, loc="upper center", ncol=2, fontsize=9, frameon=True, bbox_to_anchor=(0.5, -0.05))
plt.tight_layout(rect=[0, 0, 1, 0.93])  # Leave space for legend
plt.savefig(f"{parameter}.png", dpi=300, bbox_inches="tight")

## ind_rd_buffer_size

timesteps = 5
sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5)
sns.set_style("darkgrid")
sns.set_palette('colorblind')
sns.color_palette("muted")

plt.figure(figsize=(8,5))
parameter = "ind_rd_buffer_size"
access = 'read'
modes = ["base", "first", "second", "third", "fourth"]
categories = ["Default - ind_rd_buffer_size=4M", "ind_rd_buffer_size=2M", "ind_rd_buffer_size=1M", "ind_rd_buffer_size=8M", "ind_rd_buffer_size=16M"]
custom_colors = ["#1f77b4", "#ff7f0e", "#008000", "#d62728", "#9467bd"]  # Blue, Orange, Green

time_base = []
time_first = []
time_second = []
time_third = []
time_fourth = []

for mode in modes:
    file_name = f"output_4096k_8_{mode}_{parameter}.txt"
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
                if res[0] == f"{access}":
                    curr_iter = int(res[10])
                    if curr_iter < timesteps - 1: 
                        tmp_bw.append(float(res[9]))
                    else:
                        flag = False
                        tmp_bw.append(float(res[9]))
                        if mode == "base":
                            time_base.append(sum(tmp_bw))
                        elif mode == "first":
                            time_first.append(sum(tmp_bw))
                        elif mode == "second":
                            time_second.append(sum(tmp_bw))
                        elif mode == "third":
                            time_third.append(sum(tmp_bw))
                        elif mode == "fourth":
                            time_fourth.append(sum(tmp_bw))
                        tmp_bw = []
print(time_base)
array_data = np.array(time_base)
std_time_base = np.std(array_data, axis=0)
avg_time_base = np.mean(array_data, axis=0)

print(time_first)
array_data = np.array(time_first)
std_time_first = np.std(array_data, axis=0)
avg_time_first = np.mean(array_data, axis=0)

print(time_second)
array_data = np.array(time_second)
std_time_second = np.std(array_data, axis=0)
avg_time_second = np.mean(array_data, axis=0)

print(time_third)
array_data = np.array(time_third)
std_time_third = np.std(array_data, axis=0)
avg_time_third = np.mean(array_data, axis=0)

print(time_fourth)
array_data = np.array(time_fourth)
std_time_fourth = np.std(array_data, axis=0)
avg_time_fourth = np.mean(array_data, axis=0)

times_1 = [avg_time_base, avg_time_first, avg_time_second, avg_time_third, avg_time_fourth ]
variance_1 = [std_time_base, std_time_first, std_time_second, std_time_third, std_time_fourth]
sns.barplot(x=categories, y=times_1, yerr=variance_1, 
                    capsize=5, palette=custom_colors, edgecolor="black", linewidth=1.0)

for i, (bandwidth, error) in enumerate(zip(times_1, variance_1)):
    plt.text(i, bandwidth + 0.3, f"±{round(error, 3)}", ha='center', fontsize=11, fontweight='bold')

plt.ylabel("Execution Time (s)")

plt.xticks([])
plt.xlabel("")
handles = [plt.Rectangle((0, 0), 1, 1, color=color, edgecolor="black") for color in custom_colors]
plt.legend(handles, categories, loc="upper center", ncol=2, fontsize=9, frameon=True, bbox_to_anchor=(0.5, -0.05))
plt.tight_layout(rect=[0, 0, 1, 0.93])  # Leave space for legend
plt.savefig(f"{parameter}.png", dpi=300, bbox_inches="tight")

## ind_wr_buffer_size

timesteps = 5
sns.set_theme(style="whitegrid")
sns.set_context("paper", font_scale=1.5)
sns.set_style("darkgrid")
sns.set_palette('colorblind')
sns.color_palette("muted")

plt.figure(figsize=(8,5))
parameter = "ind_wr_buffer_size"
access = 'write'
modes = ["base", "first", "second", "third", "fourth"]
categories = ["Default - ind_wr_buffer_size=512K", "ind_wr_buffer_size=256K", "ind_wr_buffer_size=1M", "ind_wr_buffer_size=2M", "ind_wr_buffer_size=4M"]
custom_colors = ["#1f77b4", "#ff7f0e", "#008000", "#d62728", "#9467bd"]  # Blue, Orange, Green

time_base = []
time_first = []
time_second = []
time_third = []
time_fourth = []

for mode in modes:
    file_name = f"output_4096k_8_{mode}_{parameter}.txt"
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
                if res[0] == f"{access}":
                    curr_iter = int(res[10])
                    if curr_iter < timesteps - 1: 
                        tmp_bw.append(float(res[9]))
                    else:
                        flag = False
                        tmp_bw.append(float(res[9]))
                        if mode == "base":
                            time_base.append(sum(tmp_bw))
                        elif mode == "first":
                            time_first.append(sum(tmp_bw))
                        elif mode == "second":
                            time_second.append(sum(tmp_bw))
                        elif mode == "third":
                            time_third.append(sum(tmp_bw))
                        elif mode == "fourth":
                            time_fourth.append(sum(tmp_bw))
                        tmp_bw = []
print(time_base)
array_data = np.array(time_base)
std_time_base = np.std(array_data, axis=0)
avg_time_base = np.mean(array_data, axis=0)

print(time_first)
array_data = np.array(time_first)
std_time_first = np.std(array_data, axis=0)
avg_time_first = np.mean(array_data, axis=0)

print(time_second)
array_data = np.array(time_second)
std_time_second = np.std(array_data, axis=0)
avg_time_second = np.mean(array_data, axis=0)

print(time_third)
array_data = np.array(time_third)
std_time_third = np.std(array_data, axis=0)
avg_time_third = np.mean(array_data, axis=0)

print(time_fourth)
array_data = np.array(time_fourth)
std_time_fourth = np.std(array_data, axis=0)
avg_time_fourth = np.mean(array_data, axis=0)

times_1 = [avg_time_base, avg_time_first, avg_time_second, avg_time_third, avg_time_fourth ]
variance_1 = [std_time_base, std_time_first, std_time_second, std_time_third, std_time_fourth]
sns.barplot(x=categories, y=times_1, yerr=variance_1, 
                    capsize=5, palette=custom_colors, edgecolor="black", linewidth=1.0)

for i, (bandwidth, error) in enumerate(zip(times_1, variance_1)):
    plt.text(i, bandwidth + 0.3, f"±{round(error, 3)}", ha='center', fontsize=11, fontweight='bold')

plt.ylabel("Execution Time (s)")

plt.xticks([])
plt.xlabel("")
handles = [plt.Rectangle((0, 0), 1, 1, color=color, edgecolor="black") for color in custom_colors]
plt.legend(handles, categories, loc="upper center", ncol=2, fontsize=9, frameon=True, bbox_to_anchor=(0.5, -0.05))
plt.tight_layout(rect=[0, 0, 1, 0.93])  # Leave space for legend
plt.savefig(f"{parameter}.png", dpi=300, bbox_inches="tight")