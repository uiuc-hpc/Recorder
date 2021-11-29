
### Verify if I/O operations are properly synchronized uder specific semantics
------------------------------



Suppose `$RECORDER_DIR` is the install location of Recorder.

Dependencies: `recorder-viz` and `networkx`. Both can be installed using pip.

Steps:
1. Run program with Recorder to generate traces.
2. Run the conflict detector to report **potential** conflicting I/O accesses.
   Those acesses are only potentially conflicting as here we do not take happens-before order into consideration yet.

   `$RECORDER_DIR/bin/conflict_detector /path/to/traces --semantics=posix`
   
   The `semantics` option needs to match the one provided by the underlying file system. For example, if the traces were collected on UnifyFS, set it to "commit".
   
   This command will write all potential conflicts found to the file `/path/to/traces/conflicts.txt`
   
3. Finally run the verification code, which checks if those potential conflicting operations are properly synchronzied.
   
   ```python
   python ./verifyio.py -h  # print out usage
   
   #Example:
   python ./verifyio.py /path/to/traces /path/to/traces/conflicts.txt --semantics=mpi
   ```
   
   
   
#### Note on the third step:

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
