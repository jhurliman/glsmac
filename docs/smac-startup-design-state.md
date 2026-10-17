# Live SMAC startup design state

Claimed new-world Start now prepares owned custom design blocks immediately
following the existing named technology grants, before `create_world` and
`start`. The same paired simulation value survives world transfer. No UnitDef,
map unit, base production target, renderer choice or research upkeep is changed.

## Admitted producer

The producer uses the already admitted simulation IDs, independently of lobby
slots. It accepts human startup with empty custom blocks, the checked named
ownership and component capability sets, and the relevant original component
semantics. The canonical seven source profiles have their checked single named
technology; extra ownership, changed availability or unproved chassis, weapon,
armor or reactor semantics reject before the result is published. Unknown and
disabled prerequisite references both remain unavailable, with their distinct
frozen metadata retained. This is a bounded port admission policy, not a general
automatic-design or workshop classifier.

Morganites receives Infantry/Gun/Synthmetal/reactor 1, Defensive plan 2; Spartans
receives Speeder/Gun/Scout/reactor 1, Recon plan 3. Both use explicit custom ID
`64 * simulation_id`, no abilities, carry or faction exclusion, and active=true.
Cost calls the existing production `CalculateCost` and then caches its low byte.
Compatible cosmetic and cost changes are accepted as a bounded composition of
that calculator with the checked recipe, not newly original-executed variants.
No name is used to classify a component. The other five admitted capability
sets produce no custom record. The frozen predefined Scout at ID 2 is checked
for its required component/plan semantics before it is used as fallback.

`GenerateEmptyOwner` and `MarkAndSelect` expose distinct stages. Generation
sets the prototype flag at original global difficulties 0/1 and leaves it clear
at 2..5. The marking pass visits records in increasing explicit ID order, marks
active records prototyped and retains the last active Recon as starting Scout,
otherwise ID 2. Thus Morgan selects predefined 2 and Sparta its custom record.
The native ordered multi-record marking test checks the loop contract; the
new-world producer still admits only empty initial blocks and emits at most one
record. It does not claim occupied/exhausted-block generation.

The Game consumer resolves its signed64 global setting through the existing
canonical difficulty identities: Citizen 0, Specialist 1, Talent 2, Librarian 3,
Thinker 4, Transcend 5. It rejects unknown encodings, integer overflow and unknown
names. It does not add 3 to an unvalidated value or substitute a participant's
stored difficulty. Only the consumed original global index is persisted.
Stored difficulty and network state are validated supplied inputs in the
comparison probe, not unused runtime fields. This human producer does not
activate the separate AI stored-difficulty path or earlier network extra-TECH.

New-world composition iterates each admitted owner in the existing order.
Extending the five empty-output baseline profiles across admitted IDs and
settings is an explicit GLSMAC per-owner composition policy. The original
all-owner/all-difficulty grid directly covers Morgan and Sparta; the other five
baseline observations remain owner 1/global 3. No simultaneous original full
world-start equivalence is claimed.

## Ownership, restore and consumer boundary

`SmacStartupDesignState` owns one block for each admitted simulation ID, the
consumed global difficulty and each block's saved starting-Scout selection.
Custom records retain explicit IDs, frozen component indices, reactor,
24-bit ability mask, cached byte cost/plan/carry, active/prototyped flags and
uint32 exclusion. They borrow no source buffer or mutable native object.

The separate `SMSD` codec is V1 with a 16 KiB port budget. It admits 1..7 unique
owners, IDs 1..7, at most 64 sorted records within each owner's block, valid
frozen component/reactor indices, bounded fields and an owned or predefined 2
selection. The saved selection can reference an inactive record: restoration
preserves the captured state rather than rerunning selection. Structural
restore does not infer future workshop eligibility from those fields.

The enclosing simulation codec is now V2; world download is V10. V1 simulation
payloads and V1 through V9 world downloads reject explicitly. SMAX still has no SMAC
runtime payload. The existing 64 KiB aggregate and 8 KiB technology budgets remain.
The new framed 16 KiB reader checks the complete string extent/length before
allocating its payload, then builds owned values with strict counts, versions,
field widths and trailing-byte checks. Outer simulation validation requires
exact roster/technology/design owner cardinality and pairing.

Restore preserves cached costs, inactive records, prototype flags, explicit
non-dense IDs and Scout selection exactly. It does not regenerate designs,
recalculate their costs, replay technology grants or remark prototypes. Binding
compares the saved global semantic difficulty with the accepted canonical
settings, before Map allocation and again after setup/deferred registration.
The existing epoch/session/Player identity guards and claimed Start ownership
also govern designs. Reset, cancellation and terminal initialization failure
retire the entire paired value.

