# CVR container envelope

`loader::cvr::CvrContainerReader::Read(std::string_view)` is a pure decoder for
the container envelope observed in the installed GOG Alpha Centauri asset set.
It returns four ordered chunks, each with an integer tag and an owned
`std::vector<uint8_t>` payload. It performs no file I/O and has no dependency on
the game, graphics, SDL, a ruleset, or a particular unit design.

This is the first format-reading component for future conventional-unit art.
It does **not** decode geometry, colors, materials, animation, attachment points,
model composition, or unit-to-asset mapping, and does not activate any units.

## Supported profile

All offsets and sizes below are bytes. The byte order is intentionally mixed.

| Position | Field | Accepted envelope |
| --- | --- | --- |
| 0 | Signature, four bytes | `43 56 52 00` (`CVR` followed by NUL) |
| 4 | Little-endian uint32 | Exact total input size, including the file header |
| 8 onward | Four consecutive chunks | Tags 1, 2, 3, 4, in that order |
| Chunk + 0 | Big-endian uint32 | Expected tag |
| Chunk + 4 | Little-endian uint32 | Chunk size **including its eight-byte header** |
| Chunk + 8 | Payload | Nonempty; tag 1 is exactly four bytes |

The final chunk must end exactly at the declared file size. Unknown, missing,
duplicate, reordered, empty, or extra chunks are rejected. These are explicit
restrictions of the observed supported profile, not a claim that every rejected
file is corrupt or that every CVR producer uses this profile. Future variants
need evidence and deliberate support, rather than silently ignoring unknown
tags or trailing bytes.

The reader does not interpret or constrain any payload's contents. In
particular, it accepts any four bytes for tag 1; it does not treat that word as
an authenticated format version. Tag 2 is not converted to a filename, and the
other two chunks are not labeled as a palette or mesh. Embedded zero and
high-bit bytes are preserved. Later decoders must validate the opaque content
before using it as geometry or material data.

## Bounds and ownership

Each chunk header is read only after at least eight remaining bytes are
available. Its size is checked against the remaining input before advancing
the cursor. Size arithmetic uses subtraction for bounds checks, so a hostile
`UINT32_MAX` length cannot wrap a cursor or request an allocation. The declared
total length is compared with the full `size_t` input size without narrowing.

All four descriptors and the final extent are validated before payload copies
are allocated. Rejection throws `std::invalid_argument` with a `CVR container:`
prefix. No partially decoded result escapes. Normal allocation exceptions
propagate, and standard containers release any copies already made. This is
not a global memory budget: a valid large input still requires storage for its
owned payloads. The returned object can outlive, copy, or move independently of
the caller's input. The caller must keep the input readable and unchanged for
the duration of the call.

## Installed-file evidence, 2026-10-16

A read-only probe called the production reader on every case-insensitive
`.cvr` file directly in the local GOG installation. One extension is uppercase
(`VHTTAp.CVR`), so a case-sensitive `*.cvr` glob would miss it.

- Accepted **140/140** files, **12,807,702** total bytes, no rejected files.
- File sizes span 7,914 through 884,165 bytes. All total-size fields match the
  actual files, and every file has exactly the ordered four-chunk envelope.
- Tag 1 payload bytes are `9a 99 21 41` in 102 files and `00 00 20 41` in 38.
  Their meaning is not established or used by this reader.
- Observed tag 2 payloads are ASCII source-model names **without a NUL
  terminator**. The chunk length delimits these bytes.
- Tag 3 complete chunk sizes are 6,945 (101 files), 19,245 (36), and 19,246 (3),
  including the eight-byte header. No single size is assumed by the reader.

The SHA-256 of the inventory manifest is
`96f95e27c1677834d7b8e4c4e2c17d48095106dcbcc09827ce9849d137653f50`.
Its canonical UTF-8/LF form is one `sha256 + two spaces + original basename +
newline` per file, sorted by the lowercase basename. Selected file hashes:

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `VI.cvr` | 191315 | `d2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1` |
| `VT.cvr` | 185105 | `076df434678a92b0de70965a0c3c1a400de31924d9365c8c96998f0bb2e13c19` |
| `Vw00.cvr` | 15030 | `18395bc46eecb0988164c00365d4d7cbc9ca527e3e732067608fd366c62ae497` |
| `ACOLPOD.cvr` | 594208 | `4aed3542f2b7a5070aac6b384e5e40eaee9c39520200f0212798d49d18adbe06` |

This validates an envelope against this installed asset set, not an original
renderer or full-game equivalence. The filenames do not establish which art
belongs to a human Scout, Former, or Colony Pod. No original asset payloads are
included in the repository or required by automated tests.

## Reproduction and validation

The five synthetic native groups run in the existing full `gse` CTest. A
Debug build can also run the full suite with `build/bin/GLSMAC --gse-tests`.
The filtered command below additionally requires the native-test filter from
[PR #57](https://github.com/jhurliman/glsmac/pull/57); on branches without that
separate feature, omit `--gse-tests-native-filter` and its argument:

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^CVR container' \
  --prefix /tmp/glsmac-cvr-tests
```

They check byte order and variable payload sizes, independent copy/move/source
lifetimes, every truncated prefix (both original and repaired total length),
invalid magic, short/huge/empty chunk lengths, tag-1 width, duplicate/unknown/
reordered tags, and trailing bytes or a fifth chunk. Payloads are synthetic.

The optional probe is independent of the application build. It reports only
filenames, sizes and chunk tags/payload lengths, returning nonzero on rejected
files, I/O errors, or an empty inventory. For example, from the repository root:

```sh
clang++ -std=c++17 -O2 -DNDEBUG -Isrc \
  src/loader/cvr/CvrContainerReader.cpp src/loader/cvr/CvrSceneReader.cpp \
  src/loader/cvr/CvrGeometryReader.cpp \
  tools/tests/cvr_container_probe.cpp \
  -o /tmp/cvr-container-probe
/tmp/cvr-container-probe '/path/to/installed/Alpha Centauri'
```

The optional `--scene` mode additionally validates and reports the
[nested scene records](cvr-scene-records.md). The default mode retains the
envelope-only check.
The optional `--geometry` mode also validates the supported
[geometry-record profile](cvr-geometry-records.md), reporting its rejections
separately. Any rejection makes the probe return nonzero.

Local validation on Linux/WSL with Clang 15:

- Full normal CTest: **8/8**, 38.51 seconds.
- Five focused groups: clean under ASan + UBSan + LSan with leak detection
  enabled. The existing local non-PIE sanitizer workaround was retained; no
  build-policy change is included.
- Three separate copied-source negative controls disabled total-size,
  expected-tag, and final-extent checks respectively. Each actual native run
  exited 1 with **4 passed / 1 failed**, at its corresponding rejection
  assertion. The intended production source stayed unchanged throughout.
- Explicit one-TU static analysis in Debug and Release: zero failures,
  0.21 and 0.20 seconds. The standalone `-O2 -DNDEBUG -Wall -Wextra -pedantic`
  probe compiled clean and accepted the installed 140-file set.

Evidence logs, the inventory manifest and read-only survey are retained locally
under `/home/jhurliman/src/glsmac-cvr-container/`, notably
`normal-ctest.log`, `asan-focused.log`, `*-negative.log`,
`debug-analysis.log`, `release-analysis.log`, `installed-reader-probe.log`,
`installed-manifest.sha256`, and `installed-survey.json`. Native Windows/macOS
execution and any rendering equivalence are not claimed by this slice.
