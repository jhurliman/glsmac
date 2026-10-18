#!/usr/bin/env python3
"""Compile changed engine C++ objects first, then reuse them in the normal build.

Uses the configured Ninja graph and real platform compiler. This is an early
failure check, not a replacement for building header consumers or running tests.
It adds no platform jobs and does not duplicate successful object compilation.
"""

import argparse
from pathlib import Path
import subprocess


def changed_sources(root, base):
    output = subprocess.check_output([
        "git", "-C", str(root), "diff", "--name-only", "-z",
        "--diff-filter=ACMR", base, "HEAD", "--", "src/",
    ])
    return {name for name in output.decode("utf-8").split("\0")
            if Path(name).suffix in {".cpp", ".cc", ".cxx"}}


def select_targets(graph, sources):
    prefix = "CMakeFiles/GLSMAC.dir/"
    selected = set()
    for line in graph.splitlines():
        target, separator, _ = line.partition(": ")
        normalized = target.replace("\\", "/")
        if not separator or not normalized.startswith(prefix):
            continue
        for suffix in (".obj", ".o"):
            if normalized.endswith(suffix):
                source = normalized[len(prefix):-len(suffix)]
                if source in sources:
                    selected.add(target)
                break
    return sorted(selected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--base", required=True)
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    root = Path(__file__).resolve().parents[1]
    build = args.build_dir.resolve()
    sources = changed_sources(root, args.base)
    if not sources:
        print("No changed standalone C++ sources to prioritize; normal build follows.")
        return
    graph = subprocess.check_output(["ninja", "-C", str(build), "-t", "targets", "all"], text=True)
    targets = select_targets(graph, sources)
    print(f"Prioritizing {len(targets)} configured C++ objects from {len(sources)} changed sources.", flush=True)
    if not targets:
        return  # Included source fragments are checked by their normal consumers.
    command = ["cmake", "--build", str(build), "--parallel", str(args.jobs), "--target", *targets]
    if args.dry_run:
        print(subprocess.list2cmdline(command))
    else:
        subprocess.run(command, check=True)


if __name__ == "__main__":
    main()
