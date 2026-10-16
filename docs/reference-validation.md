# Original-game reference validation

Faithfulness is evaluated against identified original installations and
repeatable observations, separately for SMAC and SMAX. A remembered rule, a
placeholder value in GLSMAC, or an unlabelled third-party patch is not ground
truth.

## Reference available on aleph0

Read-only inventory on 2026-10-15 found:

- GOG installation: `C:\Program Files (x86)\GOG Galaxy\Games\Alpha Centauri`.
- Registry label: Sid Meier's Alpha Centauri Planetary Pack; installed version
  `1.1 PRACX_ddraw`, GOG build `58038807730658275`, English.
- Both `terran.exe` (SMAC) and `terranx.exe` (SMAX), and separate
  `terran_PRACX.exe` / `terranx_PRACX.exe` variants.
- `alpha.txt`, `alphax.txt`, manuals, patch readmes and a DDraw compatibility
  layer. Record their actual hashes with the inventory tool.

The installation inventory alone does not establish successful execution.
On 2026-10-15, SMAX PRACX reached the main menu and initial base-naming dialog,
but crashed after accepting the name, including when launched through its
installed shortcut. The user reports that shortcut has reached gameplay before.
The plain SMAX executable crashed at startup in this attempt. See the
[startup observation](reference-cases/SMAX-START-001.md) for exact conditions.
These results do not establish general Windows 11 incompatibility or behavior
of the separate SMAC executable. Preserve the installation while investigating.

## Capture an installation manifest

From the repository root with Python 3.9 or newer:

```powershell
python tools/reference_manifest.py --help
python tools/reference_manifest.py "C:\Program Files (x86)\GOG Galaxy\Games\Alpha Centauri" --output tmp/reference/aleph0-gog-planetary-pack.json
python -B -m unittest discover -s tools/tests -p test_reference_manifest.py -v
```

Use the tool's explicit installation path and output arguments to write local
metadata under `tmp/reference/`. The manifest contains file identities and
hashes, not copied assets. Keep actual saves, screenshots, manuals, recordings
and executables in that ignored local evidence directory or an approved private
artifact store. Keep only self-authored test fixtures, measured expectations,
provenance metadata and scenario instructions in the repository.

Capture manifests again after updates. Never overwrite a prior observation's
identity with new hashes. The tool refuses existing output files by default;
choose a new capture filename. Use `--overwrite` only to explicitly replace a
disposable manifest that no evidence references. New manifests are published
atomically using a hard link within the output directory, which requires a
filesystem supporting hard links (such as NTFS, APFS or ext4).
Rules and executable fingerprints matter separately:
the same assets can accompany different gameplay patches. PRACX/DirectDraw
compatibility indicators are useful provenance, not proof of behavioral parity.

## Evidence hierarchy and conflicts

1. Installed rules/data files identify content, parameters and prerequisites.
2. Original manuals and bundled patch notes explain intended rules and changes.
3. Controlled observations of a named original executable establish actual
   behavior, rounding, ordering and boundary cases.
4. Third-party documentation can suggest experiments but does not replace the
   pinned reference. Do not import patches' changed rules as original behavior.

When these disagree, open a fidelity discrepancy with both expected outcomes
and the exact experiment. Decide whether to reproduce the behavior, correct a
defect or provide an explicit compatibility option. Do not silently choose the
most convenient implementation. Original code disassembly is not part of the
reference workflow; follow the project's from-scratch implementation approach.

## Case format

Each rule family gets stable case IDs and small independent scenarios. Store
self-authored case metadata alongside the eventual backend fixtures. Required
fields:

| Field | Purpose |
| --- | --- |
| Case ID and status | `planned`, `observed`, `implemented`, `verified`, or `discrepancy`; status requires evidence |
| Ruleset | SMAC or SMAX, never inferred from the installation folder name |
| Reference identity | Manifest fingerprint, executable filename/hash, active patches and configuration |
| Setup | Faction, difficulty, options, map/scenario identity, turn, social settings, technologies, facilities, units and relevant tile properties |
| Action | Exact reproducible input sequence, including turn boundary when relevant |
| Before/after | Measured semantic values; screenshots or private save references support them |
| Expected result | Values, rounding, order and invariants, with the source/observation cited |
| GLSMAC coverage | Test path and commit, actual result, supported platforms and unresolved differences |

