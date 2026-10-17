# Frozen SMAC technology catalog and production prerequisites

An explicit SMAC host parses the 89 positional technology records in the same
single `alpha.txt` read as its base-square and production profiles. The complete
catalog is owned by `GlobalSettings`, frozen before initialization, and supplied
to joining clients through State/Packet/Client. SMAX has no SMAC catalog, and
clients do not read a local rules file. No ownership, social rating, initial
production, research target or faction package is created by this change.

`GetSmacFacilityPrerequisite(source_id)` consumes the existing production
profile's exact facility prerequisite token and the frozen technology catalog.
The ID is the SMAC facility source slot 1..71, not a port `ProductionItem` ID.
Missing profiles and unknown slots remain unavailable. Results distinguish no
requirement, explicit/definition disablement, a technology passing the production
definition guard, and unavailable references. A known technology's positional ID
is retained even when its definition or production range rejects it. This is not
a buildability API: positive faction identity, technology ownership and the
remaining item/base/option guards have not been evaluated. The special Ascent
path must not be replaced by this ordinary prerequisite inspection.

## Evidence and supported source format

RE-SMAC-TECH-PRE-001 reports 300 original-x86 checks, including 32 named fixtures,
for `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The complete loader, token resolver and original comparison execute with supplied
text/numeric/name-allocation services and diagnostic presentation. This is a
routine-level controlled-state contract, not a full-game or startup ownership
observation. The separate RE-SMAC-FACILITY-ELIGIBLE-001 confirms the prerequisite
guard in predicate `0x5d1950`, specifically `0x5d19a4..0x5d1a15`;
RE-SMAC-QUEUE-001 independently covers the corresponding predefined-unit guard.
The human-owned research is not copied into this PR.
The named fixture file used for the comparison below has SHA-256
`d44a285c0e8791701a5408da49f6fc4dce2a87c8927ed245594ce17a8e0a7034`.

The read-only installed source is GOG SMAC `alpha.txt`, 53,073 bytes, SHA-256
`38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
Its `TECHNOLOGY` section contains 89 records with nine columns each: display
name, key, four priority values in **source order**, two prerequisite tokens,
and nine binary flag digits. The four priorities stay positional; no conversion
to a consumer's rating order occurs. Flags are retained without applying effects.

`ParseTechnologyRules(source)` requires this canonical record/column count and
complete signed32 decimal priorities. It accepts LF/CRLF, surrounding field
whitespace and semicolon comments through the existing text helpers. Names and
references must be nonempty and NUL-free. Empty, fractional, partial and
out-of-range numeric fields, malformed flags, missing/duplicate requested sections
and extra columns reject. These are explicit GLSMAC ingestion rules, not a claim
that the original text-reader services have identical malformed-file behavior.

Stored keys are the first seven source bytes. Colliding or repeated keys retain
their positions and first-match lookup; the catalog is never compacted or
renumbered. All 89 definitions remain present, including disabled entries and
Transcendent Thought at slot 88. A serialized key longer than seven bytes rejects
rather than being silently truncated during decode.

## Exact query boundary

`ResolveToken` removes trailing ASCII spaces only and performs case-insensitive
ASCII lookup. It does not remove leading spaces or trailing tabs, truncate the
lookup token, resolve display names, or interpret numeric strings as IDs.
`None` and `Disable` are case-insensitive sentinels. Lookup scans all stored keys
in positional order, so forward references resolve after the full catalog loads.
The supported case folding is the ASCII key domain observed in the source;
locale-dependent non-ASCII comparisons are not claimed.

Unknown/empty tokens have their own `UNKNOWN` result. The original asks for a
rules-error diagnostic and returns -2 if that diagnostic returns. This port
query does not display that UI or silently alias an error to explicit Disable.
Unresolved definition references remain unavailable, including None/unknown;
this conservative refusal is separate from the evidenced None/Disable exception.

