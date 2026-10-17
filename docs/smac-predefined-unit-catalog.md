# Owned SMAC predefined unit catalog

`SmacPredefinedUnitCatalog::Build` turns the decoded SMAC component, ability,
technology and `#UNITS` tables into owned positional design records. The installed
fourteen rows are calculated from their components. In particular, the Colony
Pod, Formers and Scout Patrol component costs are 3, 2 and 1; these values are not
substituted constants. The opt-in production probe provides a caller before live
gameplay integration.

This is a prerequisite for starting designs and founding production. It does not
create backend `Def` instances, place units, select a base's production, grant
technology, check build eligibility, add a prototype surcharge, or implement
workshop, transport or ability gameplay. SMAX and faction-generated designs are
outside this producer.

## Inputs, ownership and identity

The producer accepts explicit SMAC `UnitTable`, `ChassisTable`, `WeaponTable`,
`DefenseTable`, `ReactorTable`, `AbilityTable` and `SmacTechnologyRules` values.
It reuses `UnitDesignResolver` for exact component keys and whole-catalog key
validation. Returned records retain the source row index and indices into the
returned, copied component tables. The original rows, including duplicate display
names, raw ability spelling and prerequisite tokens, remain owned by the result.
Copying, moving or destroying the caller's inputs cannot invalidate the result.

The bounded source layout has nine chassis, 21 weapons/packages, ten defenses,
four reactors, 24 ability rows and at most 64 predefined rows. The positional
identities come from those source tables, not display-name classifications.
Reactor rating 1 is the executed predefined loader's explicit builder argument;
it is not inferred from a missing unit field or a reactor's cosmetic name/power.
Reactor source metadata is retained separately. These are ingestion limits,
not claims about every malformed or modified input accepted by the original.

Named prerequisites use the existing frozen technology resolver. `None`,
`Disable`, a known technology ID and `Unknown` remain distinct. Unknown does not
become disabled or acquire an availability default. Component and ability
prerequisite metadata is retained without applying workshop eligibility checks.

## Calculation and override order

The calculation uses the source cost factors rather than combat strengths or
movement values. The exact helpers used by `Build` are also exposed for direct
queries with explicit arguments:

- `CalculateCost` implements ordered ability contributions, the separate land
  flag, land/sea/air adjustments, reactor denominator, integer rounding and the
  admitted missile shortcut. It returns the full component cost.
- `CalculateCapacity` multiplies sea cargo by reactor, applies Slow Unit
  (ability index 23), then Heavy Transport (index 19). It has no weapon argument.
  `Build` invokes it only for Transport package mode 7; other packages compute
  carry zero even when their final plan or carry override describes transport.

Unit ability-table index `i` maps to mask bit `1 << i`; the rightmost source
digit is index zero. The selected-unit mask and an ability row's rule flags are
separate fields. Rule flag bit 10 adds one point to the land cost accumulator
after ordered ability stacking. No other ability effect is implemented here.

Each design calculates cost and carry, stores their low eight bits, and obtains
its admitted automatic plan before row overrides. A nonnegative explicit plan
replaces that plan; a nonzero cost or carry replaces the corresponding cached
value. The icon is also stored as its low byte. Thus a nonzero override of 256
stores zero, whereas a zero cost/carry input retains the calculation. Negative
cost/carry/icon values use the same low-byte operation. Newly produced records
are active, unprototyped and have an empty faction exclusion mask.

The typed producer accepts a representable `UnitPlan(256)` to test the original
writer's low-byte boundary. The existing text parser still restricts plans to
its documented -1..12 ingestion range. This change does not widen that parser.

## Bounded automatic-plan and arithmetic domains

Automatic plans are admitted only for the component-index/semantic-field/ability
instances executed by `RE-SMAC-PREDEFINED-001` and
`RE-SMAC-PREDEFINED-ABILITY-001`: the fourteen installed rows, the Transport Foil
single-bit and Slow+Heavy variations, and the separate Foil/Gun Slow+Heavy control.
The producer verifies the involved chassis speed/triad/missile state, weapon
mode/offense and defense rating. Cosmetic names do not select a role. Altered or
unproved instances reject even when a row supplies an explicit final plan,
because the original still invokes the builder first. This is not a general
automatic-plan classifier or a custom Morgan/Spartan design producer.

Cost helpers admit positive signed32 component factors, reactor ratings 1..4,
24-bit masks, 24 explicit SMAC ability rows, rule flags up to eleven binary
digits, and ability cost codes -7 or greater. They widen arithmetic before
checking the supported nonnegative signed32 intermediates; overflow or
unsupported domains throw rather than silently wrap. Capacity admits source
cargo 0..255. It returns a full integer (for example, 1530 for sea cargo 255,
reactor 4 and Heavy); byte caching happens only in `Build`.

All result construction is local. A late unsupported row or arithmetic failure
returns no partial catalog. This is a fresh owned value, not a mutation of an
existing game's design block, and it makes no dirty-record replacement or
transactional runtime publication claim.

## Original evidence and executable comparison

The source contracts were read from the user-owned research handoffs
`RE-SMAC-UNIT-COST-001`, `RE-SMAC-PREDEFINED-001` and
`RE-SMAC-PREDEFINED-ABILITY-001`. No original executable, installed rules, oracle
implementation, report or game asset is committed with this change.

`tools/reference/compare_smac_predefined_units.py` invokes the actual Release C++
probe, schedules inputs from the public cases, and compares only literal executed
outputs from the retained reports. It never imports or runs the research oracle.
It verifies the source, executable, fixture and report fingerprints before use.
The comparison covers:

- 140 final records from ten complete fourteen-row loads, plus the 30 captured
  basic-design records before overrides;
- 406 final records and all 406 records before overrides from 29 complete
  ability/capacity loader cases;
- 254 direct original helper results: individual ability costs, land-flag controls
  and capacities across all chassis, reactors and both capacity flags.

The earlier report's 60 cost/eligibility consumers and 20 founding scans are
excluded. The larger cost contract's 9,005 scenarios are context, not claimed
executed comparison coverage for this implementation. Active/unprototyped initial
state is compared against captured flags equal to 1; the comparator does not
infer a general prototype-bit encoding from that initial value.

Pinned SHA-256 identities:

| Input | SHA-256 |
| --- | --- |
| Original SMAC executable | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| Installed `alpha.txt` | `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4` |
| Basic predefined report | `1cf777e554bcdb7bb67fa51c63111764d0771ebbc5cabe9e0daeadd51518fc38` |
| Ability/capacity report | `f7e7ba540baecdd4c0affdc4e9ebdea99442e04482ad711d6b0b602396e42235` |

The private reports are retained under the primary checkout's `tmp/reference/`
as `predefined-unit-loading-original-x86-results-20261016.json` and
`predefined-abilities-capacity-original-x86-results-20261017.json`. The comparison
receipt and negative controls are retained outside the checkout under
`/home/jhurliman/src/glsmac-unit-weapon-parser/predefined-*`.

## Reproduction

Build the optional probe from repository root with a C++17 compiler:

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -Isrc \
  src/game/backend/unit/SmacPredefinedUnitCatalog.cpp \
  src/game/backend/settings/SmacTechnologyRules.cpp \
  src/loader/txt/UnitDesignResolver.cpp src/loader/txt/UnitRulesTXTLoader.cpp \
  src/loader/txt/SmacRulesTXTLoader.cpp src/loader/txt/TXTLoader.cpp \
  src/util/String.cpp tools/tests/smac_predefined_unit_probe.cpp \
  -o /tmp/smac-predefined-unit-probe
python3 tools/reference/compare_smac_predefined_units.py \
  --probe /tmp/smac-predefined-unit-probe --alpha "$ALPHA" \
  --basic-report "$BASIC_REPORT" --ability-report "$ABILITY_REPORT" \
  --basic-fixtures "$BASIC_CASES" --ability-fixtures "$ABILITY_CASES" \
  --output /tmp/predefined-comparison.json
```

The probe accepts `catalog`, `cost` or `capacity` followed by the explicit alpha
path. Standard input supplies bounded mutations or helper arguments; standard
output contains staged numeric JSON only after all input is accepted. The
comparator shows the stream format. It reads installed data only on explicit
invocation and is not part of default CI.

Six native groups cover owned source lifetime and copy/move, all fourteen
synthetic positional rows, computed versus overridden low bytes, ordered ability
costs, capacity/package distinctions, Unknown prerequisites, and unsupported
identity/domain rejection. No assets are required by the native tests.

## Validation receipt

On the source branch based on `060b4ca1405664e18d64655dfbcfd1baeaeebf22`, Linux
Debug CTest passed 8/8 in 49.46 seconds. The non-PIE Debug AddressSanitizer,
UndefinedBehaviorSanitizer and LeakSanitizer configuration passed 8/8 in 69.92
seconds with `ASAN_OPTIONS=detect_leaks=1`. The six focused native groups also
passed with `--memorydebug` in both builds (1.27/1.31 seconds). Explicit production
TU correctness analysis passed with zero failures in 1.72 seconds.

The Release probe passed all comparisons listed above. Its comparison receipt
has SHA-256
`c8dc792aef9c7a83d78c484aeddf1f9278d911430427abc9ca013b74fd838581`.
Three separately compiled copied-production mutations each failed the intended
original-output assertion: omitting row cost overrides, reading the wrong land
surcharge bit, and applying Heavy before Slow. Eleven additional Release
admission/stream checks passed, including capacity 1530, out-of-range reactor,
ability mask/cost and cargo rejection, unknown references, invalid binary fields,
and trailing-input rejection with no partial output. The control receipt has
SHA-256 `555c9d8083a0225c3221be99f0f4b20707062ca555dd0092d960388845526b13`.

The first expanded fourteen-row native fixture accidentally supplied zero for
its synthetic Air Superiority cost while expecting the one-point surcharge.
Both initial full runs failed that assertion; those logs are retained as
`predefined-fixture-before-{ctest,asan-ctest}.log`. Correcting that test input,
without changing production code, produced the final passes above. The installed
rules comparison passed before and after this fixture correction.

These are local source-branch checks, not a claim about a future publication
branch, Windows/macOS validation, full-game reproduction or live unit creation.

The focused publication base `707c86bf6439671cd7091dd9beca901e78c1be4f` preserves
the exact existing technology (`714bb937`), component-resolution (`78dc8f97`),
and ability-parser (`ddda8e3e`) histories. It includes the existing timer
cancellation fix through the component stack. On publication code commit
`1c29b7f521e24e89325b7f274525000505a58870`, all five tests available in that
ancestry passed in 27.85 seconds (GSE 21.43 seconds, including all six new native
groups), and explicit production TU analysis passed with zero failures in 1.76
seconds. The actual publication Release probe repeated every original comparison
above; its report SHA-256 is
`237b0879ec4ca35f5d10c1ce2494d6f9228929a2c98305be12baf7f1cc885853`.
Only registration placement differs from the source implementation; no new
production or test assertion was changed for publication. The earlier full
sanitizer/tracking receipt belongs to the source ancestry, which includes more
runtime fixes and test executables than this deliberately smaller dependency
stack.