`game.get_startup_designs(player)` returns detached scalar/array objects with
simulation ID, global difficulty, starting Scout and complete custom records.
Actual create/start script tests mutate these copies and re-read unchanged
state. This supplies the next founding/unit consumer with authoritative IDs;
it does not create an infantry placeholder for a Spartan Rover.

Prefix rejection is atomic with respect to world publication; later map/body
or setup failure remains terminal rather than a whole-world transaction. Tests
use the actual Client packet dispatcher, claimed Start and asset-free Map
fixture, not a socket-polling or rendered-world smoke.

## Original comparison and validation

The opt-in `tools/tests/smac_startup_design_probe.cpp` reads external installed
alpha/faction sources and uses the actual table parsers, technology producer,
Generate/Mark stages, composed NewWorld producer and codec. The comparator
`tools/reference/compare_smac_startup_designs.py` consumes only literal captured
`actual` records, never imports the reference oracle or copies game data.

The Release+UBSan production probe matches 1,008 human generator-gate cases and
nine baseline projections, plus 1,017 exact production codec roundtrips. The
168 AI controls in the 1,176-case report are deliberately excluded. Earlier
baseline records omit carry, so only their captured fields are compared; the
later human grid includes carry. Founding, placed-unit state, upstream network
grants and the remainder of the first command boundary are not compared.

Pinned evidence:

- Original executable: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
- alpha.txt: `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
- Difficulty/owner gate report: `1d9c042254a5936e7dc8ba05f949140f25c4330f908f797f4fa42fa43124931a`;
  public fixture: `235404b572df4111f49e6600ccc32a049b40a5056169125ab9e3a5553d4a304a`.
- First-base-order report: `cb4ab8c026f4f57ec2aa3bc1c4647ef40c6f5752c98119af9a1a5e19eda86dc5`;
  public fixture: `d7d7b1aae73dda634850449b44fb53261656280a1c0f3f433b45c199913147e9`.

The human handoffs remain separately owned in `docs/behavior/`; this change does
not edit them. Reports, installed sources and comparison output stay outside
Git. The output also records all eight installed input hashes and the compiled
production probe hash.

On source parent 7eb1fef3, Linux normal CTest passes 8/8 in 78.29s; full
ASan+UBSan+LSan CTest passes 8/8 in 142.50s with leak detection and halt-on-error.
The 40 focused native cases also pass with allocation tracking in both builds.
Six separately compiled copied-production controls each fail the intended
assertion: omitted generation, fixed premark flag, hardcoded cost, extra
ownership acceptance, omitted late difficulty binding and restore remarking.
Intended source/object files are never replaced by those controls.

Retained evidence is under `/home/jhurliman/src/glsmac-unit-weapon-parser/`:
`design-state-{ctest,asan-ctest}.log`, focused tracking logs,
`design-state-controls/results.json`, `startup-design-original-comparison.json`
and `design-state-analysis/`. Initial draft fixture corrections included canonical MORGANITES spelling and
selecting difficulty after fixture initialization. The retained callback-count
expectation failure distinguished completed create observation from the start
observer blocked by changed difficulty. None is counted as an original or
production negative control.

After fixture-only tightening, all 54 selected native tests pass with allocation
tracking in both normal and sanitizer builds, including the older independent
generic-header rejection matrix. Final production correctness analysis covers
317 translation units with zero failures in 243.71 seconds. The Release probe
also rejects six invalid supplied-context requests. No Windows, macOS, live
rendering or full-match result is claimed for this source receipt.

## Focused publication receipt

Publication implementation `0bd69fe3` applies the source change directly to
PR136 head `55b1331f`. The parent retains the canonical roster, frozen profiles,
live simulation admission and initial-turn prerequisites. PR136 remains an
explicit merge dependency; this receipt does not claim that it has landed.

The feature delta remains 22 files. Production changes, dedicated test files,
synthetic source fixture, probe and comparator match source `19518262`.
Only the new test source/include/Add registration placement is adapted to the
parent's registry order; all existing registrations remain. An independent
read-only review checked that union and the automatic Backend fixture merge.

On this actual publication implementation, full normal CTest passes 8/8 in
74.68 seconds, and all 54 selected native groups pass with allocation tracking.
The six separately compiled production controls each fail their intended
assertions. Focused LLVM 15 analysis covers Game, GameSimulation,
SmacSimulationState and SmacStartupDesignState with zero failures in 17.79
seconds. Logs use the `design-publication-` prefix in the retained validation
directory above. The full sanitizer and original-comparison receipts remain
the source receipts; they were not rerun unchanged on this publication ancestry.
