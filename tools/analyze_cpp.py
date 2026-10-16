#!/usr/bin/env python3
"""Run the repository's correctness checks on complete production translation units."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx"}
HEADER_SUFFIXES = {".h", ".hpp", ".hh", ".hxx", ".inc", ".inl", ".tpp"}
POLICY_FILES = {".clang-tidy", "CMakePresets.json", "CMakeUserPresets.json",
                "tools/analyze_cpp.py", ".github/workflows/cpp-analysis.yml"}


def production_source(path):
    return (path.startswith("src/") and Path(path).suffix in SOURCE_SUFFIXES
            and not path.startswith(("src/tmp/", "src/gse/tests/")))


def needs_full_scan(path):
    name = Path(path).name
    return (Path(path).suffix in HEADER_SUFFIXES | {".cmake"}
            or name in {"CMakeLists.txt", ".clang-tidy"} or path in POLICY_FILES)


def git_output(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args])


def changed_paths(root, base):
    # Compare the merge base with the working tree: includes committed, staged,
    # and unstaged edits. Untracked files require a separate query.
    ancestor = git_output(root, "merge-base", base, "HEAD").decode().strip()
    tracked = git_output(root, "diff", "--name-only", "-z", "--no-renames", ancestor)
    untracked = git_output(root, "ls-files", "--others", "--exclude-standard", "-z")
    return sorted({os.fsdecode(path) for path in (tracked + untracked).split(b"\0") if path})


def compilation_sources(root, build_dir):
    database = build_dir / "compile_commands.json"
    if not database.is_file():
        raise ValueError(f"Missing {database}; configure CMake with CMAKE_EXPORT_COMPILE_COMMANDS=ON.")
    sources = set()
    for entry in json.loads(database.read_text(encoding="utf-8")):
        path = Path(entry["file"])
        if not path.is_absolute():
            path = Path(entry["directory"]) / path
        try:
            relative = path.resolve().relative_to(root).as_posix()
        except ValueError:
            continue
        if production_source(relative):
            sources.add(relative)
    if not sources:
        raise ValueError("Compilation database contains no production sources for this checkout.")
    return sorted(sources)


def select_sources(root, sources, changed, all_sources=False):
    full = all_sources or any(needs_full_scan(path) for path in changed)
    candidates = sources if full else [path for path in changed if production_source(path)]
    # Do not silently ignore newly added TUs omitted from a stale CMake database.
    missing = [path for path in changed if production_source(path)
               and (root / path).is_file() and path not in sources]
    if missing:
        raise ValueError("Reconfigure CMake; these production sources are absent from its database: "
                         + ", ".join(missing))
    return sorted({path for path in candidates if (root / path).is_file()}), full


def run_analysis(root, build_dir, executable, source, timeout, output_dir):
    command = [executable, "--quiet", "-p", str(build_dir), f"--config-file={root / '.clang-tidy'}",
               f"--header-filter=^{re.escape(str(root / 'src'))}/", str(root / source)]
    started = time.monotonic()
    try:
        process = subprocess.run(command, cwd=root, capture_output=True, text=True,
                                 errors="replace", timeout=timeout)
        status, output = process.returncode, process.stdout + process.stderr
    except subprocess.TimeoutExpired as error:
        status = 124
        output = f"Analysis exceeded {timeout:g} seconds for {source}.\n"
        for partial in (error.stdout, error.stderr):
            if partial:
                output += partial.decode(errors="replace") if isinstance(partial, bytes) else partial
    log = output_dir / f"{source}.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text(output, encoding="utf-8")
    return {"source": source, "exit_code": status,
            "seconds": round(time.monotonic() - started, 2), "log": str(log)}, output


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    scope = parser.add_mutually_exclusive_group()
    scope.add_argument("--base", help="Compare against the merge base with REF, including local edits (default: HEAD).")
    scope.add_argument("--all", action="store_true", help="Analyze every production translation unit.")
    parser.add_argument("--build-dir", type=Path, default=Path("build/analysis"))
    parser.add_argument("--clang-tidy", default="clang-tidy-15")
    parser.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 1))
    parser.add_argument("--timeout", type=float, default=120, help="Maximum seconds per translation unit.")
    parser.add_argument("--output-dir", type=Path, help="Logs and summary (default: BUILD_DIR/analysis-results).")
    args = parser.parse_args(argv)
    if args.jobs < 1 or args.timeout <= 0:
        parser.error("--jobs and --timeout must be positive")
    build_dir = (ROOT / args.build_dir).resolve()
    output_dir = (ROOT / args.output_dir).resolve() if args.output_dir else build_dir / "analysis-results"
    try:
        changed = [] if args.all else changed_paths(ROOT, args.base or "HEAD")
        sources, full = select_sources(ROOT, compilation_sources(ROOT, build_dir), changed, args.all)
        executable = shutil.which(args.clang_tidy)
        if not executable:
            raise ValueError(f"Cannot find {args.clang_tidy}; install clang-tidy-15 or pass --clang-tidy PATH.")
        version = subprocess.check_output([executable, "--version"], text=True).strip()
        if not re.search(r"\bversion 15\.", version):
            raise ValueError(f"This check policy is validated with clang-tidy 15; found:\n{version}")
        output_dir.mkdir(parents=True, exist_ok=True)
        print(f"Analyzing {len(sources)} production translation units ({'full' if full else 'changed'} scope), {args.jobs} jobs.", flush=True)
        print(version, flush=True)
        started = time.monotonic()
        results = []
        with ThreadPoolExecutor(max_workers=args.jobs) as executor:
            for result, output in executor.map(
                    lambda source: run_analysis(ROOT, build_dir, executable, source, args.timeout, output_dir), sources):
                results.append(result)
                if result["exit_code"] != 0:
                    print(f"\n--- {result['source']} ---\n{output}", flush=True)
        elapsed = round(time.monotonic() - started, 2)
        failures = sum(result["exit_code"] != 0 for result in results)
        summary = {"version": version, "scope": "full" if full else "changed", "changed_paths": changed,
                   "jobs": args.jobs, "seconds": elapsed, "failures": failures, "results": results}
        (output_dir / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
        print(f"\nAnalyzed {len(results)} translation units in {elapsed:.2f}s; {failures} failed. Logs: {output_dir}")
        return 1 if failures else 0
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(f"Analysis failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
