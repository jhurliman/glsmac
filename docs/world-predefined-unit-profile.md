# World download binding for predefined unit catalogs

World header V8 appends an explicit presence flag and the complete frozen SMAC
predefined-unit profile after the existing faction profile. SMAC requires it;
SMAX requires absence. V1-V7 and unknown world header versions are rejected.
GlobalSettings remains V7 and the nested unit profile remains V1. This is a
versioned download contract, not a stable disk-save format.

The writer validates and pre-encodes every profile before appending any header
bytes. The reader uses the existing `SmacPredefinedUnitRules` codec with the
frozen technology catalog: it validates bounded raw fields, rebuilds derived
records through the actual catalog producer, and compares the complete owned
encoding with the established host settings. It neither reads local source
files nor installs new settings. There is no duplicated cost calculation or
separate list of equality fields in the world codec.

`Game::ConfigureWorldDownload` invokes this exact validator before reading or
allocating the replacement Map. A rejected unit profile therefore leaves the
previous map, settings, managers, RNG, turn/completion state and frontend queue
intact, and uses the existing safe-disconnect path. The JSF32, turn and body
codecs are unchanged. Later malformed body or startup failures retain their
existing terminal behavior; this change does not make the whole load
transactional or prove multiplayer lockstep.

The profile supplies owned source and derived design metadata, not live unit
Defs, technology ownership, eligibility, mutable faction state or conventional
unit rendering. See [the host profile](smac-predefined-unit-host-profile.md) for
its bounded ingestion and transport policy.

## Regression boundary

The real writer test checks exact V8/presence/codec bytes for SMAC and SMAX and
leaves following body bytes untouched. Pure mismatch tests vary every source
table, late names/description/numeric fields, source count and derived outputs
through valid producer inputs. Invalid cases include V7, absent or mistyped
presence/payload, nested version, late derived fields, trailing/truncated bytes
and the nested profile budget. All preserve frozen settings. The earlier
faction malformed-presence helper now stops at its field boundary instead of
assuming it is the final header field.

The same invalid prefixes pass through the actual Client final-chunk dispatcher
both immediately and queued. These new cases retain an existing map and check
its identity and bytes, the other simulation state, and zero startup/gameplay
callbacks. This is an asset-free rejection test; it does not claim a successful
asset-bearing Client map constructor. The existing real snapshot writer -> body
decoder -> InitComplete fixture covers SMAC and SMAX continuation, canonical
instances, RNG and turn/completion state, and requires the complete resulting
snapshot to equal the source bytes, including the new profile.

Source ancestry is host commit
`efb6c5c516f19bc4fdb9393460d9087fdc4bd7bb`. This predates the separately integrated
canonical-roster Client handshake; integration must retain that handshake and
its envelope admission rather than replacing its fixtures wholesale.

Copied-production-TU controls remove only unit-profile equality or replace the
written profile payload. Both fail the corresponding native assertion with
exit 1; intended source files and the ordinary application binary remain
unchanged. No original assets or human research files are committed.

Normal Linux CTest passed 8/8 in 61.80 seconds. ASan/UBSan/LSan with
`detect_leaks=1` passed 8/8 in 110.81 seconds, using the established local
non-PIE sanitizer configuration. Nineteen focused header/writer/Client/restore
tests also passed with allocation tracking in both the normal and sanitizer
builds. Explicit
correctness analysis covered the changed codec and its actual Game consumer,
with zero failures in 17.31 seconds. The preceding host commit records the
shared-profile header scan across all 312 eligible production translation
units; this world change adds no profile API. No Windows/macOS run is claimed
for this source branch.

Receipts are retained outside Git at
`/home/jhurliman/src/glsmac-unit-weapon-parser/world-units-*`.

## Predefined frame allocation boundary

Review of publication `9a51bf3d764c0bab11a42c8a1276e6f4289695f9` found that the
outer predefined-profile string was read without a limit before the nested
codec checked its 24 KiB budget. The world reader now passes
`SmacPredefinedUnitRules::MAX_WIRE_BYTES` to the existing bounded `ReadString`
API. That API checks the declared frame length before allocating or copying the
payload, scanning its checksum, or advancing the payload cursor. This avoids
the extra oversized profile allocation; it does not add an aggregate world
download limit or change the other header profiles.

Two focused native regressions distinguish this boundary from eventual codec
rejection. Complete 24 KiB + 1 and 96 KiB frames, with valid and corrupt
checksums, must report the frame-limit error and retain the exact payload-start
cursor. Valid-checksum frames are then read independently to prove they were
complete and their following body remained intact. A separate valid profile
padded only through admitted display metadata reaches exactly 24 KiB, passes the
actual semantic codec, and passes the bounded string read without consuming its
following body. Thus the limit is inclusive and is independent of malformed
codec-content rejection.

The existing real Client immediate/queued rejection matrix includes both
oversized lengths. It still requires safe disconnect, retired download
callbacks, unchanged previous map/settings/managers/RNG/turn/frontend state and
no startup callbacks. A copied production TU using the old unbounded read fails
the frame-admission assertion while the exact-cap codec case passes. No huge
allocation failure, allocator reuse or out-of-memory condition is needed to
expose the regression; the intended source remains unchanged during the control.

On that publication ancestry, the fixed Linux/Clang15 application passes all
eight registered CTests in 75.28 seconds. Six focused groups (the two boundary
groups and four real Client groups) pass with allocation tracking in 4.16
seconds and with ASan/UBSan/LSan plus tracking in 15.66 seconds. Explicit
`WorldSnapshotRules.cpp` correctness analysis has zero failures. An optimized
Release production-TU baseline passes both boundary groups; the copied old TU
passes the exact-cap group and fails the bounded-admission group with exit 1.
These are focused sanitizer receipts, not another full sanitizer-suite run.
The established local non-PIE sanitizer link workaround is unchanged.

The focused native-filter expression is
`^(world rules header bounds predefined|world predefined profile exact|real final download chunk rejects invalid world profile before map allocation )`.
Commands, copied control sources, logs and hashes are retained under
`/home/jhurliman/src/glsmac-predefined-world-frame-bound/`. Hosted and platform
checks after publication remain separate.
