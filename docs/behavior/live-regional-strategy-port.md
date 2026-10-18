# Live regional strategy and retained ranking storage

The owner-summary caller now stages the regional strategy-code phase after its
Base, population, design and military scans. It consumes the existing retained
territory summary, area table, concentration status, signed Base totals,
personality priorities, peer military/Base rows and directed relations. Only
the requested owner's strategy entries1–127 are replaced; entry0 and other
owners remain unchanged. A rejected preparation publishes none of the staged
owner aggregates. Actual Base-record routing uses the map's retained path
workspace and the current prototype definition, without drawing gameplay RNG.

Simulation V24 saves all eight128-byte strategy rows and eight signed score
words independently. V11–23 frames retain unavailable history rather than
inventing it during restore. Cold engine storage starts at zero. Real Reset
and the following new simulation preserve prior rows and score words, including
slot0 and unavailable legacy history. The source evidence covers the
[whole-world/fresh-child strategy prefix](new-world-regional-strategy-storage.md)
and [score prefix](new-world-retained-score-table.md), not two completed native
frontend games or the last later writer before first command.

Dominance indexes the retained score table through retained rank identities.
The global full ranking update computes its order from the pre-upkeep caches,
then retains consumed positive score slots as−1 and preserves score slot0.
Owner summary does not implicitly refresh scores. The
[continuous native ranking path](ranking-dominance-score-order.md),
[slot0 controls](dominance-runner-slot0-score.md), and
[six late-game signed sentinel comparisons](dominance-consumed-score-sentinels.md)
establish these separate lifetimes. In particular,−1/−1 can satisfy the weighted
comparison while−1/0 fails; neither word is converted to zero. The predicate's
wide products preserve these measured values; native signed overflow outside
the measured arithmetic domain remains unverified.

The live dominance adapter currently admits the existing ordinary no-project
campaign. It uses actual controller difficulty, campaign rules, human
membership, retained victory deadlines/history and current frozen council
requirements when that branch is needed. Legacy port campaigns with no option
word retain their explicit existing zero-word contract. Never-admitted slots
have no Base-count writer under the port's immutable campaign roster; an
admitted peer with missing Base history rejects. Partial diagnostic/legacy
geometry without a territory summary does not run this phase or reconstruct
its history. These boundaries must expand with their actual producers when
the campaign supports projects, other scenario modes or mutable rosters.

Validation:

- Fourteen focused native groups pass, including32 literal initial-threshold
  cases,33 policy cases,14 opening projections, real route composition, live
  owner-summary routing, retained dominance inputs, actual Save/Start, actual
  Reset/new-simulation carry, codec bounds and signed comparisons.
- The affected52-group Base/startup/capture/restart regression passes, including
  installed Planet's seven factions at six difficulties. Successful checks for
  unchanged sources were reused after the final isolated ranking correction.
- Seven changed production translation units pass targeted correctness
  analysis; the final ranking/predicate edits were analyzed again.
- Focused ASan/UBSan instruments state serialization, the new adapter, owner
  economy summary, canonical turns, new simulation, Game reset and dominance.
  Eight selected live/codec groups pass on the final build. Other objects reuse
  the normal incremental build; leak detection is disabled. This is not a full
  sanitizer or platform matrix.

This connects the [strategy decision kernel](regional-strategy-decision-port.md)
to its live producer. It supplies strategy codes, not the unrelated bilateral
flag4 or a complete final regional flag word. The remaining Blind Research
valuation/selector integration and strategic AI are not completed by this
change. The supplied receipts concern stock SMAC; SMAX parity is unverified.
