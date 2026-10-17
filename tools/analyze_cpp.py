#!/usr/bin/env python3
"""Run the repository's correctness checks on complete production translation units."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx"}
HEADER_SUFFIXES = {".h", ".hpp", ".hh", ".hxx", ".inc", ".inl", ".tpp"}
POLICY_FILES = {".clang-tidy", "CMakePresets.json", "CMakeUserPresets.json",
                "tools/analyze_cpp.py", ".github/workflows/cpp-analysis.yml"}
# These are concrete exceptions in the current CMake/source graph, not filename
# conventions: unknown production .cpp files must still fail if absent from CMake.
LINUX_EXCLUDED_SOURCES = {
    "src/network/impl/Windows.cpp": "Windows-only implementation; outside Linux analysis",
}
INCLUDED_SOURCE_FRAGMENTS = {"src/ui/dom/Object.colormap.cpp"}  # included by Object.cpp


def production_source(path):
    return (path.startswith("src/") and Path(path).suffix in SOURCE_SUFFIXES
            and not path.startswith(("src/tmp/", "src/gse/tests/"))
            and path not in LINUX_EXCLUDED_SOURCES and path not in INCLUDED_SOURCE_FRAGMENTS)


def needs_full_scan(path):
    name = Path(path).name
    return (Path(path).suffix == ".cmake"
            or name in {"CMakeLists.txt", ".clang-tidy"} or path in POLICY_FILES)


def dependency_input(path):
    return (path in INCLUDED_SOURCE_FRAGMENTS
            or (path.startswith("src/") and Path(path).suffix in HEADER_SUFFIXES
                and not path.startswith(("src/tmp/", "src/gse/tests/"))))


def git_output(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args])


def changed_paths(root, base):
    # Compare the merge base with the working tree: includes committed, staged,
    # and unstaged edits. Untracked files require a separate query.
    ancestor = git_output(root, "merge-base", base, "HEAD").decode().strip()
    tracked = git_output(root, "diff", "--name-only", "-z", "--no-renames", ancestor)
    untracked = git_output(root, "ls-files", "--others", "--exclude-standard", "-z")
    return sorted({os.fsdecode(path) for path in (tracked + untracked).split(b"\0") if path})


def compilation_entries(root, build_dir):
    database = build_dir / "compile_commands.json"
    if not database.is_file():
        raise ValueError(f"Missing {database}; configure CMake with CMAKE_EXPORT_COMPILE_COMMANDS=ON.")
    sources = {}
    for entry in json.loads(database.read_text(encoding="utf-8")):
        path = Path(entry["file"])
        if not path.is_absolute():
            path = Path(entry["directory"]) / path
        try:
            relative = path.resolve().relative_to(root).as_posix()
        except ValueError:
            continue
        if production_source(relative):
            sources.setdefault(relative, []).append(entry)
    if not sources:
        raise ValueError("Compilation database contains no production sources for this checkout.")
    return sources


def compilation_sources(root, build_dir):
    return sorted(compilation_entries(root, build_dir))


def source_registrations(root, changed, base):
    """Only unchanged CMake structure with literal source-list edits is narrow."""
    candidates = [path for path in changed if path.startswith("src/") and Path(path).name == "CMakeLists.txt"]
    if not candidates:
        return {}
    ancestor = git_output(root, "merge-base", base, "HEAD").decode().strip()
    result = {}
    for path in candidates:
        diff = git_output(root, "diff", "--no-ext-diff", "--unified=0", ancestor, "--", path).decode()
        edits = [line for line in diff.splitlines() if line[:1] in ("+", "-")
                 and not line.startswith(("+++", "---"))]
        # Untracked/empty/unrecognized files cannot masquerade as registration-only.
        if not edits:
            continue
        added = set()
        for line in edits:
            text = line[1:].strip()
            if not text:
                continue
            match = re.fullmatch(r"\$\{PWD\}/([A-Za-z0-9_./-]+\.(?:cpp|cc|cxx))", text)
            if not match:
                break
            target = (root / Path(path).parent / match[1]).resolve()
            try:
                relative = target.relative_to(root.resolve()).as_posix()
            except ValueError:
                break
            if line[0] == "+" and production_source(relative):
                added.add(relative)
        else:
            result[path] = added
    return result


def dependency_command(entry):
    args = entry.get("arguments") or shlex.split(entry.get("command", ""))
    if not args:
        raise ValueError("Compilation entry has no compiler command")
    command = []
    skip = False
    for arg in args:
        if skip:
            skip = False
        elif arg in ("-o", "-MF", "-MT", "-MQ", "-MJ"):
            skip = True
        elif arg in ("-c", "-MD", "-MMD", "-MP", "-M", "-MM", "-MG"):
            continue
        elif arg.startswith(("-MF", "-MT", "-MQ", "-MJ")):
            continue
        else:
            command.append(arg)
    # Compiler preprocessing resolves conditional and transitive includes. No
    # object is compiled, no stale build depfile is trusted, and no file is written.
    return command + ["-M", "-MT", "glsmac_dependencies", "-MF", "-"]


def parse_dependencies(text, directory):
    prefix = "glsmac_dependencies:"
    if not text.startswith(prefix):
        raise ValueError("Compiler did not emit the requested dependency rule")
    text = re.sub(r"\\\r?\n", "", text[len(prefix):])
    # Make escapes spaces/#/backslashes and doubles dollars; shell quote parsing
    # would corrupt perfectly valid include names containing a quote character.
    words, word, escaped = [], [], False
    for char in text:
        if escaped:
            word.append(char)
            escaped = False
        elif char == "\\":
            escaped = True
        elif char.isspace():
            if word:
                words.append("".join(word).replace("$$", "$"))
                word = []
        else:
            word.append(char)
    if escaped:
        raise ValueError("Incomplete compiler dependency escape")
    if word:
        words.append("".join(word).replace("$$", "$"))
    if not words:
        raise ValueError("Compiler emitted an empty dependency rule")
    return {(Path(directory) / word).resolve() for word in words}


def compiler_dependencies(root, build_dir, jobs, timeout):
    entries = compilation_entries(root, build_dir)
    print(f"Resolving header consumers with compiler dependency preprocessing for {len(entries)} production units (no object build).", flush=True)

    def resolve(item):
        source, commands = item
        dependencies = set()
        for entry in commands:
            try:
                result = subprocess.run(dependency_command(entry), cwd=entry["directory"],
                                        capture_output=True, text=True, errors="replace", timeout=timeout)
                if result.returncode:
                    raise ValueError(result.stderr.strip() or result.stdout.strip() or f"compiler exit {result.returncode}")
                observed = parse_dependencies(result.stdout, entry["directory"])
                if (root / source).resolve() not in observed:
                    raise ValueError("Compiler dependency rule omits its source file")
                dependencies.update(observed)
            except (ValueError, OSError, subprocess.SubprocessError) as error:
                raise ValueError(f"Cannot resolve dependencies for {source}: {error}\n"
                                 "No partial analysis result is accepted. Reconfigure/fix missing includes or use "
                                 "--source for explicitly reviewed consumers; --all requests a conservative full scan.") from error
        return source, dependencies

    with ThreadPoolExecutor(max_workers=jobs) as executor:
        return dict(executor.map(resolve, entries.items()))


def select_sources(root, sources, changed, all_sources=False, dependencies=None, registrations=None):
    registrations = registrations or {}
    full = all_sources or any(needs_full_scan(path) and path not in registrations for path in changed)
    candidates = set(sources if full else (path for path in changed if production_source(path)))
    registered = set().union(*registrations.values()) if registrations else set()
    if not full:
        candidates.update(registered)
        headers = {(root / path).resolve() for path in changed if dependency_input(path)}
        if headers:
            if dependencies is None or not set(sources).issubset(dependencies):
                raise ValueError("Header consumer dependencies are unavailable; use compiler dependency preprocessing, "
                                 "--source for explicitly reviewed consumers, or --all for a full scan")
            candidates.update(source for source in sources if headers.intersection(dependencies[source]))
    # Do not silently ignore newly added TUs omitted from a stale CMake database.
    missing = [path for path in set(changed) | registered if production_source(path)
               and (root / path).is_file() and path not in sources]
    if missing:
        raise ValueError("Reconfigure CMake; these production sources are absent from its database: "
                         + ", ".join(missing))
    return sorted({path for path in candidates if (root / path).is_file()}), full


def select_explicit_sources(root, sources, requested):
    selected = set()
    for source in requested:
        path = (root / source).resolve()
        try:
            relative = path.relative_to(root.resolve()).as_posix()
        except ValueError:
            raise ValueError(f"Requested source is outside the checkout: {source}") from None
        if relative in LINUX_EXCLUDED_SOURCES:
            raise ValueError(f"Cannot analyze {relative}: {LINUX_EXCLUDED_SOURCES[relative]}.")
        if relative in INCLUDED_SOURCE_FRAGMENTS:
            raise ValueError(f"Requested source is an included fragment, not a complete translation unit: {relative}")
        if not production_source(relative):
            raise ValueError(f"Requested source must be a complete production translation unit under src/: {relative}")
        if not path.is_file():
            raise ValueError(f"Requested source is missing or is not a file: {relative}")
        if relative not in sources:
            raise ValueError(f"Reconfigure CMake; requested production source is absent from its database: {relative}")
        selected.add(relative)
    return sorted(selected)


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
    scope.add_argument("--source", action="append", metavar="PATH",
                       help="Analyze only this complete production translation unit (repeatable; paths relative to the checkout or absolute).")
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
        if args.source:
            sources = select_explicit_sources(ROOT, compilation_sources(ROOT, build_dir), args.source)
            changed, skipped, scope_name = [], {}, "explicit"
        else:
            changed = [] if args.all else changed_paths(ROOT, args.base or "HEAD")
            registrations = source_registrations(ROOT, changed, args.base or "HEAD")
            full_reasons = [path for path in changed if needs_full_scan(path) and path not in registrations]
            dependencies = None
            if not args.all and not full_reasons and any(dependency_input(path) for path in changed):
                dependencies = compiler_dependencies(ROOT, build_dir, args.jobs, args.timeout)
            sources, full = select_sources(ROOT, compilation_sources(ROOT, build_dir), changed, args.all,
                                           dependencies, registrations)
            if full:
                print("Full analysis requested by: " + ("--all" if args.all else ", ".join(full_reasons)), flush=True)
            elif registrations:
                print("Registration-only CMake edits: " + ", ".join(sorted(registrations)), flush=True)
            skipped = {path: reason for path, reason in LINUX_EXCLUDED_SOURCES.items()
                       if (full or path in changed) and (ROOT / path).is_file()}
            scope_name = "full" if full else ("affected" if dependencies is not None else "changed")
        executable = shutil.which(args.clang_tidy)
        if not executable:
            raise ValueError(f"Cannot find {args.clang_tidy}; install clang-tidy-15 or pass --clang-tidy PATH.")
        version = subprocess.check_output([executable, "--version"], text=True).strip()
        if not re.search(r"\bversion 15\.", version):
            raise ValueError(f"This check policy is validated with clang-tidy 15; found:\n{version}")
        output_dir.mkdir(parents=True, exist_ok=True)
        print(f"Analyzing {len(sources)} production translation units ({scope_name} scope), {args.jobs} jobs.", flush=True)
        for path, reason in skipped.items():
            print(f"Skipping {path}: {reason}.", flush=True)
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
        summary = {"version": version, "scope": scope_name, "changed_paths": changed,
                   "skipped_sources": skipped,
                   "jobs": args.jobs, "seconds": elapsed, "failures": failures, "results": results}
        if args.source:
            summary["requested_sources"] = sources
        (output_dir / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
        print(f"\nAnalyzed {len(results)} translation units in {elapsed:.2f}s; {failures} failed. Logs: {output_dir}")
        return 1 if failures else 0
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(f"Analysis failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
