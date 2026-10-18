# RE-SMAC-BASE-GROWTH-DISPLAY-001: nutrient box and growth label

This identifies the original Base screen's nutrient-panel display producer,
separate from [population upkeep](growth-and-starvation.md). **24 native label
controls and 24 following population-step controls** execute, with22 native
box/fill/overlay fragments at declared test geometry. Two extra redraw controls
change stored food or cached surplus without upkeep. [Numeric cases](base-nutrient-growth-display-cases.json).

This is stock SMAC evidence. Original arithmetic, nutrient-cost helper, integer
conversion and string assembly execute. Localized string lookup and drawing
services are supplied. Complete window construction, input dispatch and pixel
rendering are not claimed.

## Capacity and label algorithm

Let F be the current signed stored nutrients and S the **cached nutrient
surplus**. Let C be the actual nutrient-mode cost helper for the owner with
this Base context. The current box capacity is `T = C * (population + 1)`.
Use the [nutrient cost contract](nutrient-cost-factors.md), including rules,
world category, effective GROWTH, Creche and Golden Age. Do not round a combined
population product in place of the helper's rounding.

The label producer reads these inputs again when it draws. Its exact ordinary
display branches are:

| Condition | Installed English label |
| --- | --- |
| F<T and S>0 | `Growth: N turns`, where `N = ceil((T-F)/S) + 1` |
| F<T and S=0 | `Growth: -stagnant-` |
| F<T and S<0 | `Hunger!`, with **no displayed turn count** |
| F>=T and S>=0 | `Growth: 1 turn` |
| F>=T and S<0 | `Hunger: 1 turn` |

The numeric formatting path clamps its integer to0..9999, using singular
`turn` only for exactly1. For ordinary positive S and T−F, the ceiling is the
integer operation `(T-F+S-1)/S`, then add1. The extra1 is real and matches the
population routine's threshold-before-collection order. Do not remove it as an
off-by-one correction. Negative-surplus code computes an internal buffer
division, but below capacity that number is not appended to the label.

Examples with population1, neutral GROWTH, Standard category and default
human nutrient cost give T20:

| Food / surplus | Label | Next population-step result |
| --- | --- | --- |
| 0 / +2 | Growth: 11 turns | Still size1; food2 |
| 18 / +2 | Growth: 2 turns | Still size1; food20 |
| 19 / +1 | Growth: 2 turns | Still size1; food20 |
| 20 / +2 | Growth: 1 turn | Size2 |
| 5 / 0 | Growth: -stagnant- | No growth; food5 |
| 20 / 0 | Growth: 1 turn | Size2 |
| 5 / −2 | Hunger! | Still size1; food3 |
| 0 / −2 | Hunger! | Normalization makes food−1; starvation requests Base removal |

The comparisons execute the actual population routine. After a surviving
population change its resource solver receives explicit resolved surplus+2;
that supplied solver boundary is the same limitation as the earlier population
receipt. These cases establish the selected population transition, not a full
world/worker allocation outcome for the enlarged Base. Removal is checked to
its request, not complete deletion cleanup.

## Housing, stagnation and boom are not forecast overrides

The display producer does **not** run housing admission, apply the full-box
GROWTH<=−3 stagnation refusal, or replace its count with boom timing. Those
conditions affect the actual population step separately:

- Size7 without Hab Complex, food80/surplus2: `Growth: 1 turn`, but the actual
  step stays size7 and leaves food42. Adding Complex permits size8, with the
  same pre-step label.
- GROWTH−3, size1, capacity24, full food24/surplus2: `Growth: 1 turn`, but the
  population step remains size1 and caps food24.
- GROWTH+6, size1, capacity10, food0/surplus2: `Growth: 6 turns`, yet the next
  population step booms to size2 without collecting into the box.
- Cloning Vats with GROWTH−3 and food0/surplus2: `Growth: 13 turns`, yet the
  next step booms. Creche plus Golden Age affects capacity through the cost
  helper but does not add a separate boom override to the label.

In particular, the word `stagnant` here describes **zero surplus below capacity**;
it is not a direct test of the social GROWTH stagnation threshold. The original
estimate can be misleading about when population will actually change. A port
may separately expose outcome warnings, but must not label an improved simulator
forecast as the exact original count.

## Nutrient-box contents and surplus overlay

The box drawing stage first reads the stored amount and cached surplus and
uses the same cost helper/capacity. In the checked uncompressed grids:

- A cell index i is filled when `i < F`; other capacity cells are empty.
- With positive surplus, highlight prospective additions satisfying
  `F <= i < F+S`.
- With negative surplus, mark prospective loss satisfying `F+S <= i < F`.
- Marks are limited to drawn cells. Full/overfull food fills the tested box;
  it does not expand capacity. Negative food fills none.

This overlay is nutrient bookkeeping, not a population-event preview. A full
housing-blocked or stagnant Base can still show a full box; a boom can still
show the ordinary next-food overlay. The22 controls check native cell and mark
selection at supplied300×10 geometry, not original window metrics, artwork,
palette fidelity or every compressed/rounded grid layout. Huge-capacity and
zero-cost cases check the label only; a complete zero-cost box renderer is not
admitted by these controls.

## Recalculation and display lifetime

The turns value is a **draw-time local**, not a saved forecast or turn counter.
Each nutrient-panel draw calls the nutrient-cost helper for the box, then calls
it again while constructing the label. It rereads stored food and cached S and
rebuilds transient text. Two repeated label draws match, leave the entire Base
record unchanged and draw no gameplay RNG. Changing only cached S2→0 changes
the next label from11 turns to `-stagnant-`; changing only food0→20 changes it
to1 turn. These are direct redraw-input controls, not user command admissions.

The enclosing Base-screen draw invokes the nutrient panel during the active
Base-view draw path. It does not run population upkeep. Resource calculation
and rendering remain distinct:

- The already native-checked [visible worker handler](manual-worker-handler.md)
  selects/recalculates before admission, edits allocation, then forces the full
  resource pipeline before its virtual redraw. Worker→Doctor can change cached
  surplus2→0, changing the subsequent label from11 turns to `-stagnant-` at food0.
- The specialist-selection commit tail likewise checks visibility, selects its
  viewed Base, forces recalculation, then invokes the same virtual redraw.
  Existing specialist tests stop before that tail; this tail-to-draw ordering
  is established statically here, not a new continuous rendered menu execution.
- Hidden view/parent diagnostic worker calls skip that forced recalculation and
  redraw. They can retain old cached resource outputs after allocation changes;
  they do not establish normal hidden-view player input.
- Opening/selecting/refreshing a Base has the existing
  [resource-cache rules](base-recalculation-and-cache.md). A cached zero request
  can skip nutrient normalization. The label itself neither normalizes food
  nor invalidates resources: it uses the current stored/cached inputs.

Successful resource recalculation can change food's sign through normalization,
even though it does not collect upkeep nutrients. Do not implement an inspector
refresh by running the population step, and do not assume every resource request
actually recomputes the cache. The original label's read-only construction and
its callers' possible recalculation effects are separate contracts.

Remaining limits: complete window/message-loop repaint scheduling, end-to-end
specialist dialog rendering, live pixel timing, AI-window admission, compressed
box geometry, extreme overflow and full post-growth world resolution. No SMAX
or PRACX behavior is inferred; no port UI changes are included.
