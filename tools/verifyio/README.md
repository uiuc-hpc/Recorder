# VerifyIO
VerifyIO collects execution traces, detects data conflicts, and verifies proper synchronization against specified consistency models. 
The workflow for VerifyIO can generally be divided into three independent steps (Step 1 - Step 3). This process requires [Recorder](https://github.com/uiuc-hpc/Recorder/tree/dev), [VerifyIO](https://github.com/uiuc-hpc/Recorder/tree/dev/tools/verifyio), and the corresponding traces.

Make sure `$RECORDER_INSTALL_PATH` is the install location of Recorder. 

Additional [scripts](https://github.com/uiuc-hpc/Recorder/blob/dev/tools/verifyio/scripts/) are available for exporting results as CSV files and visualizing them (Step 4 and Step 5). The verification can be performed in a batched manner for multiple traces. For this, alternative steps can be followed to handle multiple traces more efficiently.

## Step 1:  Run the application with Recorder to generate traces.

```bash
mpirun -np N -env LD_PRELOAD $RECORDER_INSTALL_PATH/lib/librecorder.so ./test_mpi

# On HPC systems, you may need to use srun or
# other job schedulers to replace mpirun, e.g.,
srun -n4 -N1 --overlap --export=ALL,LD_PRELOAD=$RECORDER_INSTALL_PATH/lib/librecorder.so ./test_mpi
```
For detailed information on the Recorder and guidance on its usage, please refer to: https://recorder.readthedocs.io/latest/overview.html

## Step 2: Conflict Detection
Run the conflict detector to report **potential** conflicting I/O accesses. Those acesses are only potentially conflicting as here we do not take happens-before order into consideration yet.
To detect conflicts, use the `conflict-detector` tool from Recorder:

```bash
$RECORDER_INSTALL_PATH/bin/conflict-detector /path/to/trace
```
This command will write all potential conflicts found to the file `/path/to/traces/conflicts.txt`

Alternatively, the [`auto_detect.sh`](https://github.com/uiuc-hpc/Recorder/blob/dev/tools/verifyio/scripts/auto_detect.sh)  script can be used to process multiple traces and detect conflicts. The script requires the path to the directory containing the traces as an argument. Please adjust the configuartion for resource manager accordingly. 

```bash
./auto_detect.sh /path/to/target/traces
```

## Step 3: Semantic Verification
The next step is to run the semantic verification using [`verifyio.py`](https://github.com/uiuc-hpc/Recorder/tree/dev/tools/verifyio). It checks if those potential conflicting operations are properly synchronzied. By default, MPI-IO semantics and the vector clock algorithm are used for verification.

### Dependencies for step 3:

Ensure the following dependencies are installed:

#### Python Libraries
- **recorder-viz**:  For visualizing recorder traces. `pip install recorder-viz`
- **networkx**: For creating, analyzing, and manipulating complex networks and graphs. Install using `pip install networkx`.

```bash
python /path/to/verifyio.py /path/to/trace
```
Available arguments:
* --semantics: Specifies the I/O semantics to verify. Choices are: POSIX, MPI-IO (default), Commit, Session, Custom
* --algorithm: Specifies the algorithm for verification. Choices are: 1: Graph reachability 2: Transitive closure 3: Vector clock (default) 4: On-the-fly MPI check
* --semantic_string: A custom semantic string for verification. Default is: "c1:+1[MPI_File_close, MPI_File_sync] & c2:-1[MPI_File_open, MPI_File_sync]""
* --show_details: Displays details of the conflicts.
* --show_summary: Displays a summary of the conflicts.
* --show_full_chain: Displays the full call chain of the conflicts.

Alternatively, the [`auto_verify.sh`](https://github.com/uiuc-hpc/Recorder/blob/dev/tools/verifyio/scripts/auto_verify.sh) script can be used for verification. This script requires the path to the directory containing the traces, and the path to verifyio as arguments. By default, it verifies all supported semantics, i.e., POSIX, MPI-IO, Commit, Session with Vector clock algorithm.

```bash
./auto_verify.sh /path/to/target/traces /path/to/verifyio.py
```

#### Note on step 3:

 The code first matches all MPI calls to build a graph representing the happens-before order. Each node in the graph represents a MPI call, if there is a path from node A to node B, then A must happens-before B. 

   Given a conflicing I/O pair of accesses (op1, op2). Using the graph, we can figure out if op1 happens-before op2. If so, they are properly synchronzied.
   This works if we assume the POSIX semantics. E.g., op1(by rank1)->send(by rank1)->recv(by rank2)->op2(by rank2), this path tells us op1 and op2 are properly synchronized.
   
However, things are a little different with default MPI user-imposed semantics (i.e., nonatomic mode). According to the MPI standard, many collective calls do not  guarantee the synchronization beteen the involved processes. The standard explictly says the following collectives are guaranteed to be synchronized:
 - MPI_Barrier
 - MPI_Allgather
 - MPI_Alltoall and their V and W variants
 - MPI_Allreduce
 - MPI_Reduce_scatter
 - MPI_Reduce_scatter_block

With user-imposed semantics, the **"sync-barrier-sync"** construct is required to guarnatee sequencial consistency. Barrier can be replaced by a send-recv or the collectives listed above. Sync is one of MPI_File_open, MPI_File_close or MPI_File_sync.

Now, given two potential conflicting accesses op1 an op2, our job is to find out if there is a **sync-barrier-sync** in between.


## Step 4: Export Results to CSV

### Dependencies for step 4 & 5:

Ensure the following dependencies are installed:

#### Python Libraries
- **argparse**: For parsing command-line arguments.
- **pandas**: For data manipulation and analysis. Install using `pip install pandas`.
- **seaborn**: For advanced data visualization. Install using `pip install seaborn`.
- **matplotlib**: For creating static, animated, and interactive plots. Install using `pip install matplotlib`.
- **numpy**: For numerical computations. Install using `pip install numpy`.

VerifyIO results can be exported to a CSV format for further analysis by using [`verifyio_to_csv.py `](https://github.com/uiuc-hpc/Recorder/blob/dev/tools/verifyio/scripts/verifyio_to_csv.py). Use the following command, providing the output file (usually stdout from VerifyIO execution) as an argument:

```bash
python verifyio_to_csv.py /path/to/verifyio/output /path/to/output.csv
```
Optional arguments:
* --dir_prefix="/prefix/to/remove": Removes a specified prefix from the paths in the output.
* --group_by_api: Groups the output by API calls.

## Step 5: Heatmap Visualization

To visualize the results from VerifyIO, use the [`verifyio_plot_heatmap.py`](https://github.com/uiuc-hpc/Recorder/blob/dev/tools/verifyio/scripts/verifyio_plot_violation_heatmap.py) script:

```bash
python verifyio_plot_heatmap.py --file=/path/to/output.csv
```
Alternatively, for visualizing up to three CSV files in a single heatmap, use the following argument:

```bash
python verifyio_plot_heatmap.py --files="/path/to/output1.csv" "/path/to/output2.csv" "/path/to/output3.csv"
```

