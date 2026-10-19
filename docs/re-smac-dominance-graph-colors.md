# Graph color producer, lifetime and native-slot binding

Pinned terran.exe SHA256:
0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01.
Public behavior/dominance-graph-colors.md and adjacent reference/cases.

45f550 global graphics initializer:45f6b9 pushes236,10,palette0,PALETTE.PCX;
native5f0be0 load executes. Loop45f6d1 EDI580/ESI0,5fb010 samples y23,
31,39,47; native stores45f6ed→6aeeb4,45f700→6aeed4,45f713→6aeef4,
45f71e→6aef14. EDI+=8,ESI+=4 to32. New poisoned128-byte array control
capturesall32 stores, exact independent external PCX reconstruction.
Normal249,241,252,255,251,65,102,235;
alternate1,32,187,209,85,79,109,186. These supersede PE loader defaults as
gameplay-ready inputs; the earlier geometry case colors remain their declared
pre-producer values, not relabeled initialized values.

Fulltext Capstone direct-memory reference audit reports onlytwo direct stores
to first64bytes:45f6ed and45f700. Otherreferences are reads. This is explicitly
not a general alias-complete program analysis. Existing4297e0 actor/cache source
containsno normal/alternate table literals; it writes separate actor/CLUT
outputs. Do not conflate its faction PCX colors or territory70f818 CLUT with
these slot tables. No new full4297e0 cache-wrapper execution is claimed.

probe_dominance_graph_colors.py composes writer-free existing minimap slot
setup. It reexecutes only the native slot producer, then complete59de20 twice:
firstoriginalAlpha, secondemulated Alpha with FACTIONS rows1/3swapped. Both
Alpha opens in custom pass receive same virtualbytes; hostfilesunchanged.
59de20 loads all7 sequentialtext records95a4c8+slot*59c and invokes full
59d1d0 parser,45eb70 file selection/actualPCXdecoder for each. Same existing
actor/cache4297e0 and sprite storage5fc7d0 presentation sinks apply. Memory
watch6aeeb4..6aef33 andactivepalette959aa8..959ea7 recordszero writes during
eachcomplete list/parse/image pass. Before/after128bytes andpalette match.

Actual graph color selection fragment463d87..463dd3 executes per slot1..7
for eachload, plus7calls withactiveMask959ff1=2,21total. No score provider or
graph geometry loop rerun. StackESP+20 points toactual parsedrecord+1c,
ESP+10=4*(slot−1). String compare6a82dc='SPARTANS', case-sensitive; ordinary
read463dc1 6aeeb8+offset, alternate463dcd 6aeed8+offset. These normalize to
table[slot]. EAX at463dd3 iscaptured before primitive arguments are pushed.
Record+4 isloaded filename; +1c isretainedgraph comparison key; stockvalues
equal, but do not assign bothfields oneunproven general custom meaning.

Default actualparsedkeys GAIANS,HIVE,UNIV,MORGAN,SPARTANS,BELIEVE,PEACE;
custom UNIV,HIVE,GAIANS,MORGAN,SPARTANS,BELIEVE,PEACE. Bothselect graph
indices241,252,255,251,79,102,235 byslot. Sparsecurrentactive onlyslot1 leaves
all7selectorsunchanged. Defaultcanonical→nativeordinal isthus explicit source
order; prior setup-wizard-choice-transfer receipt proves selectedloadedslot
preserved into human/newgame. DenseportIDs needmapping, not tableindex reuse.
Sevenhistoricalseries remains priorgraphreader authority independentofactive
participants; no newhistorywriter/geometry run or arbitraryorder recovery.

New receipts: dominance-graph-colors-original-x86-20261019.json, developmentlog,
dominance-graph-color-table-direct-refs-20261019.json. Source asset hashes/public
numericcases pinALPHA/PALETTE and Gaia/University files; no artpayload copied.