Separate deterministic rule expectations from random outcomes. Capture a
repeatable controlled setup or distributions for chance-based mechanics; do
not assert matching PRNG streams between unrelated implementations. GLSMAC
replay/save consistency, however, must preserve its own RNG state exactly.

## First observation queue

| Case family | Experiments |
| --- | --- |
| Base yields | Headquarters and non-HQ bases; nutrient/mineral/energy bonuses; recycling tanks; faction/terrain combinations |
| Workers and growth | Base square vs worked tiles; specialists; tile reassignment; starvation; growth box size and overflow at difficulty/social settings |
| Production/support | Scout/former/pod costs, mineral carryover and switching, support at small population, facility maintenance and hurry costs |
| Research | Starting faction technologies, research cost, prerequisite availability and discovery/production ordering |
| Founding | Legal land/sea sites, pod consumption, base size, headquarters assignment and faction exceptions |
| Terraforming | Prerequisites, farm/mine/solar/road time, cancellation, work stacking, yield caps and bonuses |
| Movement/combat | Terrain/roads/rivers/ZOC, fractional movement, conventional and psi combat, morale/reactor/terrain modifiers, retreat and capture |
| Victory | Remaining bases/units/pods, elimination, surrender/allies and enabled victory settings |

Start with the minimal conquest scenario in [the release plan](faithful-1.0.md).
Expand the inventory to every rule family before declaring SMAC or SMAX
complete. Data-file content counts measure inventory coverage only, not gameplay
correctness.

## Validation layers

- GSE interpreter and focused rule tests run without original assets. Fixtures
  exercise the real script handlers; mock-only results are labelled as such.
- Backend scenario tests exercise real entities, validated events, turn order,
  snapshots and replay without UI timing. Add these before persistence changes.
- Integration runs verify assets, rendering, input, animation, sound, actual
  save/load and multiple clients. They require the local purchased installation.
- Paired original/GLSMAC experiments verify fidelity; an engine unit test alone
  cannot establish it. Keep observed facts separate from proposed rules.
- Performance and longevity runs use named hardware, graphics drivers, build
  type, seed/options and a recorded workload. Capture baseline and changed
  results using the same method.

## Base snapshot compatibility

The base-manager section of a world snapshot starts with the typed string
`GLSMAC_BASES_V2`. It contains population definitions, facility definitions, base
instances, and the next base ID, in that order. Definitions must be restored
before a base can resolve its population and facility IDs. Each base record now
stores accumulated nutrients as a nonnegative signed 64-bit integer, initially
zero. The script getter reads this authoritative value; its setter requires a
game write scope and rejects negative or non-integer values. Population events,
rollback, turn processing, and the base screen use this same field. Growth and
starvation calculations are unchanged by this storage migration.

Old unversioned and V1 base snapshots are unsupported. The unversioned integer
population count fails the new reader's string-type check; the new string marker likewise
fails an old reader's integer-type check. Unknown version markers are rejected
before changing base-manager state. Mixed-version peers therefore cannot join
using these incompatible formats. This section marker does not establish a
stable save-game contract or implement top-level protocol negotiation.

The native backend tests run without original assets and exercise facility
metadata and base references through real managers and entities, plus legacy
and unknown-format rejection. Nutrient tests cover zero, a nonzero balance, and
the signed 64-bit maximum through the real script setter and snapshot codec;
invalid writes preserve the nutrient balance, and negative base records leave
tile ownership and the next base ID unchanged. This does not establish
transactional rollback of a malformed manager snapshot. The actual
population event apply/rollback handlers also run against a real base, with
only their game initialization services substituted. Collection-count limits,
malformed snapshot hardening, complete authoritative state, and full save/reload
validation remain separate M1 work.
