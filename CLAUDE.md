# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What Recorder Is

Recorder is a parallel I/O tracing library that intercepts I/O calls at multiple levels of the stack (POSIX, MPI-IO, HDF5, PnetCDF, NetCDF) without requiring application modification. It works via `LD_PRELOAD` to inject `librecorder.so` at runtime.

## Build Commands

```bash
# Configure (out-of-source build required)
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=$PREFIX

# Optional configure flags
-DRECORDER_WITH_HDF5=/path/to/hdf5
-DRECORDER_WITH_PNETCDF=/path/to/pnetcdf
-DRECORDER_WITH_NETCDF=/path/to/netcdf
-DRECORDER_ENABLE_CUDA_TRACE=ON
-DRECORDER_ENABLE_FCNTL_TRACE=ON     # default ON
-DRECORDER_ENABLE_PARQUET=ON

# Build
cmake --build build

# Run all tests (requires MPI)
cd build && ctest

# Run a single test
cd build && ctest -R test_mpi -V

# Install
cmake --build build --target install
```

HDF5 is detected automatically. PnetCDF and NetCDF require explicit paths via `-DRECORDER_WITH_*`.

## Repository Structure

- `lib/` — core library implementation
- `include/` — public headers; `recorder-gotcha.h` (~260 KB) is auto-generated from function lists
- `test/` — CTest-integrated test suite
- `tools/` — post-processing binaries (recorder2text, recorder2timeline, conflict-detector, etc.)
- `deps/GOTCHA/` — bundled function-hooking library (submodule)
- `tools/reporter/` — Python-based trace analysis
- `tools/generator/generator.py` — regenerates `recorder-gotcha.h` from function lists

All build outputs (libraries and binaries) go to `build/bin/`.

## Architecture

### Interception Flow

1. At program load, `__attribute__((constructor))` calls `recorder_init()` for non-MPI programs. For MPI programs, init is deferred to the `MPI_Init` wrapper.
2. `recorder_init()` calls `gotcha_init()` → registers ~300 function wrappers; `logger_init()` → opens per-rank trace files; `utils_init()` → sets up helpers.
3. Every wrapped function follows the same pattern using two macros in `recorder.h`:
   - `RECORDER_INTERCEPTOR_PROLOGUE` — saves timestamps, allocates a `Record`, calls the real function via GOTCHA
   - `RECORDER_INTERCEPTOR_EPILOGUE` — serializes arguments to strings, calls `logger_record_exit()`, returns to caller
4. At `MPI_Finalize` (or program exit), `recorder_finalize()` flushes and compresses trace data.

### Key Data Structures

- **`Record`** (`recorder-logger.h`) — one per intercepted call; holds `tstart`/`tend`, `func_id`, `arg_count`, `args[]`, `call_depth`, and a per-thread stack pointer for nested-call tracking.
- **`RecorderLogger`** — per-process singleton holding rank, record count, CST/CFG grammar state, and compression flags.
- **`RecorderMetadata`** — trace file header written once per rank; encodes version, feature flags, and MPI info.

### Trace Compression Pipeline

Recorder applies multiple compression passes:
1. **Sequitur** (`recorder-sequitur.c`) — detects repeated call sequences and encodes them as grammar rules (intra-process).
2. **CST/CFG** (`recorder-cst-cfg.c`) — context-sensitive and context-free grammar compression.
3. **Inter-process pattern recognition** — rank 0 coordinates cross-rank deduplication.
4. **Zlib** — applied to timestamps and structured data.

All compression is controlled at runtime via environment variables (see below).

### GOTCHA Wrappers

`recorder-gotcha.h` declares all wrapped functions using three macros:
- `GOTCHA_WRAP(func, ret_type, args)` — declares the wrapper and a handle
- `GOTCHA_WRAP_ACTION(func)` — creates the `gotcha_binding_t` entry
- `GOTCHA_REAL_CALL(func)` — invokes the original function through GOTCHA

`recorder-gotcha.c` populates `gotcha_binding_t` arrays per layer and calls `gotcha_wrap()` for each enabled layer during init. Layers can be disabled at runtime via `RECORDER_*_TRACING=0`.

## Runtime Configuration (Environment Variables)

```
RECORDER_TRACES_DIR             output directory for trace files
RECORDER_POSIX_TRACING          0/1 (default 1)
RECORDER_MPIIO_TRACING          0/1 (default 1)
RECORDER_MPI_TRACING            0/1 (default 0)
RECORDER_HDF5_TRACING           0/1 (default 1, no-op if built without HDF5)
RECORDER_PNETCDF_TRACING        0/1 (default 1)
RECORDER_NETCDF_TRACING         0/1 (default 1)
RECORDER_INTERPROCESS_COMPRESSION              0/1
RECORDER_INTERPROCESS_PATTERN_RECOGNITION      0/1
RECORDER_INTRAPROCESS_PATTERN_RECOGNITION      0/1
RECORDER_STORE_TID              record thread IDs
RECORDER_STORE_CALL_DEPTH       record call nesting depth
RECORDER_EXCLUSION_FILE         file of path prefixes to skip
RECORDER_INCLUSION_FILE         file of path prefixes to trace exclusively
RECORDER_WITH_NON_MPI=1         enable tracing for non-MPI programs
```

## Tests

CTest tests run each binary with `LD_PRELOAD` pointing to `librecorder.so` and isolated per-test `RECORDER_TRACES_DIR` directories under `build/test/traces/`.

| Test | Processes | Notes |
|---|---|---|
| `test_posix` | 1 | POSIX I/O |
| `test_iopr` | 2 | lseek + pwrite patterns |
| `test_mpi` | 4 | MPI collectives, point-to-point, MPI-IO |
| `test_hdf5` | 1 | HDF5 (only when HDF5_FOUND) |
| `test_phdf5` | 2 | Parallel HDF5 (only when HDF5_FOUND) |

`test_signal` and `test_hybrid` are compiled but excluded from CTest: `test_signal` is designed to receive a kill signal mid-run and crashes during normal exit; `test_hybrid` relies on a hardcoded `mpirun` path.
