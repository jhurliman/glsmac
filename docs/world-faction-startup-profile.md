# World binding for frozen SMAC faction startup metadata

World header version **7** appends the frozen faction startup profile after the
version-6 world-size catalog and accepted selection. It writes an explicit
presence Boolean and, for SMAC, the existing version-1 faction codec payload.
The writer validates and encodes that payload against the frozen technology
catalog before writing any header bytes. SMAX retains absent SMAC profiles.

The reader requires the same frozen profile presence and compares the complete
owned profile after decoding it against the session technology catalog. This
includes canonical port/source/section bindings, ordered raw bonus pairs,
derived tags and parameters, supported records beyond the shared eight-record
budget, and non-record numeric metadata. It does not collapse duplicates,
normalize raw spelling for equality, or treat numeric TECH as a technology ID.
Versions 1 through 6, unknown versions and malformed or different profiles
reject. No local faction files are read and no session fields are changed.

The actual download reader already validates this header before reading or
allocating a replacement map. Its immediate and queued Client paths therefore
reject missing, different or late-malformed faction data while retaining an
existing map and the established session. The JSF32, turn/completion and world
body codecs are unchanged. This is rejection before body restoration, not
transactional rollback of a malformed later body or a stable save-format claim.

This completes transport binding for the
[frozen host profile](smac-faction-startup-host.md). It applies no technology
ownership, social rating, facility grant or other startup effect. The pure
[parser contract](smac-faction-startup-profile.md) continues to define the
supported source subset and its original-executable evidence.

## Regression boundaries

The native writer test checks the exact version/presence/payload and unchanged
following body bytes for both rulesets. Frozen-equality tests vary all twelve
bonus positions in every faction (84 valid changes), plus order, scalar omission
and scalar keyword/tag in each faction (21 more). Late malformed cases cover
source bindings, derived fields, out-of-range or mistyped integers, missing and
truncated frames, and trailing profile fields.

The real Client final-chunk dispatcher runs those late-profile cases immediately
and queued, with an existing map retained. It checks the same map pointer and
serialized bytes, settings, managers, RNG, turn/completion flags and absence of
startup callbacks. These fixtures preload frozen settings; the preceding host
slice separately tests the actual State/Packet handshake and readiness callback.
They do not restore an asset-bearing valid world body.

The older technology-presence malformed fixture now stops at the actual
technology boundary instead of assuming it is the final header field. This
keeps its named missing/mistyped-field cases meaningful after appended profiles.

Source ancestry is host profile `ccff31f9a72ae1616a7c9a1b28131c48ab7e17c9`.
On the local Linux/LLVM 15.0.7 toolchain, normal CTest passed 8/8 in 60.08
seconds and ASan/UBSan/LSan CTest passed 8/8 in 81.77 seconds, with leak detection
and halt-on-error enabled. The sanitizer executable uses the established local
non-PIE workaround. All fourteen focused cases also passed with allocation
tracking in both builds (nine header cases, the actual writer and four Client
cases); both runs reported no tracked leaks.

Two copied-production controls failed their intended assertions: removing the
faction equality check accepted different raw/derived metadata, and writing an
absent faction flag violated the actual writer's payload contract. Intended
sources were not mutated. The combined host and world source passed the full
LLVM 15 correctness policy over all 309 Linux production translation units with
four jobs in 113.91 seconds, with zero failures; the explicit Windows-only
implementation is outside that compilation database. This covers the shared
Settings.h scope left pending by the host slice's focused analysis. No human
research files or original assets are part of this change, and no Windows/macOS
validation is claimed here.

## Focused publication

The separate publication is stacked on host transport PR #123 at
`ad6f765355e6a4f3186ce4c0b1320563ea0b3ea3`. Source commit
`3f0d4d7a0d5ca282c4fdf18f8da9098892130fdb` applied without conflicts as
`12ecc817ad75f0b709322d1bd54d9c3c1c973e41`; the production header, dedicated
tests and malformed-header fixture are byte-identical. That exact narrower
ancestry passed normal CTest 6/6 in 37.57 seconds, including the actual Client
dispatcher cases. A copied registration-only translation unit ran the complete
header test group and both copied-production controls failed their intended
assertions. Focused production header analysis reported zero failures in 1.32
seconds. These are separate from the integrated-source full sanitizer and
309-unit analysis receipts above; no full publication sanitizer claim is made.