For a known production reference, only IDs 0..87 pass the range guard. If `a`
and `b` are the definition's resolved parent references, its production guard is
`a >= -1 && (b >= -1 || a == -1)`. Thus None/Disable passes, ordinary/Disable
and Disable/None fail. This does not recursively require parent ownership and
does not inspect raw flags. The original loader's counted-definition predicate
is different (both parents at least -1); there is no shared universal `enabled`
flag. Catalog slot 88 resolves normally but is unavailable for this production
query. All of these results still leave the requesting faction's ownership and
other production admission requirements unevaluated.

## Synchronization and limits

Global-settings **V4** replaces V3. The SMAC presence flag now governs all three
profiles. Following the framed production profile is a framed technology catalog
with its own V1, exact count 89, then source ID/name/key/four priorities/two
prerequisites/flags for each record. Integer frames are checked before narrowing;
record identities must equal their positions and flags stay within nine bits.
Unknown versions, incorrect types/counts/identities, invalid text, truncation and
trailing fields reject. No untrusted count controls allocation.

All profiles and every remaining global setting are staged before publication.
Locked equality includes every modeled technology field; identical catalogs
still permit ordinary lobby updates. Parse/late decode failure preserves the
previous identity, profiles, lock, map settings, difficulty and game name.
Selecting/resetting clears the catalog. This is atomic global-settings parsing,
not a transaction for a later malformed faction body in State.

WorldSnapshotRules files are deliberately unchanged. The next world-header
version must bind this exact catalog before any gameplay consumer relies on it;
that follow-up is coordinated with the separate production-profile binding.
No live production admission or snapshot ownership state is activated here.

## Validation

Asset-free native tests cover source/codec boundaries, ordered collisions,
sentinels/whitespace/case, all modeled locked fields, late-failure atomicity, and
actual host/State/Packet/Client acceptance and protocol rejection. The optional
`tools/tests/smac_technology_catalog_probe.cpp` reads only an explicit source path
and prints the catalog or a supplied token's resolution; it never searches or
modifies an installation:

```sh
clang++ -std=c++17 -O2 -DNDEBUG -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -Isrc src/game/backend/settings/SmacTechnologyRules.cpp \
  src/loader/txt/SmacRulesTXTLoader.cpp src/loader/txt/TXTLoader.cpp \
  src/util/String.cpp tools/tests/smac_technology_catalog_probe.cpp \
  -o /tmp/smac-technology-probe
/tmp/smac-technology-probe /private/alpha.txt
/tmp/smac-technology-probe /private/alpha.txt --resolve 'ecology   '
```

The Release probe matched independent extraction of all 89 installed records
and resolved all 89 installed keys to their expected positions. Its token
results also matched all 16 token cases in the hash-pinned original fixture,
preserving the original requested-error distinction as `UNKNOWN`. The other
16 fixtures include ownership/numeric-reference contexts outside this API and
are not claimed as full-predicate differential matches. The original executable
was not rerun for this implementation. Missing arguments/file, an unsupported
option and malformed-source controls returned nonzero without successful output.

Local Linux/WSL validation used Clang/LLVM 15. The normal full suite passed 8/8
in 43.23 seconds; 19 selected host/catalog/protocol cases passed with allocation
tracking. The full ASan/UBSan/LSan suite passed 8/8 in 54.39 seconds with
`detect_leaks=1`; the same 19 selected cases also passed with allocation tracking
and no tracked memory/OpenGL leaks. That local build retains the existing
non-PIE sanitizer startup workaround; repository toolchain policy is unchanged.
The three changed production translation units passed correctness
analysis with zero failures in 1.65 seconds. Shared-header CI selection remains
unchanged; this receipt is explicitly the focused local analysis scope.

Four copied-production negative controls each returned exit 1 at its intended
native assertion: case-sensitive keys, removal of the None/Disable exception,
disabled frozen-catalog equality, and production-profile publication before a
late technology parse error. No intended checkout source was replaced by a
mutant. The initial normal compile exposed a test-only string/character delimiter
typo, corrected before the reported runs. Receipts are retained under
`/home/jhurliman/src/glsmac-technology-audit`.
