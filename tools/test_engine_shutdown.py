#!/usr/bin/env python3
"""Run actual asset-free Engine shutdown against an existing Linux Ninja Debug build.

Build GLSMAC first. Reuses its application objects, replacing only main with the
standalone harness. This opt-in check does not rebuild or run the full GSE suite.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import time


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, help="Default: BUILD_DIR/engine-shutdown")
    args = parser.parse_args()
    require(sys.platform.startswith("linux"), "This harness supports Linux/WSL Ninja builds only")
    root = Path(__file__).resolve().parent.parent
    build = args.build_dir.resolve()
    cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
    require("CMAKE_GENERATOR:INTERNAL=Ninja\n" in cache, "Expected a single-config Ninja build")
    require("CMAKE_BUILD_TYPE:STRING=Debug\n" in cache, "Expected a Debug build with memory tracking")
    database = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
    entries = [e for e in database if Path(e["file"]).resolve() == root / "src/main.cpp"]
    require(len(entries) == 1, "Compilation database must contain this checkout's src/main.cpp exactly once")
    entry = entries[0]
    require(Path(entry["directory"]).resolve() == build, "Unsupported compilation working directory")
    compile_command = list(entry["arguments"]) if "arguments" in entry else shlex.split(entry["command"])
    require(compile_command.count("-o") == 1 and compile_command.count("-c") == 1,
            "Expected GCC/Clang -c/-o compilation command")
    old_object = compile_command[compile_command.index("-o") + 1]
    require((build / old_object).is_file(), "Missing application objects; build GLSMAC first")
    ninja = subprocess.run(["ninja", "-t", "commands", "bin/GLSMAC"], cwd=build,
                           capture_output=True, text=True, check=True, timeout=30)
    candidates = []
    for line in ninja.stdout.splitlines():
        if line.startswith(": && ") and line.endswith(" && :"):
            line = line[5:-5]
        command = shlex.split(line)
        if old_object in command and "-c" not in command:
            candidates.append(command)
    require(len(candidates) == 1, "Unsupported Ninja link layout: expected one direct application link command")
    link_command = candidates[0]
    require(link_command.count("-o") == 1 and not any(token in link_command for token in ("&&", ";", "||", ">", "<")),
            "Unsupported shell-wrapped link command")
    require(not any(token.startswith("@") for token in link_command), "Link response files are not supported")
    for token in link_command:
        if token.endswith(".o"):
            require((build / token).is_file(), "Missing link object: " + token + "; build GLSMAC first")

    out = (args.output_dir or build / "engine-shutdown").resolve()
    out.mkdir(parents=True, exist_ok=True)
    source = root / "tools/tests/engine_shutdown.cpp"
    obj, binary = out / "engine-shutdown.o", out / "engine-shutdown"
    compile_command[compile_command.index("-o") + 1] = str(obj)
    compile_command[compile_command.index("-c") + 1] = str(source)
    link_command[link_command.index(old_object)] = str(obj)
    link_command[link_command.index("-o") + 1] = str(binary)
    manifest = {"source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                "build_dir": str(build), "source_dir": str(root), "commands": []}
    environment = dict(os.environ)

    def run(name, command, cwd, timeout):
        log_path = out / (name + ".log")
        start = time.monotonic()
        record = {"name": name, "command": command, "cwd": str(cwd), "log": str(log_path)}
        with log_path.open("w", encoding="utf-8") as log:
            try:
                result = subprocess.run(command, cwd=cwd, env=environment, stdout=log,
                                        stderr=subprocess.STDOUT, timeout=timeout)
                record["exit"] = result.returncode
            except subprocess.TimeoutExpired:
                record["exit"] = "timeout"
        record["seconds"] = round(time.monotonic() - start, 3)
        record["log_sha256"] = hashlib.sha256(log_path.read_bytes()).hexdigest()
        manifest["commands"].append(record)
        (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(record), flush=True)
        if record["exit"] != 0:
            print(log_path.read_text(encoding="utf-8", errors="replace"), file=sys.stderr)
            raise RuntimeError(name + " failed; retained diagnostics: " + str(log_path))

    run("compile", compile_command, build, 60)
    run("link", link_command, build, 60)
    for mode in ("default", "single-thread"):
        command = [str(binary), "--gse-tests", "--memorydebug", "--prefix", str(out / (mode + "-data"))]
        if mode == "single-thread":
            command.append("--single-thread")
        run(mode, command, root, 20)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.SubprocessError) as error:
        print("Engine shutdown regression: " + str(error), file=sys.stderr)
        sys.exit(1)
