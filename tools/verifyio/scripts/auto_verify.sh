#!/bin/bash


#SBATCH -N 1
#SBATCH -J jobname
#SBATCH -t 01:00:00
#SBATCH -p partition
#SBATCH -o /path/to/out.out


if [ -z "$1" ]; then
    echo "Usage: $0 <base_directory>"
    exit 1
fi

BASE_DIR="$1"
PROGRAM="/path/to/verifyio.py"
SEMANTICS=("POSIX" "MPI-IO" "Commit" "Session")

for semantic in "${SEMANTICS[@]}"; do
    for dir in "$BASE_DIR"/*/; do
        if [ -d "$dir" ]; then
            echo "Entering directory: $dir"
            (
                cd "$dir" || exit
                python "$PROGRAM" "$dir" "--semantics=$semantic"
            )
            echo "Finished --semantic=$semantic"
        fi
    done
done
