# Technology map revelation

The research grant can prepare and commit actual map revelation without invoking
scripts between its technology and map writes. `PrepareSmacTechnologyReveal`
authenticates the current positive simulation participant and saved live worker
knowledge. It copies every actual tile's explored bit and complete GLSMAC
improvement snapshot for that recipient, including sea and previously explored
tiles, and snapshots every registered Base's current population and known bit.
Claims, Unit visibility, other observer memories, current terrain and entities
remain unchanged. Commit only swaps the prepared owned knowledge.

The existing saved `all_map_revealed` mask is the sole retained capability.
Already-set recipients return no prepared change: repeat acquisition must not
refresh old snapshots. Restore retains both the capability and old memory.
The actual research caller owns startup/shortcut/technology-effect admission;
the accepted `TECH-REVEAL-001` checker isolates effect bit 8, independently from
Secrets bit 1. Installed technology 64 has flags 73, but its numeric ID is not
the effect predicate. This helper does not implement grant scoring or Secrets.

Knowledge V4 appends a uint32 count and fixed 18-byte Base observations after
the Pact suffix: uint64 stable port ID, known-by mask, population-presence mask,
and eight population bytes. Absent values encode zero; known zero differs from
absence. IDs, duplicate records, population 0..127, exact extent and the existing
4 MiB aggregate budget are validated. Variable lengths are scanned before
record allocation. Live binding requires referenced Base IDs to exist; staged
Base removal discards only that Base's remembered record. There is no second
population authority: these are remembered snapshots, not current Base values.

Focused tests exercise actual preparation without publication, mixed-owner
Base snapshots, full land/sea memory copying, no visibility changes, repeat and
world restore preservation, Base retirement and malformed codec/binding input.
They compose live GLSMAC entities using the accepted semantics; they do not
rerun the original acquisition or claim rendered fog-of-war parity.

Validation: cached incremental build and 13 affected native groups passed with
memory tracking, including real malformed Client downloads. Four changed
production translation units passed targeted LLVM 15 analysis.
