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


## Focused publication

Publication commit `e3c91cf4fc87dece22772ff16f5aa81811f093eb` applies this exact
codec/test change atop host PR #128 at
`79f9cbf386508934157aaad141a65ae0fb0cdc25`. Its joined prerequisite base retains
#124, #125 and #115 histories. No integration branch or later canonical roster
implementation is imported; the preceding source/integration distinction still
applies.

The actual publication application passed all six registered CTests in 45.75
seconds. Its world codec and Game consumer passed explicit correctness analysis
with zero failures in 16.50 seconds. A copied-production control removing only
complete unit-profile equality fails the intended changed-catalog assertion
through this ancestry's full native/script suite. The earlier source-branch
sanitizer and tracked-filter receipts are not publication-head sanitizer claims.
The three non-ASCII arrow/dash sequences in the original note are corrected to
plain ASCII here; production and regression source are unchanged.
