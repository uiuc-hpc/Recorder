import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import numpy as np

# Define timesteps
timesteps = ["T0", "T1", "T2", "T3", "T4", "T5", "T6"]

# Compute speedups using the provided baseline and optimized execution times
def compute_speedup(baseline, optimized):
    return optimized /baseline 

# C1 Write Speedup
speedup_c1_write_8 = compute_speedup(
    np.array([1241.195, 1276.22, 1243.295, 1235.555, 1255.62, 1283.6, 1273.975]),
    np.array([1196.305, 12398.5, 10254.5, 9575.5, 10351., 10330., 10962.5])
)
speedup_c1_write_16 = compute_speedup(
    np.array([3927., 4112.5, 4080.5, 3943.5, 4056.5, 3926., 4037.5]),
    np.array([3802.5, 17024.5, 17757., 18427.5, 17900., 18734.5, 18746.5])
)
speedup_c1_write_32 = compute_speedup(
    np.array([4775., 4808.5, 4742.5, 4779.5, 4678., 4735.5, 4700.]),
    np.array([4508.5, 36877.5, 24907.5, 25161., 25065., 24825., 25670.5])
)

# C1 Read Speedup
speedup_c1_read_8 = compute_speedup(
    np.array([17001., 17082., 16849.5, 17048.5, 16821.5, 16961., 17018.5]),
    np.array([16372.5, 205513.5, 122047.5, 114558.5, 125624.5, 119646.5, 114692.])
)
speedup_c1_read_16 = compute_speedup(
    np.array([23727.5, 23273., 23439., 23805.5, 23729.5, 23588.5, 23936.]),
    np.array([23474.5, 29712., 216973., 212868.5, 202841., 208813., 206459.5])
)
speedup_c1_read_32 = compute_speedup(
    np.array([27802., 27821.5, 27524., 27544.5, 27739., 27865., 27408.5]),
    np.array([28148., 331428.5, 333559.5, 349637., 320792., 329722., 325801.])
)

# C2 Write Speedup
speedup_c2_write_8 = compute_speedup(
    np.array([2942.375, 2883.75, 2847.945, 2721.435, 2765.735, 2678.745, 2643.31]),
    np.array([2937.22, 27579., 28861.5, 31376., 32037., 31503.5, 32156.5])
)
speedup_c2_write_16 = compute_speedup(
    np.array([5062., 4883.5, 5037.5, 4663.5, 4832., 4592., 4789.]),
    np.array([4957.5, 43529.5, 44703.5, 43275.5, 44472.5, 45468., 45493.5])
)
speedup_c2_write_32 = compute_speedup(
    np.array([7181.5, 6958., 6453., 6594.5, 6287., 6344.5, 6403.]),
    np.array([7514., 55857.5, 56059.5, 56574.5, 54236., 56519.5, 54965.5])
)

# C2 Read Speedup
speedup_c2_read_8 = compute_speedup(
    np.array([121238.5, 120982., 124172., 123701., 122222., 118465., 120262.5]),
    np.array([116116.5, 322921.5, 337345., 312109.5, 317957.5, 329811.5, 326054.])
)
speedup_c2_read_16 = compute_speedup(
    np.array([224187., 227730., 227556.5, 220601.5, 219692., 220782., 215744.]),
    np.array([224359., 582414., 611935., 597673., 612766.5, 608192., 598657.5])
)
speedup_c2_read_32 = compute_speedup(
    np.array([335432., 340152., 295842.5, 346074., 292683.5, 332390., 293067.5]),
    np.array([320711.5, 1049923.5, 1029248.5, 1072497., 1046665., 1059696., 1066442.5])
)

# Prepare DataFrame for Seaborn
df_speedup = pd.DataFrame({
    "Timestep": timesteps * 12,
    "Speedup (×)": np.concatenate([
        speedup_c1_write_8, speedup_c1_write_16, speedup_c1_write_32,
        speedup_c1_read_8, speedup_c1_read_16, speedup_c1_read_32,
        speedup_c2_write_8, speedup_c2_write_16, speedup_c2_write_32,
        speedup_c2_read_8, speedup_c2_read_16, speedup_c2_read_32
    ]),
    "Configuration": (["C1 Write 8N"] * 7 + ["C1 Write 16N"] * 7 + ["C1 Write 32N"] * 7 +
                      ["C1 Read 8N"] * 7 + ["C1 Read 16N"] * 7 + ["C1 Read 32N"] * 7 +
                      ["C2 Write 8N"] * 7 + ["C2 Write 16N"] * 7 + ["C2 Write 32N"] * 7 +
                      ["C2 Read 8N"] * 7 + ["C2 Read 16N"] * 7 + ["C2 Read 32N"] * 7)
})

# Plot using Seaborn with Baseline Line
plt.figure(figsize=(18, 8))
sns.barplot(data=df_speedup, x="Timestep", y="Speedup (×)", hue="Configuration", palette="tab10")

# Add a horizontal line at speedup = 1 to indicate the baseline
plt.axhline(y=1, color='black', linestyle='--', linewidth=2, label="Baseline (1×)")

# Formatting
plt.xlabel("Timestep", fontsize=18 )
plt.ylabel("Speedup (×)", fontsize=18)
plt.xticks(fontsize=18)
plt.yticks(fontsize=18)
plt.legend(loc='upper left', fontsize=17)
plt.grid(axis='y', linestyle='solid', alpha=0.7)

# Save the plot
plt.tight_layout()
plt.savefig("IOR_BW_Speedup.png", format="png", bbox_inches="tight")
plt.savefig("IOR_BW_Speedup.pdf", format="pdf", bbox_inches="tight")
plt.show()
