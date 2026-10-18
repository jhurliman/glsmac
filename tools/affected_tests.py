#!/usr/bin/env python3
"""Select existing GSE consumers before a push; CI still runs complete coverage.

This is a conservative static include graph, not a C++ call-graph proof. Shared
headers, snapshot/serialization edits, runtime changes and unresolved boundaries
broaden to all native groups. Literal GSE includes are followed transitively.
"""
import argparse
import json
import os
from pathlib import Path
import posixpath
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GSE_INCLUDE = re.compile(r"#include\s*\(\s*(['\"])([^'\"]+)\1\s*\)")
CPP_INCLUDE = re.compile(r'^\s*#include\s+"([^"\n]+)"', re.MULTILINE)
SERIALIZATION = re.compile(r"Serialize|Unserialize|WorldSnapshot|CONTINUATION_VERSION|SNAPSHOT_VERSION|Read(?:Int|Bool|String|Bytes)|Write(?:Int|Bool|String|Bytes)")


def normalized(path):
    return posixpath.normpath(str(path).replace("\\", "/"))


def graph(root, extra_paths=()):
    dependencies = {}
    gse_files = sorted((root / "GLSMAC_data").rglob("*.gls.js"))
    gse_names = {path.relative_to(root).as_posix() for path in gse_files} | {path for path in extra_paths if path.endswith(".gls.js")}
    for path in gse_files:
        name = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8")
        includes = []
        for match in GSE_INCLUDE.finditer(text):
            target = normalized(posixpath.join(posixpath.dirname(name), match[2]))
            includes.append(target if target.endswith(".gls.js") else target + ".gls.js")
        literal_count = len(list(GSE_INCLUDE.finditer(text)))
        if len(re.findall(r"#include\s*\(", text)) > literal_count:
            # Section factories concatenate a literal directory with a section
            # name. Include every candidate, not just directly named modules.
            prefixes = re.findall(r"#include\s*\(\s*['\"]([^'\"]*)['\"]\s*\+", text)
            for prefix in prefixes:
                target = normalized(posixpath.join(posixpath.dirname(name), prefix))
                includes.extend(candidate for candidate in gse_names if candidate.startswith(target))
            if len(re.findall(r"#include\s*\(", text)) > literal_count + len(prefixes):
                includes.extend(gse_names) # Unknown dynamic expression: retain all candidates.
        dependencies[name] = set(includes)
    for path in sorted((root / "src").rglob("*")):
        if path.suffix not in (".cpp", ".h"):
            continue
        name = path.relative_to(root).as_posix()
        dependencies[name] = set()
        for include in CPP_INCLUDE.findall(path.read_text(encoding="utf-8")):
            candidates = [normalized(posixpath.join(posixpath.dirname(name), include)), normalized("src/" + include)]
            target = next((candidate for candidate in candidates if (root / candidate).is_file()), None)
            if target:
                dependencies[name].add(target)
            else:
                # Keep removed quoted includes in the graph; deletion must not
                # hide their surviving fixture consumers.
                dependencies[name].update(candidates)
    return dependencies


def closure(dependencies, start):
    visited, pending = set(), [start]
    while pending:
        current = pending.pop()
        if current in visited:
            continue
        visited.add(current)
        pending.extend(dependencies.get(current, ()))
    return visited


