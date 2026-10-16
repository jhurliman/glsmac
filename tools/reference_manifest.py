#!/usr/bin/env python3
"""Fingerprint a locally purchased SMAC/SMAX installation without launching it.

Only metadata is emitted: file names, sizes, SHA-256 hashes, and presence flags.
The manifest does not establish that executables run or that files are stock.
Original files are never copied into the output. Run with Python 3.9 or newer.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile


# Keep this allowlist explicit: do not inventory unrelated user files or saves.
REFERENCE_FILES = (
    "alpha.txt",
    "alphax.txt",
    "alpha centauri.ini",
    "terran.exe",
    "terranx.exe",
    "terran_pracx.exe",
    "terranx_pracx.exe",
    "terran_mod.exe",
    "terranx_mod.exe",
    "prac.dll",
    "prax.dll",
    "ddraw.dll",
    "ddrawcompat.ini",
    "ddraw.ini",
    "thinker.dll",
    "thinker.ini",
    "pracx change log.txt",
    "smac_xp2000_readme.txt",
    "readme.txt",
    "readme_ac.txt",
    "manual.pdf",
    "goggame-1207658936.info",
    "gaians.txt",
    "hive.txt",
    "univ.txt",
    "morgan.txt",
    "spartans.txt",
    "believe.txt",
    "peace.txt",
    "angels.txt",
    "cyborg.txt",
    "drone.txt",
    "fungboy.txt",
    "pirates.txt",
    "caretake.txt",
    "usurper.txt",
)

CORE_FILES = frozenset(("alpha.txt", "alphax.txt", "terran.exe", "terranx.exe"))
PATCH_FILES = frozenset(
    (
        "terran_pracx.exe",
        "terranx_pracx.exe",
        "terran_mod.exe",
        "terranx_mod.exe",
        "prac.dll",
        "prax.dll",
        "ddraw.dll",
        "ddrawcompat.ini",
        "ddraw.ini",
        "thinker.dll",
        "thinker.ini",
        "pracx change log.txt",
        "smac_xp2000_readme.txt",
    )
)


class ReferenceManifestError(Exception):
    """An invalid installation or output location supplied by the caller."""


def _fingerprint(path):
    digest = hashlib.sha256()
    size = 0
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            size += len(block)
            digest.update(block)
    return {"path": path.name, "size_bytes": size, "sha256": digest.hexdigest()}


def build_manifest(installation):
    """Return deterministic JSON-compatible metadata for an installation."""
    root = Path(installation).expanduser().resolve()
    if not root.is_dir():
        raise ReferenceManifestError(
            f"Installation directory does not exist or is not a directory: {root}. "
            "Pass the game folder containing alpha.txt or alphax.txt."
        )

    known = frozenset(REFERENCE_FILES)
    found = {}
    for path in sorted(root.iterdir(), key=lambda item: item.name):
        name = path.name.casefold()
        if name not in known or not path.is_file():
            continue
        if name in found:
            raise ReferenceManifestError(
                f"Ambiguous reference files differ only by case: "
                f"{found[name].name}, {path.name}. Choose an unambiguous installation."
            )
        found[name] = path

    if not CORE_FILES.intersection(found):
        raise ReferenceManifestError(
            f"No alpha.txt, alphax.txt, terran.exe, or terranx.exe found in {root}. "
            "Pass the installed game folder, not its parent or a launcher folder."
        )

    files = {name: _fingerprint(found[name]) for name in sorted(found)}
    profiles = {}
    for profile, executable, rules in (
        ("smac", "terran.exe", "alpha.txt"),
        ("smacx", "terranx.exe", "alphax.txt"),
    ):
        profiles[profile] = {
            "executable": executable,
            "executable_present": executable in files,
            "rules": rules,
            "rules_present": rules in files,
            "reference_pair_present": executable in files and rules in files,
        }

    return {
        "schema_version": 1,
        "installation_path": str(root),
        "execution_verified": False,
        "profiles": profiles,
        "patch_indicators": sorted(PATCH_FILES.intersection(files)),
        "distribution_indicators": (
            ["goggame-1207658936.info"] if "goggame-1207658936.info" in files else []
        ),
        "files": files,
        "missing_known_files": sorted(known.difference(files)),
        "limitations": [
            "No executable was launched or tested.",
            "Presence and hashes do not establish official version or unmodified files.",
            "Patch indicators show files present, not patches proven active.",
            "Only allowlisted reference files are inventoried; complete game assets are not validated.",
            "Missing known files may be optional or edition-specific.",
        ],
    }


def _write_manifest(output, encoded):
    """Replace the destination without modifying an existing hard-linked file.

    The new manifest uses tempfile's permissions (owner-only on POSIX), rather
    than inheriting permissions or links from any previous output file.
    """
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            newline="\n",
            dir=output.parent,
            prefix=f".{output.name}.",
            suffix=".tmp",
            delete=False,
        ) as destination:
            temporary = Path(destination.name)
            destination.write(encoded)
        os.replace(temporary, output)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("installation", type=Path, help="Installed game directory")
    parser.add_argument(
        "--output",
        type=Path,
        help="Write JSON outside the installation (default: print to standard output)",
    )
    args = parser.parse_args(argv)

    try:
        manifest = build_manifest(args.installation)
        encoded = json.dumps(manifest, indent=2, sort_keys=True, ensure_ascii=True) + "\n"
        if args.output is None:
            sys.stdout.write(encoded)
        else:
            output = args.output.expanduser().resolve()
            root = Path(manifest["installation_path"])
            if output == root or root in output.parents:
                raise ReferenceManifestError(
                    "Output must be outside the reference installation; "
                    "use a path such as tmp/reference/manifest.json."
                )
            _write_manifest(output, encoded)
    except (ReferenceManifestError, OSError) as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    sys.exit(main())
