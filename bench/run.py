#!/usr/bin/env python3
"""Compares the Kriol compiler's output with C and Python on small benchmarks.

Usage: bench/run.py [--kriol build/kriol] [--cc clang] [--runs 5]

Each benchmark exists as <name>.kriol, <name>.c and <name>.py and prints one
number; the script checks that every version prints the same one, and reports
the median wall time of several runs, the compile times and the program sizes.
"""

import argparse
import os
import platform
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path

BENCH_DIR = Path(__file__).resolve().parent
BENCHMARKS = ["fib", "crivo", "matriz", "mandelbrot"]
WASI_RUNNER = BENCH_DIR.parent / "tests" / "wasm" / "run_wasi.mjs"


def timed(command):
    """Runs a command and returns (seconds, stdout); fails on a non-zero exit."""
    start = time.perf_counter()
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    return time.perf_counter() - start, result.stdout.strip()


def median_run(command, runs):
    """The median wall time of several runs, and the output they all printed."""
    times, outputs = [], set()
    for _ in range(runs):
        seconds, output = timed(command)
        times.append(seconds)
        outputs.add(output)
    if len(outputs) != 1:
        sys.exit(f"{command[0]}: the output changes between runs: {outputs}")
    return statistics.median(times), outputs.pop()


def size_kib(path):
    return os.path.getsize(path) / 1024


def measure(name, args, work):
    """Compiles and runs every version of one benchmark."""
    source = BENCH_DIR / name
    kriol_exe, c_exe, wasm = work / f"{name}.kriol.bin", work / f"{name}.c.bin", work / f"{name}.wasm"

    row = {"name": name}
    row["kriol_compile"] = median_run([args.kriol, f"{source}.kriol", "-o", str(kriol_exe)], 3)[0]
    row["c_compile"] = median_run([args.cc, "-O2", f"{source}.c", "-o", str(c_exe)], 3)[0]
    row["kriol_size"], row["c_size"] = size_kib(kriol_exe), size_kib(c_exe)

    row["kriol"], expected = median_run([str(kriol_exe)], args.runs)
    runs = {"c": [str(c_exe)], "python": [sys.executable, f"{source}.py"]}
    if args.node:
        subprocess.run([args.kriol, f"{source}.kriol", "--target", "wasm32-wasi", "-o", str(wasm)],
                       check=True, capture_output=True)
        runs["wasm"] = [args.node, "--no-warnings", str(WASI_RUNNER), str(wasm)]

    for version, command in runs.items():
        row[version], output = median_run(command, args.runs)
        if output != expected:
            sys.exit(f"{name}: {version} printed {output!r}, Kriol printed {expected!r}")
    row["result"] = expected
    return row


def tool_version(command):
    return subprocess.run(command, capture_output=True, text=True).stdout.splitlines()[0].strip()


def cpu_name():
    try:
        with open("/proc/cpuinfo") as cpuinfo:
            for line in cpuinfo:
                if line.startswith("model name"):
                    return line.split(":", 1)[1].strip()
    except OSError:
        pass
    return platform.processor() or platform.machine()


def print_report(rows, args):
    print(f"# {cpu_name()}, {platform.system()} {platform.release()}")
    print(f"# {tool_version([args.kriol, '--version'])}, {tool_version([args.cc, '--version'])}")
    print(f"# Python {platform.python_version()}" + (f", Node {tool_version([args.node, '--version'])}" if args.node else ""))
    print(f"# median of {args.runs} runs, in seconds\n")

    columns = ["kriol", "c", "python"] + (["wasm"] if args.node else [])
    print("| benchmark | result | " + " | ".join(columns) + " | kriol/c | kriol compile | c compile | kriol KiB | c KiB |")
    print("|" + "---|" * (len(columns) + 7))
    for row in rows:
        times = " | ".join(f"{row[c]:.3f}" for c in columns)
        print(f"| {row['name']} | {row['result']} | {times} | {row['kriol'] / row['c']:.2f} "
              f"| {row['kriol_compile']:.3f} | {row['c_compile']:.3f} "
              f"| {row['kriol_size']:.0f} | {row['c_size']:.0f} |")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--kriol", default=str(BENCH_DIR.parent / "build" / "kriol"))
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--no-wasm", action="store_true", help="skip the wasm32-wasi runs under Node")
    args = parser.parse_args()
    args.node = None if args.no_wasm else shutil.which("node")

    with tempfile.TemporaryDirectory(prefix="kriol-bench-") as work:
        rows = [measure(name, args, Path(work)) for name in BENCHMARKS]
    print_report(rows, args)


if __name__ == "__main__":
    main()
