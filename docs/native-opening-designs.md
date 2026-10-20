# Original opening designs and Scout selection

`GenerateSmacOpeningDesigns` composes original capability refresh and fresh
prototype construction for the stock named-starting-technology opening profile.
It clears the first-new-design global before the capability pass, then requests
an available Synthmetal defender before an available Speeder Recon. Availability
comes from component prerequisites and canonical ownership, not faction identity.
The first allocated custom slot remains the first-new-design value.

`ConstructSmacOpeningDesign` implements the fresh `597390` constructor for those
two stock component combinations and the basic Scout2 shortcut. It preserves
unused byte47 and prerequisite word50, writes native intermediate classifications,
reactor/cost/plan, resets exclusion/icon, and applies difficulty and allocation
flags in original order. A just-created defender can precede a Recon in the next
slot. Arbitrary existing custom designs, advanced components, replacement and
workshop state are excluded from this API.

`MarkAndSelectSmacStartingScout` implements the caller loop separately: every active
custom design receives flags OR4, including an already-prototyped design. The last
active plan3 Recon wins; otherwise predefined Scout2 remains selected. The first
new design is not necessarily the selected Scout.

Inputs are caller-owned staged records and the owned stock catalog/prerequisites.
The admitted profile excludes acquired advanced technologies, own Bases/Units,
contacts, PSI bonuses/projects and existing custom designs. Automatic names come
from a required caller callback when allocation occurs, as already localized
single-byte text. The original name producer and complete installed predefined
catalog initialization remain integration work; no placeholder name is silently
chosen by production. Numeric trace comparisons exclude text-copy stores but
include final name bytes. Failed staged updates must not be published.

The [installed design producer](native-installed-designs.md) now supersedes the
supplied-name/basic-three admission for the stock early startup caller. It loads
all fourteen records, shares the Fission component builder, supplies actual
localized names and derives Base production costs from initialized records.
The earlier controls below remain useful for poisoned unused custom fields and
isolated constructor/selection behavior.

## Original evidence and validation

Both frozen bundles were fully verified before numeric fixture import:

- `independent-startup-fresh-design-evidence-20261020`: 197 files, manifest
  `9012dd9c23863510c5c88f8a7d056dd4f2611555e68a6ab086d7bcb1a31a099a`;
  36 complete 512-design controls and 480 ordered numeric design stores.
- `independent-startup-early-designs-evidence-20261020`: 202 files, manifest
  `128260d7e184474156e253ba8a34910a0d24227048826f001593151515a2ab1c`;
  102 complete design/eight-faction controls, 5,268 ordered numeric stores and
  36 separate caller prototype stores, plus first-design and Scout results.

Original executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The native probes supply declared name text only; gameplay helper results are
not replaced. Controls cover owners1/2/7, difficulty0/1/3/5, all seven named grant
sets, no grants, combined grants and different personalities. There are no RNG
draws. Purchased art and full original capture buffers stay outside Git.

The generated zero-storage Gaia composition now runs this producer and caller
selection rather than directly refreshing capabilities and hardcoding Scout2.
Its declared basic caller design pool remains unchanged, first-new-design is -1,
and the resulting first-faction/map/ownership/RNG boundaries still match.
Subsequent Base/territory/Scout checks remain projections until full resource
and seven-faction controller composition is implemented.

Cached incremental build and five selected native groups pass, including the
last-active-Recon control and generated composition. Correctness analysis passes
for both affected production TUs. A focused four-TU ASan/UBSan run passes the same
groups; other objects are uninstrumented, with leak detection disabled. No new
runtime screenshot or full platform matrix is claimed for this component.
