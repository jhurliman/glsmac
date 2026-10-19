# Saved dominance history producer

The simulation now retains the original turn-major1000×8 signed-short history.
Fresh worlds clear the whole block independently of the working ranking table.
The existing ordinary full-ranking path publishes its fresh composite scores
into the turn being completed, before advancing the turn or exposing consumed
working scores. Inactive and absent positive slots publish zero; slot0 and all
other rows retain their bits. Writes stop at turn1000, and positive scores wrap
to their low signed16 bits rather than saturating.

Simulation format31 saves the entire little-endian16000-byte payload, including
slot0 and future rows. Formats11–30 migrate with history absent. Loading or
advancing a migrated campaign does not invent old samples.

The source authority is [dominance-graph](behavior/dominance-graph.md). Native
slot identities are represented by the port's existing stable simulation-owner
indices, as with its ranking table; importing original graph blocks and mapping
custom original loaded slots are not claimed here.

This is the storage/global-provider slice. The lower console still says
`History unavailable`: its native score-only rebuild, current-score scale,
seven-series colors, bounded reader and actual graph drawing are the next
integration. No curve or screenshot is fabricated for this producer-only change.

Validation includes all9 original writer contexts with complete-payload hashes,
all16000 raw byte values through the codec, fresh-world zeroing, actual simulation
and world save/restore, old-version absence, truncated payload rejection, and
the real complete-turn provider. Sixteen affected history, ranking, regional
strategy, summary, turn-queue and restoration groups pass. Targeted correctness
analysis passes3 production translation units. Focused ASan/UBSan instruments
the3 changed production units and the new test unit; other objects use the normal
cached build and leak detection is disabled. The portable source receipt also
passes9 writer contexts,17 graph contexts and6860 line requests; graph-coordinate
port validation is not claimed by those Python checks alone.
