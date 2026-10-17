# Frozen TERRAIN action catalog

The SMAC host now reads the twenty positional `#TERRAIN` rows from the same
owned `alpha.txt` source used for the other frozen rules. The catalog retains
land/sea names and prerequisite tokens, the raw signed32 active work time,
order text, letter, keystroke and optional opaque annotation. Action identity
is the row index. Farm, Mine, Solar, Forest and Road are slots 0, 2, 3, 4 and 5.
Names never select an action, and this layer neither clamps work time nor
decides whether an owner can perform an order.

`GlobalSettings::GetSmacTerraformingRules()` returns the single optional owned
authority. Host preparation stages all source reads before publishing or
locking. Global16 transports the profile; World30 compares its exact contents
before reading world state. Both reject missing SMAC presence and bound the
outer frame before copying. SMAX has no inferred catalog. The nested V1 codec
owns twenty rows, bounds each text field to 256 bytes, validates ordered IDs and
signed32 integers, and rejects trailing fields. Its maximum is 42,508 bytes;
the existing aggregate lobby packet limit still applies independently.
These versions follow the separately integrated research Global15/World29.

The text reader uses the existing unique-section helper and an explicit strict
port subset: twenty nonblank/noncomment rows; eight comma-separated fields;
an optional ninth annotation; unquoted nonempty fixed text; complete signed32
decimal work time; no NUL; at most 4,096 bytes per row. Missing/duplicate
sections, different row counts, unsupported quoting and malformed columns
reject. This does not claim the original parser's unmeasured recovery rules.
Other section parsers are unchanged.

The retained original full-loader capture
`tmp/reference/former-command-loaded-inputs-20261017.json` has SHA-256
`d13aba8981ec971c523dfd5360bd42e130a94978107640234913bd0fdc4a042e`.
It records the two resolved prerequisite integers and active time at row
`0x6afd00 + 32*id`, offsets 8, 12 and 24. The installed source digest is
`38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
The retained `linear-0059e110.json` shows the active integer store at
`0x59e1f8` directly after the numeric reader; no time clamp is added here.
The public fixture contains those sixty numeric outputs with synthetic names
and text. No installed input rows or original executable bytes are included.

The optimized UBSan probe passed all twenty installed parser → owned codec →
token-resolution projections against that pinned capture, and independently
checked the committed fixture literals. No original execution was rerun.
Four changed production translation units passed targeted LLVM15 analysis;
those files and the new native test TU passed syntax checking. Five registered
native groups cover the literal projections, strict text admission, codec
bounds, host/world equality and outer-frame rejection. The existing actual
Client malformed-download matrix includes late TERRAIN changes/truncation and
oversized frames. Those linked engine groups are pending the shared cached
integration run. No full engine or platform suite was started for this slice.

Former progress, cancellation, terrain replacement and rendering are separate
live consumers of this catalog, not effects of loading it.
