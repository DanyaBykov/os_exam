#!/usr/bin/env python3
import sys
import subprocess
import argparse
import time
import os

CONFIG_FILENAME = "mpi_config.txt"
SHM_NAME = "/mpi_exam_shm"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", type=int, required=True)
    parser.add_argument("--mode", choices=["net", "shm"], default="net")
    parser.add_argument("command", nargs=argparse.REMAINDER)

    args = parser.parse_args()

    if not args.command:
        print("Error: No executable specified.")
        sys.exit(1)

    executable = args.command[0]
    user_args = args.command[1:]
    count = args.n

    print(f"Setting up {count} processes in '{args.mode}' mode ---")

    with open(CONFIG_FILENAME, "w") as f:
        if args.mode == "net":
            f.write("1\n")
            f.write(f"{count}\n")
            for _ in range(count):
                f.write("127.0.0.1\n")
        else:
            f.write("0\n")
            f.write(f"{count}\n")
            f.write(f"{SHM_NAME}\n")

    procs = []
    try:
        for rank in range(count):
            cmd = [executable, str(rank), CONFIG_FILENAME] + user_args
            
            p = subprocess.Popen(cmd)
            procs.append(p)
            print(f" -> Started Rank {rank} (PID: {p.pid})")

        print("Waiting for processes to finish ---")
        exit_codes = [p.wait() for p in procs]
        
        if any(code != 0 for code in exit_codes):
            print("Warning: Some processes failed! ---")
        else:
            print("Success! All processes finished. ---")

    except KeyboardInterrupt:
        print("\nInterrupted! Killing processes... ---")
        for p in procs:
            p.kill()
    
    finally:
        if os.path.exists(CONFIG_FILENAME):
            os.remove(CONFIG_FILENAME)

if __name__ == "__main__":
    main()