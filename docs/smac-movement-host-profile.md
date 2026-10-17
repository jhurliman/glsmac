# Frozen SMAC movement rule in host and world state

The host now freezes the parsed road movement rule with its other SMAC profiles. `GlobalSettings::GetSmacMovementRules()` exposes the owned profile; `Quantum()` derives Q by clamping its raw signed32 value to 1..100. There is no independently writable Q field, local-client rules-file read, or live Unit activation in this change.

The arithmetic and original evidence remain documented in [smac-movement-rules.md](smac-movement-rules.md). This change supplies admitted game context for those calculations; it does not establish terrain cost, current allowance, social ratings, starting packages or command eligibility.

## Host admission and transport

`InitializeHostRules` parses movement from the same `alpha.txt` string already used for the other profiles. All profiles and the existing mutable settings tail are staged and serialized before any profile is published or the session locks. SMAC preparation still reads exactly the canonical eight sources; repeat preparation uses the frozen profiles. SMAX and joining clients do not read these sources. Reset and reselection clear the movement profile with the other frozen data.

Global settings V9 appends an explicit movement-presence flag and the existing strict V1 movement codec after the predefined-unit profile, before map settings. SMAC requires presence; SMAX requires absence. The reader validates the complete incoming record and every frozen profile before committing settings. Equality compares the raw rule, so -1 and 0, or 100 and 101, remain different authorities even though their derived Q values agree. Versions before V9 and unknown future versions are rejected; this is an intentional wire-format change.

The existing complete packet-size admission includes the added profile. Malformed movement, late map/tail failures, and locked profile differences preserve existing settings, including the world-size lifecycle lock. This is settings-record staging, not a whole-world transactional restore claim.

## World binding

World rules header V12 adds explicit movement presence and the same owned codec after predefined-unit metadata. The writer encodes all profiles before emitting the header. The reader checks presence, strict framing and exact raw equality against the frozen session before reading RNG, turn, simulation or map data. V1..V11 headers are rejected. SMAX retains an absent movement profile.

The snapshot does not replace the session's movement rules or re-read installed files. Header rejection preserves an already retained map and its bytes, settings, RNG, turn/completion flags and callbacks in both immediate and queued actual Client final-chunk paths. Malformed world-body rollback is outside this guarantee.

## Validation

Source ancestry: metadata b438449cd90daf93fa248690cba3195af4acc873 plus the pure movement implementation 8cdf38e20d0c773ddf8d75c483bc9bb8b4781f36 (cherry-picked as 07d6fee5). The synthetic host rule deliberately uses raw -1/Q1 rather than installed defaults.

Tests cover raw signed32 endpoints and clamped values, host cache/read counts, source failure/retry, same-Q raw mismatches, malformed/absent/unexpected and old-version records, late failure preservation, and actual host State serialization through Packet/Client readiness. Independent world frames verify the new field order, valid SMAC/SMAX roundtrips and rejection before the body. Actual download fixtures retain their previous map during movement-header rejection.

The final normal Linux suite passes all 8 CTests (82.28s). The complete ASan/UBSan/LSan suite also passes 8/8 (152.74s), with `detect_leaks=1:halt_on_error=1` and UBSan halt/stack traces. Matching 26 focused groups pass with allocation tracking in both builds. The local sanitizer linker retains the previously documented `-no-pie` workaround; no production suppressions or sanitizer policy changes were introduced.

Three copied production-TU controls fail their intended assertions: skip frozen settings raw equality, skip world movement equality, and write false movement presence. The first two distinguish raw authority from its derived Q; the last checks the independent actual writer layout. Intended sources are never mutated for these controls.

Full LLVM 15 correctness analysis passes all 329 production translation units with zero failures (246.46s, two workers). The Windows-only network implementation is outside that Linux analysis. No native Windows/macOS or rendered gameplay result is claimed here.
