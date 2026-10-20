# Conventional Unit flags16 initialization, mutation and persistence

**New conventional Units initialize native word+8 to0. Existing Units must retain it.** It is separate from DWORD status+4, stored visibility byte+39, owner byte+14 and duration byte+15. The [49 native controls](conventional-unit-flags16-authority-cases.json) bind the constructor, full Scout factories, visibility, relocation, ownership, capture-store, editor, countdown and record codecs. [Independent arithmetic/legacy translation](conventional_flags16_reference.py). No foreground interaction or graphics capture.

Pinned executable SHA256: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`. Frozen bundle: `tmp/reference/conventional-unit-flags16-authority-evidence-20261020`.49 literal instruction guards and33 explicit-literal Unit+8/+9 writers are included; the writer inventory is not an exhaustive pointer-alias proof.

## Initial authority

Actual record initializer **`5d8420(unit_id,design_id,owner)`** zeros word+8 at`5d8463`. It independently zeros DWORD status+4 (`5d84ad`), stored visibility byte+39 (`5d84e6`) and duration byte+15 (`5d8456`), sets owner/design, and initializes next/previous links to−1. Three complete calls start from dirty`0xa5` records for owners0/1/3 and produce flags0/status0/visibility0.

Actual factory **`5d8520(design,owner,x,y)`** calls that initializer (`5d85b9`), inserts/relocates (`5d863e`) and executes visibility update (`5d8646`). Two complete conventional Scout factory controls, owners1 and3 at`(22,38)`, begin with dirty flags`0xffff` and finish with **flags0, status0, stored visibility0**. Their only flags16 write is the constructor's zero store. Factory bookkeeping, position/stack insertion and visibility code execute; logging/UI services are supplied boundaries. The supplied Scout design row is the previously authenticated installed Scout Patrol design, not a claim to execute the complete design loader again.

The factory's separate artifact/editor branch can OR word bit`0x20` at`5d85ee`; that does not set the`0x40/0x400` pair. Do not generalize the Scout result to every package/editor profile.

The [live admission consumer](live-object-name-stack-admission.md) explains why the newborn Scout remains drawable despite visibility0: ordinary own Units bypass stored disclosure unless `(flags16&0x440)==0x440`.

## Which events preserve, set or clear the pair

| Path | Exact word+8 behavior | Evidence |
|---|---|---|
| New record / ordinary Scout factory | initialize0 | Complete native calls |
| Detach `5d80f0` then insert `5d81d0` | preserve whole word | Complete calls with0/440/ffff |
| Ownership conversion `4e3690(unit,new_owner)` | preserve whole word; owner/status/order are separate writes | Complete owner3→1 calls with0/440/ffff |
| Movement/capture owner-store `5b0c21..5b0c34` | copy captor owner, clear victim order, preserve flags | Declared original fragment with0/440/ffff |
| Disclose Unit `5cd150(unit,observer)` | OR stored mask; if observer!=0, `flags &= 0xfbbf` | Complete calls, observer0/1/3 |
| Disclose tile `5cd320(x,y,observer)` | same clear on admitted linked stack for nonzero observer | Complete two-Unit stack calls |
| Editor attributes `4f2f3d..4f2faf` | selected checkbox mask can independently set/clear40 and400 | Declared store fragment |
| Upkeep `577c78..577c9e` | countdown byte+15 reaching0 clears pair; starting0 does not | Declared countdown fragment |
| Modern native record transfer / old48-byte import | preserve original word+8 | Actual codec/translation fragments |

### Visibility is a real flags mutation

`5cd150` follows previous links to the stack head, ORs observer bit into each Unit's stored visibility byte, and walks next links. At`5cd2a0..5cd2a4`, observer0 skips the word clear; nonzero observer applies **`0xfbbf == (~0x440)&0xffff`**. `5cd320` has the equivalent`5cd635..5cd63f` test/store. Six two-Unit controls begin from`0xffff` and visibility`0x80` and check both records. Nonzero observer leaves flags`0xfbbf`, preserves every unrelated bit and produces visibility`0x80|(1<<observer)`. Observer0 leaves flags`0xffff` while adding bit1 to visibility.

The larger visibility routine`5cdaa0` contains the same clearing continuations, including`5ce61b`, `5ce7ab`, `5ceac5`, `5cecb3`, `5cf063`, `5cf59f`, `5cfa1d`, `5cfcc1`, `5cff31`. Their surrounding detection/diplomacy/native exceptions remain authoritative; this is not permission to clear the pair whenever any observer radius overlaps a tile. Apply the mutation when the actual admission/disclosure event is delivered, and update all linked Units that event visits.

### Relocation/ownership preservation is not whole-move preservation

The raw position/stack helpers do not write flags16. Higher-level movement`5abe40` separately changes word bit`0x4` (`5ac545`/`5ac623`) and reaches full visibility update at **`5adc51 → 5cdaa0`**. Consequently, a complete move can preserve the pair through relocation and then clear it through admitted detection. Do not reset flags on every move, and do not promise that every complete move preserves them.

Ownership conversion writes owner at`4e394d`, order0 at`4e395a` and masked DWORD status at`4e396f`; it does not reconstruct or zero flags16. The inline movement/capture owner-store has the same separation. The bounded owner-store control stops before the later presentation/visibility continuation; it is not a complete combat-capture trace. Native callers of conversion include`4e73bc`, `54bb40`, `582776`, `582fe3`, `583052`. Existing imported/captured Units may carry either bit; preserve their record through owner changes until a separately verified clearing event applies.

### Editor and duration

The editor fragment begins `flags &= 0xf81f`, then maps dialog selection bits1/2/4/8/10/20 to word bits20/80/100/200/40/400 respectively (hex). Thus selection`0x10` sets40,`0x20` sets400 and`0x30` sets both. Selection`0x40` sets **DWORD status+4 bit`0x100`**, not the flags word; this is another reason not to infer flags16 from a retained status DWORD. The exact dialog-mask mapping is captured; names/semantics of all editor checkboxes are outside this receipt.

The upkeep fragment reads byte+15. If already0, it leaves flags untouched. Otherwise it decrements; only the transition1→0 applies`&0xfbbf`. Controls from flags`0xffff` show durations0→0 preservingffff,1→0 producingfbbf,2→1 preservingffff. Retain byte+15 as well if imported/editor states must reproduce this expiry. Do not clear flags merely because the counter is0.

## Native save/import

Inside `5be250`, Unit block selection compares format version`95a29c` against **3**, established by`5beb9f: mov edi,3`. Version>=3 transfers **52×unit_count bytes** beginning at`9951b0` (`5bed16..5bed63`). This includes the two little-endian flag bytes at offsets8/9 unchanged. Plain FILE write/read fragment controls restore full52-byte records exactly for flags0/40/400/440/a55a/ffff, after overwriting the destination withcc. Neither load nor save re-runs the Unit constructor in this block.

For version<3, the old layout uses48 bytes per Unit. The translation reads old word+8 (`5bee1c`) and writes new word+8 (`5bee20`) unchanged while widening/rearranging other fields. Three complete translator fragments with440/a55a/ffff match an independent48→52 mapping byte-for-byte. **Old native format does not mean missing flags.** Preserve them rather than defaulting them to0.

These are bounded actual Unit-block/translation fragments with a memory FILE service, not a complete file save/load lifecycle. Compressed/encrypted branches and later whole-engine reconciliation were not newly executed. The native saved record already has enough information to retain the word; do not derive it from owner, visibility or status after import.

## Port schema recommendation

1. Add an explicit unsigned16-bit retained flags field to conventional authoritative Unit state, independent from its retained DWORD status. Initialize it to0 on a **newly constructed** conventional Unit, based on`5d8420` and the complete Scout factory controls.
2. Populate it from native Unit record bytes8/9 on import, including old48-byte records after translation. Preserve it on ownership conversion, stack/facing changes and raw relocation; apply the verified visibility/countdown mutations at their owning events.
3. Serialize the field explicitly and restore it before admission/rendering. New-schema readers should validate the unsigned16-bit range. Round trips should retain nonzero discriminators such as440/a55a/ffff, not just ordinary zero defaults.
4. For an **older port schema that omitted this field**, zero is a compatibility default for the older port's newly constructed conventional-Unit profile. That is a migration policy, not evidence that every historical/native record was zero. If a raw native record is also available, its word takes precedence. If flags were discarded from an imported record and no original remains, status/visibility cannot reconstruct them; do not claim a lossless migration.

No production state/schema edits are made by this research task.

## Verify

```
python tmp/reference/check_conventional_flags16.py --verify-only
```

49 controls,49 source guards and33 explicit-literal writer locations pass. The small archive contains numeric/source evidence and checker dependencies, with no terrain art or framebuffer captures. The writer inventory locates explicit Unit+8/+9 accesses in indexed functions; register-based aliases and bulk copies mean it is not a proof that no other path can propagate these bits. The concrete initial/default/save and mutation rules above rely on their separately executed controls and pinned source, not on that absence claim.
