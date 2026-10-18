# Base eco-damage calculation

`base/SmacEcology` implements the complete measured calculation from resolved
radius improvements and ecology inputs to final Base damage. It also exposes
the research-aggregate contribution derived from that final value.

The radius scan reuses the existing doubled-X21-square resolver. It queries all
valid squares, including unworked, foreign, unexplored and sea squares. The
center's worked multiplier is explicit. The nine counted improvements, three
extra weights, forest subtraction and per-square Tree Farm/Hybrid Forest
rounding follow the [terraforming receipt](terraforming-ecology-input.md).
Semantic improvement bits require translation from authenticated live terrain;
they are not the native packed map word or GLSMAC's terrain flags.

The next stage floors raw points divided by eight, consumes the retained clean
allowance, applies the capped mining-satellite deduction and additive facility/
project divisor, and preserves negative intermediate burden. It consumes
ordinary gross minerals before support, outgoing convoys and late Space Elevator
doubling. See [clean-mineral burden](clean-minerals-and-ecology-burden.md).

Final scaling retains the human atrocity/climate term, nonnegative clamp,
Voice/Transcendence and perihelion doublings, human/AI difficulty choice,
stored technology acquisition score minus future counter, native-life setting,
PLANET clamp and ordered rounding. Final damage has no artificial100 cap.
[The original scaling receipt](eco-damage-scaling.md) supplies the inputs and
literal intermediate values. Evaluated signed32 overflow outside the measured
domain throws instead of relying on undefined C++ arithmetic.

Validation uses original-executable literal observations:35 complete radius
contexts,16 clean-burden arithmetic cases with every recorded intermediate,
34 final-scaling cases and nine final-damage/aggregate projections. Three
overflow checks exercise the checked arithmetic boundaries. Five focused native
groups and targeted correctness analysis pass on the cached incremental build.
The clean-mineral receipt's other21 completion/bloom-history cases and scaling
receipt's separate acquisition transitions are not claimed as tests of this
calculation. No full platform or sanitizer matrix was repeated for this pure
arithmetic change.

Live integration remains required. The caller must supply retained clean-mineral
and atrocity history, actual cumulative sea-level and perihelion authorities,
current facilities/projects and actual worked selections. Their
[input lifetimes](ecology-input-lifetimes.md) distinguish fresh initialization,
nonzero faction initialization and saved history. Resource previews must not
modify the research aggregate or draw RNG. The actual owner pass must clear only
its aggregate, run each Base's research before adding that Base's contribution,
then perform the ecology CRT draw even at zero damage, as established by
[the complete two-Base caller receipt](base-ecology-research-aggregate.md).
This change does not yet publish live damage, implement ecological events or
complete automatic research. All supplied evidence concerns stock SMAC;
SMAX parity remains unverified.
