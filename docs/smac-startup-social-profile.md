# Frozen startup social model metadata

The SMAC host now freezes the four selected model-zero source rows alongside
the existing faction startup metadata. This is the source prerequisite for
the later first-command social refresh; it does not calculate or publish
runtime ratings.

`RE-SMAC-START-FACTION-001` establishes the early model selection as index zero
in each category. `RE-SMAC-START-FIRST-001` and the retained-morale/first-command
handoffs distinguish that early selection from later intrinsic construction
and refresh. In particular, this profile does not supply a default retained
MORALE value or move late faction SOCIAL effects ahead of first founding.

## Source admission and ownership

The existing `FactionTXTLoader::ParseStartupRules` host path reads `alpha.txt`
once, validates its lookup tables, and requests the seven canonical faction
files. Before any faction-file callback, it now also validates the 19 nonempty
`#SOCIO` rows, the canonical category order, and the four positional model-zero
rows at zero-based positions 3, 7, 11 and 15. These positions include the three
preceding label/description/category rows. Each selected row owns its normalized
name, prerequisite and modifier text; the profile stores them once globally.
No category or effect is inferred from a cosmetic model name.

The supported startup domain requires exactly three comma-separated fields,
prerequisite `None`, and empty modifiers. Thus the installed Frontier, Simple,
Survival and None rows are supported, but a modified nonneutral first model
fails explicitly. Other model rows are counted to preserve the positional
layout and otherwise remain uninterpreted. This is not a complete social-model
parser or model-selection eligibility implementation.

Names are nonempty and limited to 256 bytes, excluding NUL, CR/LF, comma and
semicolon. Lexical trimming and comments use the existing faction source
policy. The byte limit, required layout and neutral-only subset are explicit
GLSMAC ingestion policy, not a claim about original malformed-input behavior.
Direct `MORALE` remains an unsupported faction keyword in this checkpoint;
its newly measured scalar/sticky-flag projection is separate work.

## Exact transport binding

Faction profile V2 encodes four owned name/prerequisite/modifier triples before
the existing seven faction definitions. Decode requires the exact count, limits
each string before allocation (256, 4 and 0 bytes), validates the full result,
and rejects trailing fields. Equality includes every model source string.
Faction V1 is unsupported; a receiver never inserts guessed defaults.

GlobalSettings V8 and world header V11 require the new profile. Their previous
versions are rejected before settings/world publication. Existing host staging,
locked profile equality, complete packet-size admission, SMAX absence and
join-without-local-file-read behavior remain in force. World header validation
still precedes map allocation. This is prefix rejection, not transactional
rollback of a subsequently malformed world body.

The existing opt-in `smac_faction_startup_probe --models` route uses the actual
installed-source parser and production codec to display the four owned rows.
No original source file or binary is included in the repository.

## Validation

Source base: `445eba1d2c8b5a8cb1a7021d8ddf08dc509b4476`.
Validation uses the dedicated Linux/WSL Clang 15 builds, with receipts retained
under `/home/jhurliman/src/glsmac-unit-weapon-parser/social-profile-*`.

The native tests cover model source positions, bounded field admission,
source lifetime/copying, exact equality, old versions and malformed framing.
Existing actual Client settings and immediate/queued final-download tests now
reject changed or invalid model rows while retaining their state/callback
assertions. The asset-free complete writer/body/initialization roundtrip stays
in the full suite; it does not establish rendered or socket-driven startup.

Two independently compiled copied-source controls remove model equality or
neutral admission. The first fails the actual Client, frozen-settings and
world-header assertions; the second fails before faction source reads. Intended
checkout source and normal binaries are never replaced for these controls.

The installed production reader accepts all four stock rows and roundtrips the
complete profile. The existing original comparison still matches 16/16 parsed
record/count projections and 7/7 installed faction source bindings. Those
comparisons exclude runtime social refresh; the stock model read is source
validation, not a newly executed original loader experiment.

Final source receipts (Clang/LLVM 15, two analysis jobs):

- Full normal CTest: 8/8, 88.81 seconds.
- Full ASan/UBSan/LSan CTest: 8/8, 209.79 seconds, leak detection enabled.
  The existing local sanitizer configuration uses non-PIE executables.
- All 28 focused profile/settings/world/Client groups pass with allocation
  tracking in both builds. No OpenGL or tracked memory leak is reported.
- Full shared-header correctness analysis: 328 production translation units,
  zero failures in 282.88 seconds.
- Both copied-source controls exit 1 at the intended assertions.
- Release/UBSan parser comparison: 16/16 original parser projections, 7/7
  installed faction bindings, and all four model rows survive production
  parse/codec roundtrip. The existing technology probe now gets its model
  metadata through the same source parser and still matches 107/107 original
  reset/grant projections plus 107 exact codec roundtrips.

The first comparator invocation used a nonexistent report filename and failed
before reading evidence; the corrected invocation used the pinned existing
`startup-faction-inputs-original-x86-results-20261016.json`. This was a command
path correction, with no source or oracle change.
