# Ordinary land exploration kernel

`map::smac_exploration::Explore` consumes a supplied doubled-X grid, land
center, faction, sea classifications and exploration masks. It returns an
owned updated exploration grid and ordered coordinates where the caller must
refresh that faction's remembered improvements from current map data. It does
not supply remembered values, foreign-unit visibility, command/design admission,
or a live Game/Map hook.

The direct nine positions always refresh, even when already explored. The
sixteen outer candidates receive one ordered pass; previously explored outer
positions are skipped. Valid unexplored cardinal neighbors count two and
diagonals one. A land candidate reveals below four, a sea candidate below six;
later candidates see earlier reveals. X receives one adjustment when wrapping,
then clips; Y clips. All other exploration bits and positions are retained.

The input arrays contain one byte per valid doubled-X position, indexed by
`y * (width / 2) + x / 2`. Sea bytes must be zero or one. Exploration preserves
all eight bits; the acting faction is explicitly 1..7. The port admits even
widths 10..512 and heights 1..256, with a valid same-parity land center. These
are bounded port admission rules, not a measured generation policy. Original
captures use 64×32; widths below ten are rejected because narrow-map aliases
are outside this receipt. All shape/value checks finish before output copying;
the input remains unchanged on success or failure.

## Evidence and tests

The human-owned source is `RE-SMAC-ORDINARY-LAND-EXPLORATION-001`, documented
in `docs/behavior/ordinary-land-exploration.md` in the research checkout. The
fixture projector copies its semantic expected sets without simulating sight:

| Input | SHA-256 |
| --- | --- |
| `ordinary-land-exploration-cases.json` | `20065782ed34d701f978fd6fb9711f2a80590bfc3691e2a58fd0933992d8d68c` |
| `ordinary-land-exploration-transition-cases.json` | `757a9fa95819724d19c09c49501afcbeb8a8e37b76ebd7af1f1e710045b60a78` |
| Original factory report | `691c54d7213bc351a3a4933cec456cde2e1402fd15898cc5632109f923fd08c4` |
| Transition binding report | `bc9adc50b2859d31a97e866865ac27627c92bd00a97fbb21af66034d9b5a0259` |

The 194 fresh original factory cases cover Gaia Scout and Spartan Rover profiles,
five uniform land types, coast, boundaries and retained exploration. Uniform
land variants project to the same supplied sea classification; no terrain
occlusion is inferred. Fourteen transition expectations reuse seven earlier
continuous move→Colony-factory traces, with the first modeled result retained
as the next input. This does not execute movement or activate Colony sight in
the port.

Source refresh lists are sets sorted by coordinates, not observed callback
order. The native corpus checks the entire explored grid and refresh sets;
separate literal tests check direct and outer table order, refresh of already
explored direct positions, skipped outer memory and all-bit preservation.
The source receipt records that fixed-nine, simultaneous and fixed-point
alternatives disagree with 48, six and six placement cases respectively.
The complete corpus therefore retains those distinguishing inputs. No new
original execution or copied-production mutation run is claimed here.

Validation on this standalone main-based branch: all four native groups pass
with allocation tracking (1.116s), including all 194+14 outcomes, ownership,
17 rejection scenarios and port geometry boundaries. LLVM15 analysis of the
one new production TU reports zero failures (0.92s). These are focused Linux
checks; no full-suite, sanitizer, other-platform or live gameplay claim is made.

Initial 21-square startup revelation, increased sight, sea/air units, current
foreign visibility/loss, territory, live movement scheduling and persisted
remembered values remain separate producers. This kernel adds no default
activation, RNG effects or serialization format.
