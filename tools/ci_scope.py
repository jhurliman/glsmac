#!/usr/bin/env python3
"""Choose CI coverage without treating deferred platform jobs as tested."""

import json
import os
from pathlib import Path
import re
import subprocess


# Source-directory CMake files commonly contain only source/test registrations.
# This path classifier does not inspect their contents: build-semantic edits in
# those files require ci:full. Root configuration and platform runtime select it.
PLATFORM_PREFIXES = (
    ".github/", "src/env/", "src/network/", "src/graphics/", "src/audio/",
    "src/input/", "src/error_handler/", "src/common/", "src/gc/",
    "src/engine/", "src/task/gseprompt/",
    "dependencies/", "cmake/", "resource/icon/", "resource/macos/",
)
PLATFORM_FILES = {
    "CMakeLists.txt", "CMakePresets.json", "CMakeUserPresets.json",
    "src/main.cpp", "src/GLSMAC.cpp", "src/GLSMAC.h",
    "src/config/Config.h", "src/util/UUID.cpp", "src/util/UUID.h",
    "src/util/System.cpp", "src/util/System.h", "src/util/FS.cpp",
    "src/util/FS.h", "src/util/String.cpp", "src/util/Thread.cpp",
    "src/util/Thread.h", "tools/test_thread_lifecycle.py", "tools/ci_scope.py",
    "tools/tests/test_ci_scope.py",
    "src/gse/Async.cpp", "src/gse/Async.h", "src/util/Timer.cpp", "src/util/Timer.h",
}
PLATFORM_MANIFESTS = {"vcpkg.json", "vcpkg-configuration.json", "conanfile.py", "conanfile.txt", "conan.lock"}


def select_scope(event_name, event, paths):
    if event_name == "workflow_dispatch" or (event_name == "push" and event["ref"] == "refs/heads/stable"):
        return True, True, event_name
    if event_name not in ("pull_request", "push", "merge_group"):
        raise ValueError("Unsupported CI event")
    if event_name == "pull_request":
        if not isinstance(event["pull_request"]["draft"], bool):
            raise ValueError("Missing boolean pull-request draft state")
        if any(label["name"] == "ci:full" for label in event["pull_request"].get("labels", [])):
            return True, True, "explicit ci:full integration checkpoint"
    if not paths or all(p.endswith(".md") for p in paths):
        return False, False, "Documentation-only or empty change; engine and lifecycle tests not requested"
    sensitive = [p for p in paths if p in PLATFORM_FILES
                 or p.startswith(PLATFORM_PREFIXES) or p.endswith((".cmake", ".rc", ".ico", ".icns", ".plist", ".mm"))
                 or p.rsplit("/", 1)[-1] in PLATFORM_MANIFESTS
                 or (p.endswith("CMakeLists.txt") and not p.startswith("src/"))]
    if sensitive:
        return True, True, "platform-sensitive change: " + sensitive[0]
    return True, False, "Linux coverage; platform suites not requested for this change"


def validate_lifecycle(run, full, scope, lifecycle):
    if run not in ("true", "false") or full not in ("true", "false") or scope != "success":
        raise ValueError("CI scope selection did not succeed")
    if run == "false" and full != "false":
        raise ValueError("Full coverage cannot skip test jobs")
    if lifecycle != ("success" if run == "true" else "skipped"):
        raise ValueError("Required test result does not match selected CI scope")


def validate_results(run, full, scope, unix, windows):
    """The final check never translates a missing/failed required job to success."""
    validate_lifecycle(run, full, scope, unix)
    expected = "success" if full == "true" else "skipped"
    if windows != expected:
        raise ValueError("Windows regression result does not match selected CI scope")


def changed_paths(event_name, event):
    # --no-renames includes both removed and added paths: moving a platform
    # implementation out of its usual directory must not evade full coverage.
    if event_name == "pull_request":
        base = event["pull_request"]["base"]["sha"]
        head = event["pull_request"]["head"]["sha"]
    elif event_name == "merge_group":
        base, head = event["merge_group"]["base_sha"], event["merge_group"]["head_sha"]
    elif event_name == "push":
        base, head = event["before"], event["after"]
    else:
        return []
    if not all(re.fullmatch(r"[0-9a-f]{40}", ref) for ref in (base, head)):
        raise ValueError("Invalid comparison revision")
    comparison = base + ("..." if event_name == "pull_request" else "..") + head
    output = subprocess.check_output([
        "git", "diff", "--no-renames", "--name-only", "-z", comparison,
    ])
    return output.decode("utf-8").split("\0")[:-1]


def main():
    event_name = os.environ["GITHUB_EVENT_NAME"]
    event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text())
    paths = [] if event_name == "push" and event["ref"] == "refs/heads/stable" else changed_paths(event_name, event)
    run, full, reason = select_scope(event_name, event, paths)
    unix = ["ubuntu-24.04", "macos-14"] if full else ["ubuntu-24.04"]
    lifecycle = [{"os": "ubuntu-24.04", "compiler": "g++"}]
    if full:
        lifecycle.append({"os": "macos-14", "compiler": "clang++"})
    with open(os.environ["GITHUB_OUTPUT"], "a") as output:
        output.write("run=" + str(run).lower() + "\n")
        output.write("full=" + str(full).lower() + "\n")
        output.write("unix=" + json.dumps(unix) + "\n")
        output.write("lifecycle=" + json.dumps({"include": lifecycle}) + "\n")
    with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as summary:
        summary.write("CI scope: " + reason.replace("\n", " ") + ".\n")
    print("CI scope:", reason)


if __name__ == "__main__":
    main()