def select(paths, root=ROOT, changed_lines=""):
    paths = {normalized(path) for path in paths}
    dependencies = graph(root, paths)
    scripts = sorted(path.relative_to(root).as_posix() for path in (root / "GLSMAC_data/tests").glob("*.gls.js") if not path.name.startswith("_"))
    native = sorted(path.relative_to(root).as_posix() for path in (root / "src/gse/tests").glob("*.cpp") if path.name != "Scripts.cpp")
    reasons = []
    full_native = False
    full_scripts = False
    affected_scripts = {script for script in scripts if closure(dependencies, script) & paths}
    targets = set(paths)
    for path in paths:
        if path.endswith(".cpp") and (root / (path[:-4] + ".h")).is_file():
            targets.add(path[:-4] + ".h")
    selected_native = {source for source in native if closure(dependencies, source) & targets}
    if any(path.startswith(("src/gse/", "src/gc/", "src/task/gsetests/", "src/common/", "src/engine/"))
           and not path.startswith("src/gse/tests/") for path in paths):
        full_native = full_scripts = True
        reasons.append("Shared script/runtime/ownership boundary")
    if any(path.startswith("src/") and path.endswith(".h") and not path.startswith("src/gse/tests/") for path in paths):
        full_native = True
        reasons.append("Shared production header: include graph cannot prove all runtime consumers")
    if any(path.startswith("src/gse/tests/") and path.endswith(".h") for path in paths) and not selected_native:
        full_native = True
        reasons.append("Unresolved shared native fixture dependency")
    if any(path.startswith("GLSMAC_data/default/") for path in paths):
        full_native = True
        reasons.append("Shared embedded GSE module: retain native live-caller/mocked contract consumers")
    production = [path for path in paths if path.startswith("src/") and not path.startswith("src/gse/tests/")]
    if (production and SERIALIZATION.search(changed_lines)) or any("Snapshot" in path or "SimulationState" in path for path in production):
        full_native = True
        reasons.append("Snapshot/serialization format: retain independent restoration fixtures")
    script_changes = {path for path in paths if path.endswith(".gls.js")}
    if script_changes and (not affected_scripts or any("#include(" in (root / path).read_text(encoding="utf-8") and
            len(GSE_INCLUDE.findall((root / path).read_text(encoding="utf-8"))) < (root / path).read_text(encoding="utf-8").count("#include(")
            for path in script_changes if (root / path).is_file())):
        full_scripts = True
        reasons.append("Unresolved or dynamic GSE dependency")
    if production and not selected_native:
        full_native = True
        reasons.append("No proven native consumer: fall back to all native groups")
    if any(path == "CMakeLists.txt" or path.startswith(("cmake/", ".github/")) for path in paths):
        full_native = full_scripts = True
        reasons.append("Build/test registration boundary")
    if full_native:
        selected_native = set(native)
    if full_scripts:
        affected_scripts = set(scripts)
    # Include registration helpers too: AddTest may be called by an inline fixture.
    registration_sources = set().union(*(closure(dependencies, source) for source in selected_native)) if selected_native else set()
    return {"native_all": full_native, "native_sources": sorted(selected_native),
            "native_registration_sources": sorted(source for source in registration_sources if source.startswith("src/gse/tests/")), "scripts": sorted(affected_scripts),
            "reasons": reasons or ["Transitive includes and directly changed test registrations"]}


def execute(plan, binary, root=ROOT):
    binary = str(Path(binary).resolve())
    with tempfile.TemporaryDirectory(prefix="glsmac-affected-tests-") as directory:
        env = {key: value for key, value in os.environ.items() if key not in
               ("GLSMAC_GSE_TEST_SHARD", "GLSMAC_GSE_TEST_SOURCE_FILTER")}
        if plan["native_sources"]:
            if not plan["native_all"]:
                env["GLSMAC_GSE_TEST_SOURCE_FILTER"] = "(^|/)(" + "|".join(re.escape(path) for path in plan["native_registration_sources"]) + ")$"
            subprocess.run([binary, "--gse-tests", "--gse-tests-native-filter", ".*", "--quiet", "--prefix", str(Path(directory) / "native")],
                           cwd=root, env=env, check=True, timeout=600)
        env.pop("GLSMAC_GSE_TEST_SOURCE_FILTER", None)
        for index, script in enumerate(plan["scripts"]):
            subprocess.run([binary, "--gse-tests", "--gse-tests-script", "./" + script, "--quiet", "--prefix", str(Path(directory) / str(index))],
                           cwd=root, env=env, check=True, timeout=30)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", help="Git comparison base (includes staged and unstaged changes)")
    parser.add_argument("--paths", nargs="*", help="Explicit changed paths; serialization analysis then scans those production files")
    parser.add_argument("--binary", help="Cached executable; required only with --run")
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--json", action="store_true", help="Print the complete machine-readable plan")
    args = parser.parse_args()
    if args.paths is None and not args.base:
        parser.error("Supply --base or --paths")
    if args.run and not args.binary:
        parser.error("--run requires --binary")
    if args.paths is not None:
        paths = args.paths
        changed_lines = "\n".join((ROOT / path).read_text(encoding="utf-8") for path in paths
                                  if path.startswith("src/") and (ROOT / path).is_file())
    else:
        paths = subprocess.check_output(["git", "diff", "--no-renames", "--name-only", "-z", args.base], cwd=ROOT).decode().split("\0")[:-1]
        diff = subprocess.check_output(["git", "diff", "--no-renames", "--unified=0", args.base, "--", "src"], cwd=ROOT).decode()
        changed_lines = "\n".join(line[1:] for line in diff.splitlines() if line.startswith(("+", "-")) and not line.startswith(("+++", "---")))
    plan = select(paths, changed_lines=changed_lines)
    if args.json:
        print(json.dumps(plan, indent=2), flush=True)
    else:
        coverage = "all existing groups" if plan["native_all"] else ", ".join(plan["native_sources"]) or "none"
        print("Native coverage: " + coverage)
        print("Script consumers: " + (", ".join(plan["scripts"]) or "none"))
        for reason in plan["reasons"]:
            print("Selection: " + reason)
        print(flush=True)
    if args.run:
        execute(plan, args.binary)


if __name__ == "__main__":
    main()
