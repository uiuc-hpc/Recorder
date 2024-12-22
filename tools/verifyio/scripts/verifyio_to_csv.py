import csv
import re
import argparse
import pandas as pd


def parser(path, dir_prefix=None):
    with open(path, "r") as file:
        log_data = file.read()

    data = []
    dir_regex = re.compile(r"Entering directory:\s+(.+)")
    io_time_regex = re.compile(r"Step 1. read trace records and conflicts time:\s+([\d.]+)\s+secs")
    mpi_calls_regex = re.compile(r"Step 2. match mpi calls:\s+([\d.]+)\s+secs, mpi edges:\s+(\d+)")
    happens_before_regex = re.compile(r"Step 3. build happens-before graph:\s+([\d.]+)\s+secs, nodes:\s+(\d+)")
    run_the_algorithm_regex = re.compile(r"Step 4. run vector clock algorithm:\s+([\d.]+)\s+secs")
    verification_regex = re.compile(r"semantics verification time:\s+(\d+)")
    semantic_violation_regex = re.compile(r"Total semantic violations:\s+(\d+)")
    conflict_pairs_regex = re.compile(r"Total conflict pairs:\s+(\d+)")
    ap_regex = re.compile(r"Finished\s+--semantic=(POSIX|MPI-IO|Commit|Session)")  

    lines = log_data.strip().split("\n")
    entry = {}
    for line in lines:
        line = line.strip()
        if dir_match := dir_regex.match(line):
            if entry:
                data.append(entry)
            match_str = dir_match.group(1)
            if dir_prefix is None:
                entry = {"directory_name": match_str}
            else:
                split_str = match_str.split(dir_prefix)
                if len(split_str) > 1:
                    entry = {"directory_name": split_str[1]}
                else:
                    entry = {"directory_name": None} 
        elif io_time_match := io_time_regex.search(line):
            entry["io_time"] = io_time_match.group(1)
        elif mpi_calls_match := mpi_calls_regex.search(line):
            entry["match_mpi_calls"] = mpi_calls_match.group(1)
            entry["mpi_edges"] = mpi_calls_match.group(2)
        elif happens_before_match := happens_before_regex.search(line):
            entry["build_happens-before_graph"] = happens_before_match.group(1)
            entry["nodes"] = happens_before_match.group(2)
        elif run_the_algorithm_match := run_the_algorithm_regex.search(line):
            entry["run_the_algorithm"] = run_the_algorithm_match.group(1) 
        elif verification_match := verification_regex.search(line):
            entry["verification_time"]= verification_match.group(1) 
        elif semantic_violation_match := semantic_violation_regex.search(line):
            entry["total_semantic_violation"] = semantic_violation_match.group(1)
        elif conflict_pairs_match := conflict_pairs_regex.search(line):
            entry["total_conflict_pairs"] = conflict_pairs_match.group(1)
        elif ap_match := ap_regex.search(line):
            entry["api"] = ap_match.group(1)
    if entry:
        data.append(entry)

    return data


def to_api_format(data):
    df = pd.DataFrame(data, columns=[
        'directory_name', 'io_time', 'match_mpi_calls', 'mpi_edges', 'build_happens-before_graph', 'nodes', 
        'run_the_algorithm', 'verification_time', 'total_semantic_violation', 'total_conflict_pairs', 'api'
    ])
    df_no_conflicts = df.drop(columns='total_conflict_pairs')
    df_pivot = df_no_conflicts.pivot(index='directory_name', columns='api')
    df_pivot.columns = ['_'.join(col).strip() for col in df_pivot.columns.values]
    df_pivot = df_pivot.reset_index()
    df_total_conflicts = df[['directory_name', 'total_conflict_pairs']].drop_duplicates()
    df_pivot = pd.merge(df_pivot, df_total_conflicts, on='directory_name')
    return df_pivot



def write_to_csv(data, csv_file):
    with open(csv_file, "a", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=["directory_name", "io_time", "match_mpi_calls", "mpi_edges", "build_happens-before_graph", "nodes", "run_the_algorithm", "verification_time", "total_semantic_violation", "total_conflict_pairs", "api"])
        writer.writeheader()
        writer.writerows(data)

    print(f"Data has been written to {csv_file}")

def write_df_to_csv(df, csv_file):
    df.to_csv(csv_file, index=False)
    print(f"Data has been written to {csv_file}")

if __name__ == '__main__':
    arg_parser = argparse.ArgumentParser(description="Parse log file and export data to CSV")
    arg_parser.add_argument("path", type=str, help="Path to the log file")
    arg_parser.add_argument("csv_file", type=str, help="Name of the CSV file to save the data")
    arg_parser.add_argument("--dir_prefix", type=str, help="Remove the directory prefix", required=False)
    arg_parser.add_argument("--group_by_api", action="store_true", help="Group data by API", required=False)

    args = arg_parser.parse_args()
    
    parsed_data = parser(args.path, args.dir_prefix)
    if args.group_by_api:
        to_api_format(parsed_data)
        write_df_to_csv(to_api_format(parsed_data), args.csv_file)
    else:
        write_to_csv(parsed_data, args.csv_file)   