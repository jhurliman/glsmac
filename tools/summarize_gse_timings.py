#!/usr/bin/env python3
"""Publish real test-body plus teardown timings from CTest's retained log."""
import argparse
import json
from pathlib import Path
import re

TIMING = re.compile(r"^\[GSE timing\] ([0-9.]+) ms \| (.*?) \| (.*)$", re.MULTILINE)


def summarize(text):
    groups = [{"milliseconds": float(ms), "source": source.replace("\\", "/"), "name": name}
              for ms, source, name in TIMING.findall(text)]
    return {"groups": sorted(groups, key=lambda group: group["milliseconds"], reverse=True),
            "measured_group_count": len(groups),
            "summed_group_seconds": sum(group["milliseconds"] for group in groups) / 1000}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--summary")
    args = parser.parse_args()
    path = Path(args.log)
    if not path.is_file():
        print("No CTest log exists; build or test startup did not complete.")
        return
    result = summarize(path.read_text(encoding="utf-8", errors="replace"))
    Path(args.output).write_text(json.dumps(result, indent=2), encoding="utf-8")
    if args.summary:
        with open(args.summary, "a", encoding="utf-8") as output:
            output.write("\nGSE timing: " + str(result["measured_group_count"]) + " completed groups; " +
                         format(result["summed_group_seconds"], ".2f") + " summed group seconds (includes teardown; parallel wall time differs).\n\n")
            output.write("| Slowest completed group | Seconds | Registration source |\n| --- | ---: | --- |\n")
            for group in result["groups"][:20]:
                source = group["source"].split("/src/", 1)[-1]
                output.write("| " + group["name"].replace("|", "\\|") + " | " + format(group["milliseconds"] / 1000, ".3f") + " | " + source + " |\n")
    print(json.dumps({key: value for key, value in result.items() if key != "groups"}))


if __name__ == "__main__":
    main()
