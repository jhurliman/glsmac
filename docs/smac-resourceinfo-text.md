# SMAC RESOURCEINFO physical text

`SmacRulesTXTLoader::ParseBaseSquareRules` now consumes nine physical lines
after the unique `#RESOURCEINFO` header. Blank lines and semicolon-only comment
lines occupy a row and contribute zero values. The Base Square, Bonus Square
and Recycling Tanks triples come from positions 2, 3 and 5; labels can be
renamed or repeated. Later row text is ignored. This fixes the prior use of
the generic section reader, which skipped these physical lines.

Only this parser changes. Technology, movement quantum, production/facilities,
world-size and generic TXT section parsing retain their existing contracts.
The returned owned `BaseSquareRules`, frozen settings format and yield
calculator are unchanged. This is SMAC source ingestion, not new gameplay
resource scheduling or a SMAX rule inference.

## Conversion and admitted boundary

Numeric fields are trimmed at ASCII spaces/tabs. Decimal accepts a sign and
stops at the first nondigit. Unsigned leading `0d`/`0D`, `0x`/`0X` and
`0b`/`0B` select decimal, hex and binary respectively. A sign before a prefix
therefore follows ordinary decimal conversion (`-0x10` yields zero).
Empty fields, `*` and a nonnumeric prefix yield zero. `12junk` yields 12;
`0b102` yields 2. Conversion stores the low 32 bits and maps them explicitly
into signed32 without C++ signed overflow or out-of-range signed casts.

The port deliberately admits only numeric-prefix magnitudes through 2^32,
the largest magnitude in the measured fixture. Larger prefixes throw instead
of claiming unmeasured arbitrary-length conversion. Nonempty noncomment rows
require a label and three comma-delimited N/M/E fields; the fields themselves
may be empty. As before, the fourth value and later annotations are outside
the returned center-yield subset. Quoted fixed fields are rejected; quotes
after the fourth value's trailing comma are ignored annotations, including
the installed file's annotation. Comment-only lines remain zero rows; inline
semicolons before the fourth field are rejected because their remainder
handling was not established by the evidence.

Other explicit port rejection policies are missing/duplicate headers, early
EOF or a new section before all nine rows, embedded NUL or bare CR, and a
consumed physical line exceeding 1024 bytes after removal of CRLF's CR.
Exactly 1024 bytes is admitted. LF and CRLF are supported. A complete ninth
row without a final newline remains supported by the port, without a claim
about original EOF recovery. There is no generalized CSV or malformed-input
recovery implementation. Errors return no partial rules object; existing host
staging continues to prevent failed source publication.

## Evidence and tests

`RE-SMAC-BASE-TEXT-001` executes the original section/physical-line/name/token
readers and conversion through resource row stores, then the complete center
yield routines. Its 38 cases contain 608 yield triples. The executable is
SHA256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`;
the resource loader fragment is 0x59e2a2..0x59e2f3, numeric converter 0x642710.
The public input fixture is
`65fe760ee3c87687663e4d3e297533342fe804e35001a6a294761ef62bddfea6`;
the retained original report is
`693bed4c3a35e6888afb5275f0d42260795497ab5fe1acddacbea361263bedaf`.

The checked-in `SmacResourceText.h` contains only 37 self-authored text inputs
and literal projections: the nine selected values and 592 expected triples.
`tools/reference/project_resourceinfo_cases.py` verifies the public fixture
hash and query order before projecting it. It neither imports an oracle nor
runs the original executable. The installed text is not copied into Git.

Native tests run the actual parser into the unchanged yield calculator,
exercise physical-line shifts, endpoints and unsupported boundaries, and
freeze/serialize/restore two changed sources through the real settings APIs
before recalculating yields. They also verify that frozen technology,
production and movement profiles remain unchanged. Existing tests that
inserted skipped blank/comment lines within RESOURCEINFO were intentionally
replaced with measured physical-row expectations; optional fourth columns,
ignored annotations, renamed/moved labels and complete EOF remain covered.

The opt-in `smac_resource_text_probe.cpp` links the actual parser and yield TU.
`compare_resourceinfo_text.py` checks the pinned original fixture against its
literal outputs, optionally reading the player's installed `alpha.txt`
(SHA256 `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`).
An optimized local probe matches all 38 cases and 608 yield triples, including
the installed source. This validates three selected triples, not every
unused resource-table column or an entire original rules loader.

`RE-SMAC-FULL-RULES-001` independently reaches normal return from two original
complete-loader runs and confirms the installed numeric table. Its fixture
hash is `b8c7545a90a43f1160185d55748e3663127ab71e3cc489faf426d08639d62cda`,
report hash `0c27176eb1f42ccd50ac8252e5ec408f18ebf1069913c965849d324d67d60fdf`.
The no-draw boundary is argument-specific: loader(0) skips optional faction
setup and preserves the RNG pair, including after all 402 original
constructors. The normal setup caller forwards nonzero mode 1, which runs
faction setup and its shared palette-ID draw. Complete setup/command RNG
origins remain outside this parser's scope.
This parser change makes no RNG lifecycle claim and does not rerun either
original investigation.

Final source validation passed all 8 local CTests in 84.17 seconds. All six
native RESOURCEINFO groups passed with allocation tracking in both the normal
build and ASan/UBSan/LSan build (leak detection and halt-on-error enabled).
The optimized UBSan probe matched all 38 retained cases and 608 yield triples;
regenerating the owned fixture produced byte-identical output. Explicit
LLVM 15 analysis of the changed production TU reported zero failures in
2.78 seconds. No shared API or wire format changed.

Five copied-production controls each exited 1 at the intended assertion:
the old section reader, skipping physical blank/comment rows, strict integer
parsing, clamping wrapped signed values, and accepting an early inline
semicolon. Intended source files were never replaced for these controls.
The initial native-test macro compile error and the installed annotation
quote rejection were corrected before these final receipts.

The focused publication applies only source commit 3497c7cc to pinned main
386cbc9, without conflicts or fixture changes; all eight feature files
were identical before this receipt paragraph. That exact production/test
stack passed 8/8 CTests in 77.68 seconds, the six tracked RESOURCEINFO groups,
and one-TU LLVM 15 analysis with zero failures in 2.51 seconds. Its separately
built optimized UBSan probe also matched all 38 cases and 608 yield triples.
These publication results are distinct from the source sanitizer run above.
