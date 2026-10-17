# Frozen SMAC resource rules

`SmacResourceRules` is the single owned settings authority for all nine
physical `RESOURCEINFO` N/M/E triples and the resource-related `RULES`
metadata. It replaces the former three-triple settings member. The existing
`ParseBaseSquareRules` entry point remains a compatibility projection and
still accepts a `RESOURCEINFO` section without a `RULES` section.

The frozen profile preserves these values:

| Source position, one-based | Retained value |
| --- | --- |
| RESOURCEINFO 1–9 | Ocean, base, bonus, forest, recycling, improved land, improved sea, monolith and borehole triples |
| RULES 2 | Nutrients per citizen |
| RULES 9 | Unroaded mine mineral limit |
| RULES 10 | Mine nutrient effect |
| RULES 54, 55, 56 | Nutrient, mineral and energy limit technology tokens |
| RULES 58 | Advanced mining-platform technology token |

Positions establish row identity. Display labels can repeat or change; the
fourth resource column and annotations are not modeled. All 30 numeric
fields retain signed32 values, including zero and negative metadata. The
profile applies no yield arithmetic, demand clamp or gameplay default.
`BaseSquare`, `TileNutrients`, `TileMinerals` and `TileEnergy` return copied
views in the existing calculators' constructor order. There is no second
mutable center-rule store.

Technology tokens retain their spelling. Validation resolves each against
the supplied frozen 89-row catalog through the existing token resolver:
known IDs, explicit `None` and explicit `Disable` remain distinguishable;
unknown tokens reject. The profile does not decide technology ownership or
the resource helpers' interpretation of those two sentinels. A future live
consumer must supply that evidence-backed query policy.

## Ingestion and transport

`RESOURCEINFO` continues to use the measured physical-line and numeric-prefix
behavior described in [the text-parser receipt](smac-resourceinfo-text.md).
Blank/comment lines occupy a zero row, and the existing bounded conversion
can wrap into signed32. The extraction now retains all nine triples without
changing that conversion.

The resource `RULES` fields deliberately use the existing strict port policy:
exactly 70 logical rows, semicolon comments, surrounding whitespace and one
optional final comma, signed decimal integers with complete-token consumption.
The four references must be nonempty, NUL-free and at most 256 bytes. This
policy does not claim equivalence to the original custom `RULES` text reader;
in particular it does not apply RESOURCEINFO prefix/wrap behavior to RULES.
The 256-byte token budget is a port admission limit, not a recovered original
buffer size.

Profile V1 uses the existing typed `Buffer` encoding: version, 27 row-major
integers, three RULES integers, then four strings. Its maximum is exactly
1,482 bytes: 31 integer frames of 14 bytes and four string frames of at most
262 bytes. The decoder checks the complete budget, every signed32 narrowing,
individual string bounds, version/types, references and exact end. Writers
validate before encoding. Outer settings and world readers bound the frame
before copying or checksum processing; an over-limit world frame leaves its
cursor at the start of that frame.

GlobalSettings V10 carries the full profile instead of nine inline integers.
Host initialization reads `alpha.txt` once, stages the technology catalog and
resource profile with the other existing profiles, then publishes the whole
validated settings value. A receiver stages the resource bytes, resolves them
against its incoming catalog and compares every field before changing frozen
settings. V9 and other old settings versions reject.

World V16 binds the same complete profile to the frozen session before map
allocation. The writer preencodes all profiles before appending the header;
the reader compares all 34 semantic fields, including exact token spelling.
V15 rejects. SMAX continues to carry no SMAC profile. Simulation encoding,
RNG ownership and the world body layout are unchanged.

## Evidence and regression coverage

The generated fixture retains all nine triples from the 37 self-authored
`RE-SMAC-BASE-TEXT-001` cases, totaling 999 resource values, alongside the
previous center projections and 592 center-yield triples. The generator pins
fixture SHA256
`65fe760ee3c87687663e4d3e297533342fe804e35001a6a294761ef62bddfea6`.
Installed text is not included in Git, and no original executable is rerun.

Six new native groups cover positional views, literal original rows, strict
RULES rejection, full codec extents and exact-cap valid strings, all-field
settings/world equality, copied ownership and failed-host retry. Existing
host/wire fixtures now supply real named synthetic technology tokens in the
four newly interpreted RULES positions; their arithmetic values are unchanged.
Legacy center-mutation helpers still write out-of-range int64 values rather
than narrowing them before testing rejection.

The real final-download Client matrix adds changed late resource rows,
demand/mine fields, changed or missing late technology keys, a truncated last
token and old V15 input. Immediate and queued dispatch check safe disconnect,
unchanged retained map bytes/settings/managers/turn/RNG/frontend queue and no
world/gameplay callbacks. Existing oversized-frame cases now include the
resource profile. These run through the production Client/header path; they
are not a replacement for a complete loaded-world gameplay test.

The opt-in probe supports `--full-profile` for an installed rules file.
`compare_resourceinfo_text.py --full-profile --installed ...` retains the
38-case/608-yield original comparison, checks the installed nine triples and
the three numeric/four token RULES values, and exercises exact profile restore.
The installed RULES expectations are source metadata observations, not a new
original-reader lexical experiment. The probe compiles all of its linked
dependencies consistently in Release mode; mixing Debug `Buffer`/`Class`
objects into the standalone process is invalid.

This prerequisite does not change `get_tile_resources`, base intake, manual
worker scheduling, fungal capability construction, technology possession,
global/local resource events or default startup. Those live producers remain
separate work. No neutral context values are invented here.

## Validation

On the source branch based on `5ed286fd`, all eight normal CTests passed in
87.90 seconds and all eight ASan/UBSan/LSan CTests passed in 177.34 seconds
(leak detection and halt-on-error enabled). The 38 focused native groups,
including the six new groups and real Client matrix, also passed with
allocation tracking in both builds. Full LLVM 15 correctness analysis of
338 production translation units reported zero failures in 136.70 seconds.
These are local Linux checks, not Windows/macOS or hosted publication results.

The standalone probe and all its linked dependencies were compiled with
Release settings and UBSan. It matched all 38 original text cases and 608
center-yield triples, plus the installed complete resource profile and exact
codec restore. Regenerating the owned fixture reproduced the checked-in file.
Four copied-production controls each exited 1 at the intended assertion:
discarding the last resource row, ignoring token equality, omitting world
profile equality and removing the outer frame limit. Intended production
files were never substituted for those controls.

The retained initial test-macro compile failure, missing probe library setup,
and two invalid mixed Debug/Release probe attempts were corrected in the
test/build harness before these final results; none required a production
behavior change. Local logs, source/control hashes and the comparison report
are retained under `/home/jhurliman/src/glsmac-startup-main-refresh/` with
the `resource-` prefix and `resource-final-manifest.json`.
