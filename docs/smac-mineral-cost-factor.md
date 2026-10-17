# SMAC mineral cost factor, arithmetic contract v1

`CalculateMineralCostFactor` is a pure calculation of the mineral cost factor.
It accepts explicit resolved inputs and returns an integer before an item's
rules cost is applied. It does not read the map, faction rankings, facilities,
network state, rules files or a mutable game object. It is not connected to
production scheduling, construction legality or unit/prototype costs.

## Supported formula

A human starts at 10. AI starts at the global difficulty's factor:

| Global difficulty | 0 | 1 | 2 | 3 | 4 | 5 |
| --- | --- | --- | --- | --- | --- | --- |
| AI starting factor | 13 | 12 | 11 | 10 | 8 | 7 |

For AI, subtract one when the leader is human and network-session mode is false.
Separately subtract one when the supplied leading-human dominance result is
true. Network mode suppresses only the first deduction. The human calculation
ignores these adaptive inputs. Dominance is an already normalized result of the
leader's mode-zero query, not a ranking or victory-state implementation here.

Multiply by the nonnegative mineral rules multiplier and truncate division by
10. Then, for Tiny multiply by 8 and truncate division by 10; for Small multiply
by 9 and truncate division by 10. Standard and larger preserve that result.
These resolved categories must be supplied; map dimensions do not determine
them in this API. The installed multiplier is 10, but there is no implicit
default or nutrient-multiplier substitution.

Let `S` be the resulting nonnegative integer. Apply INDUSTRY last:

| INDUSTRY | Factor |
| --- | --- |
| -8 or lower | `(S + 1) / 2`, truncated |
| -7 through -3 | `ceil(13*S / 10)` |
| -2 | `ceil(6*S / 5)` |
| -1 | `ceil(11*S / 10)` |
| 0 | `S` |
| +1 | `ceil(9*S / 10)` |
| +2 | `ceil(4*S / 5)` |
| +3 | `ceil(7*S / 10)` |
| +4 | `ceil(3*S / 5)` |
| +5 or higher | `(S + 1) / 2`, truncated |

The low out-of-range branch is intentional. Human, Standard, multiplier 10 gives
13 at INDUSTRY -7 and 5 at -8. There is no minimum-one floor: zero and early
truncation to zero remain zero. Growth, Creche, Golden Age and base-context inputs
are absent because the mineral branch does not use them. Round the factor before
any future item-cost multiplication; combining those stages changes results.

## API rejection policy

All fields are required. Difficulty must be 0..5 and the resolved size enum must
be recognized, including on the human path. This is API metadata validation;
it does not claim the original human path reads difficulty or queries dominance.
Negative multipliers are rejected. Every signed32 INDUSTRY value is accepted,
and range selection avoids overflowing an `INDUSTRY + 7` expression.

Arithmetic uses signed64 temporaries but admits only nonnegative signed32
selected products and numerators. It checks the rules product before division,
the world-size product before division, and the selected INDUSTRY numerator:
`13*S+9`, `6*S+4`, `11*S+9`, `9*S+9`, `4*S+4`, `7*S+9`, `3*S+4` or `S+1`.
The +3 path also checks its intermediate `8*S`. This follows the selected
optimized arithmetic rather than rejecting a reduced fraction because a
different, unreduced expression would overflow. For example, `S=214748364`
at INDUSTRY -2 is supported through `(6*S+4)/5`.

Invalid metadata throws `std::invalid_argument`; an unsupported arithmetic
intermediate throws `std::overflow_error`. These are explicit port domain
restrictions, separate from the faithful formula on the supported domain.
Original signed overflow/wrap and negative multipliers have not been established.

## Evidence and reproducibility

Research identifier: **RE-SMAC-MINERAL-COST-001**. The retained evidence executes
the original SMAC cost routine at `0x4f9d60` in mineral mode 1. It contains 1,589
arithmetic rows, 216 original dominance integrations with human/AI factor outputs,
and 864 admitted production comparisons, all passing in the retained report.
Arithmetic rows supply the dominance result; integration rows execute the
original dominance routine. These are controlled original-x86 checks, not a
full-game observation or a claim of SMAX equivalence.

| Artifact | SHA-256 |
| --- | --- |
| SMAC `terran.exe` | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| Installed `alpha.txt` | `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4` |
| `mineral-cost-factors-cases.json` | `7da4f8ac592d5878de967ae51e19c9397ea1dd42046ba304064acdee3188fcd0` |
| `mineral-cost-original-x86-results-20261016.json` | `db57119677ba60f36d189f1e5c01fd13abd635237b3723cf23d78775707755b2` |

The committed synthetic fixture retains the 29 named factors and 432 factor
expectations from the 216 original integration records. Port tests supply the
recorded dominance result; they do not recreate or validate those world/ranking
states. Additional tests cover metadata, independent deductions, separate
rounding, zero factors, discontinuities and selected integer boundaries.

The optional comparison tool links the actual C++ calculator and compares its
outputs against all 2,021 retained original factor outputs. Its hash-pinned v1
grid description reconstructs the arithmetic rows' inputs; no original routine
is run by this tool. The 864 downstream production comparisons are deliberately
outside this factor-only comparison. No evidence harness footer is run and no
human-owned research output is modified.

```sh
c++ -std=c++17 -O2 -DNDEBUG -Isrc \
  src/game/backend/resource/SmacMineralCostFactor.cpp \
  tools/tests/smac_mineral_cost_probe.cpp -o /tmp/smac-mineral-cost-probe
python3 tools/reference/compare_smac_mineral_cost.py \
  --probe /tmp/smac-mineral-cost-probe \
  --fixtures /path/to/mineral-cost-factors-cases.json \
  --report /path/to/mineral-cost-original-x86-results-20261016.json
```

Ordinary CI needs none of those private files: the native cases are included in
the usual `GLSMAC --gse-tests --quiet` suite. This main-based slice does not
require the later native-filter CLI or any gameplay kernel dependencies.

## Local validation

On this focused main-based branch, Clang 15.0.7 Debug full CTest passes 4/4 in
17.24 seconds (the complete GSE suite takes 14.09 seconds). Its six added native
groups cover the committed semantic fixtures and the input/arithmetic boundaries.
Curated LLVM 15 analysis reports zero failures for the production translation
unit in Debug (0.25 seconds) and Release (0.26 seconds).

The opt-in production probe matches all 2,021 retained original factor outputs
in Release and in a separate ASan/UBSan/LSan build. Another 29 probe cases check
invalid metadata, zero factors, extreme INDUSTRY and arithmetic boundaries in
both builds. Leak detection and halt-on-error are enabled; the existing local
non-PIE sanitizer workaround is confined to the standalone probe. This is
sanitizer coverage of the calculator/probe, not a full-game sanitizer claim.

Three separately compiled copies of the production source provide negative
controls: combining staged rounding yields 9 mismatches, clamping low INDUSTRY
yields 316, and suppressing dominance in network mode yields 46. Each comparator
exits 1. The committed calculator and human-owned evidence remain unchanged.
No native Windows/macOS execution or live construction-cost validation is claimed.