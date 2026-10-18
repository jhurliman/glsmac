#!/usr/bin/env python3
"""Exercise real registration provenance, shard admission and failure exits."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", required=True)
    parser.add_argument("--source", required=True)
    args = parser.parse_args()
    executable = str(Path(args.executable).resolve())
    with tempfile.TemporaryDirectory(prefix="glsmac-test-selection-") as directory:
        calls = 0
        def run(selection, pattern="^test if tests work$", native=True):
            nonlocal calls
            calls += 1
            env = {key: value for key, value in os.environ.items() if key not in
                   ("GLSMAC_GSE_TEST_SHARD", "GLSMAC_GSE_TEST_SOURCE_FILTER")}
            env.update(selection)
            command = [executable, "--gse-tests", "--quiet", "--prefix", str(Path(directory) / str(calls))]
            if native:
                command.extend(["--gse-tests-native-filter", pattern])
            result = subprocess.run(command, cwd=args.source, env=env, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10)
            assert result.returncode in (0, 1), (result.returncode, result.stdout)
            return result

        for value in ("", "1/0", "4/4", "0/65", "-1/4", "0/2/trailing"):
            result = run({"GLSMAC_GSE_TEST_SHARD": value})
            assert result.returncode == 1 and "Invalid native test selection:" in result.stdout, result.stdout
        result = run({"GLSMAC_GSE_TEST_SHARD": "0/4"}, native=False)
        assert result.returncode == 1 and "requires --gse-tests-native-filter" in result.stdout, result.stdout
        result = run({"GLSMAC_GSE_TEST_SOURCE_FILTER": "["})
        assert result.returncode == 1 and "Invalid native test selection:" in result.stdout, result.stdout
        result = run({"GLSMAC_GSE_TEST_SOURCE_FILTER": r"(^|/)src/gse/tests/Mesh\.cpp$"},
                     "rectangle getters and render copies")
        assert result.returncode == 0 and "[GSE timing]" in result.stdout, result.stdout
        unknown = "<unknown registration source>" in result.stdout
        result = run({"GLSMAC_GSE_TEST_SOURCE_FILTER": "nonexistent-source-file"})
        assert result.returncode == (0 if unknown else 1), result.stdout
        accepted = []
        for index in range(4):
            result = run({"GLSMAC_GSE_TEST_SHARD": str(index) + "/4"})
            if result.returncode == 0:
                assert "  test if tests work..." in result.stdout, result.stdout
                accepted.append(index)
            else:
                assert "No native tests matched filter:" in result.stdout, result.stdout
        assert len(accepted) == 1, accepted
        print("Registration source selection, conservative fallback, shard partition and invalid selection exits passed")


if __name__ == "__main__":
    main()
