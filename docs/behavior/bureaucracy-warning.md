# RE-SMAC-BUREAU-WARN-001: pre-founding bureaucracy warning

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. Twenty-one named cases and 2,400 additional combinations execute the
original map-scale builder, complete warning-interval helper and caller
decision/response fragments. Actual dialogs, bureaucracy drone generation and
complete founding have not been executed.

## Warning interval

For nonnegative map dimensions, normal difficulty indices and intermediate
products that fit signed 32-bit arithmetic (overflow remains unverified):

1. Compute actual tile count as `floor(map_width / 2) * map_height` in the
   game's doubled-horizontal coordinate convention.
2. Set map scale to `floor(sqrt(tile_count))`.
3. Use the faction's difficulty index for a human faction. For AI, use 3
   regardless of its stored difficulty.
4. Set effective efficiency to `max(current_faction_efficiency, 0)`.
5. Compute:

`interval = floor((8 - difficulty) * (4 + effective_efficiency) * map_scale / 112)`

Keep the integer square root step. Using the world-size label or a floating
square root until the final division can change custom-map results. Negative
efficiency is treated as zero here; positive efficiency has no rating cap in
this calculation. These are warning rules, not a complete inefficiency or
citizen-drone formula.

With efficiency zero and installed map dimensions:

| World | Width × height | Human difficulty 0 | Human difficulty 5 | AI (fixed difficulty 3) |
| --- | --- | --- | --- | --- |
| Tiny | 48 × 24 | 6 | 2 | 4 |
| Small | 64 × 32 | 9 | 3 | 5 |
| Standard | 80 × 40 | 11 | 4 | 7 |
| Large | 90 × 44 | 12 | 4 | 7 |
| Huge | 128 × 64 | 18 | 6 | 11 |

Difficulty indices 0 and 5 are Citizen and Transcend. The AI column describes
the helper's arithmetic; it does not imply automated AI founding displays this
manual-command warning.

## Trigger and response

Before adding the new base, inspect the faction's **existing** base count.
Zero bases skips the warning. A positive count warns exactly when divisible
by the interval. Count equal to one interval selects the first warning;
later positive multiples select its repeated-warning variant.

For example, interval 4 warns while attempting bases 5, 9, 13 and so on,
because the existing counts are 4, 8 and 12. It does not warn at every count
above four or when the prospective count merely equals four.

The caller cancels when the warning dialog's returned result is negative;
zero or positive continues to later processing. This sign contract differs
from the territory confirmation's zero-versus-nonzero branch. The cases supply
the result explicitly and do not observe actual dialog buttons or keyboard input.

## Custom-map zero interval

There is no minimum-one clamp in the checked helper. A synthetic 8 × 8 map
with human difficulty 5 and efficiency zero returns interval zero. With zero
existing bases the caller skips division. With a positive count its original
division raises an x86 divide-by-zero exception in the emulator.

This is a supplied custom-map boundary, not an observed original-game crash
or evidence that normal setup permits that map. Do not silently describe a
minimum-one clamp as an original rule. A port's handling of unsupported custom
sizes needs an explicit policy and setup validation.

## Acceptance scope

The [21 named cases](bureaucracy-warning-cases.json) cover first/repeated and
off-threshold counts, map presets/custom rectangle, human/AI difficulty,
negative/high efficiency, response signs and the zero-interval boundary.
The grid combines human/AI, six difficulty indices, eight efficiency values,
five installed map sizes and counts zero/interval-minus-one/interval/
interval-plus-one/twice-interval, cycling through all seven faction slots.

The findings specify a warning. The separately checked
[bureaucracy citizen input](bureaucracy-drone-input.md) now establishes the
component and its global-list-position distribution, not final citizen results.
Later citizen processing, earlier territory handling, complete command
sequencing, dialog rendering and full-game observations remain open.
