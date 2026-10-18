# RE-SMAC-BASE-RENAME-001: original inspector Rename

The checked original accepts duplicates, empty names and space-only names. Its
text control retains **24 bytes**, without trimming, case folding or Unicode
normalization. Rename also has a captured-Base diplomacy branch; accepting the
same text is not universally a harmless no-op.
[Reusable cases](base-rename-cases.json) contain29 complete handler controls,
14 native text/finalization controls and four complete save/load round trips.

## Text and acceptance

The inspector supplies the current name as the dialog's default and requests
a24-byte limit. The native input setter truncates an overlong supplied narrow
string to its first24 bytes, writes a terminator and modifies its writable
source buffer accordingly. Thus24 data bytes plus a terminator can be retained.

- Empty accepted text becomes an empty Base name. Space-only text remains spaces.
- Leading/trailing spaces and letter case are preserved.
- An exact duplicate of another owned or foreign Base is accepted. A differently
  cased duplicate is also accepted. No uniqueness, suffix or case-insensitive
  comparison runs in the commit path.
- High-byte narrow-string values survive unchanged. The checked path is byte
  based, not a UTF-8/UTF-16 codec. A supplied UTF-8 sequence crossing byte24 is
  split by the setter. This establishes byte semantics, not a universal Windows
  code page, font coverage or an actual IME/clipboard input policy.
- A NUL ends the name; bytes after it are not part of the retained C string.

The accepted dialog return is0. Any nonzero return skips name commit. Native
modal finalization returns−1 for the supplied cancellation result. Cancel can
update temporary dialog text while preserving the Base's name; do not assert
that every scratch field or preflight resource cache remains unchanged.

The final Base commit itself copies the returned NUL-terminated text directly;
it does not add a second truncation or validation pass. The24-byte limit belongs
to the native text control. Do not submit an unbounded string to the raw commit
and call the resulting behavior normal dialog admission.

## Ownership and Base Control

The complete handler first selects the inspector's current Base and recalculates
its resource state. It then checks that the Base owner is the local faction,
unless the existing editor override is enabled. A foreign-owner control without
that override returns before opening the name dialog; the override admits it.

After this ownership gate, requesting **Base Control is mandatory** before the
dialog. A refused request skips the dialog and commit. The checked ordinary
local path executes the native acquire/release helpers. Network-busy and pending
control guards are separately executed and refuse. Full successful network
acquisition, acknowledgements and replication are outside this receipt.

Both accepted and cancelled dialogs release control. The original relies on its
UI/control lifetime: these tests use a registered, existing selected Base and
do not establish arbitrary invalid IDs, replay/version authentication, concurrent
ownership change or port command admission during suspended turns. Preserve the
port's required authenticated command boundary rather than treating this GUI
callback as a general mutation API.

## Commit, name bookkeeping and selection

After admission, commit changes the selected Base's name bytes. In the checked
ordinary contexts, other Base fields, other Base records, record count/order,
selection and faction state remain unchanged after the preflight boundary.
The native default-name counters are preserved. There is no rename-time call
to the default founding-name generator or a separate uniqueness registry.

A port's name-registration structure must accommodate the verified duplicates;
its own set invariants cannot establish a native uniqueness requirement. This
receipt does not re-specify all later automatic founding-name generation.

The handler initially resolves the Base from the inspector's selected identity.
Accepted rename does not select a different Base, sort the Base array or navigate
to a neighbor. Same-name acceptance still takes the commit/release/redraw path.

## Captured Base consequences

After committing, run this extra branch only when:

1. Current owner is the local faction;
2. Former owner differs from the local faction;
3. Former owner is not in the human-faction mask;
4. The ordinary network flag is clear.

Add3 to the former-owner→local diplomatic counter, clamping that stored value
to0..20. The checked controls produce0→3 and19→20. If the separate active
diplomacy context names that same directed pair, its mirrored counter also
increases by3; that mirror is not clamped by this helper.

Then, if the former owner's directed relation toward the local faction has
**any of bits1/2/4 and also bit32**, draw the actual shared gameplay RNG in
range[0,2). On result0, set relationship bit1024 in that direction. On result1,
leave that bit unchanged. Do not describe this marker alone as an immediate
treaty/Pact break. Literal native seeds1/3 produce0 in these controls and set
the bit; seeds2/4 produce1 and do not. The RNG is not the CRT stream.

There is no old-name/new-name equality test before these consequences. A captured
AI Pact Base accepted with its unchanged name still incurs the counter update
and RNG call. Cancellation, an ordinary original-owner Base, or a human former
owner bypasses these effects in the checked controls.

## Repaint order and persistence

After name commit and any diplomacy, the handler releases Base Control, requests
the inspector name-header repaint, then requests map repaint at the Base's
current coordinates with literal parameters3/2. The actual header-copy fragment
reads the new name from the Base record. No separate stale UI name is authoritative.

The actual map dispatcher visits views in slot order: a nonnull primary view is
included regardless of its active flag; later nonnull views require their active
flag. A controlled primary/inactive-secondary/active-third case dispatches to
the primary then third view. This measures dispatch order, not pixels, timing,
camera movement or successful presentation on an actual window.

Cancelled/refused controls do not take those post-commit header/map requests.
The earlier resource recalculation remains a distinct preflight operation.

Four complete native ordinary save/load controls retain an actual handler's
empty, high-byte,24-byte and duplicate names exactly, preserving both duplicate
records. Native file-layout backing is supplied for the headless world; this is
codec persistence, not an actual menu save, reopened inspector or visible restore.

All dialog choices/text are supplied at declared modal boundaries. Native text
truncation/finalization, ownership/control gates, commit, diplomacy/RNG, header
copy and map-view dispatch execute. Inspector/per-view drawing is supplied;
there is no full text-dialog keyboard/focus continuity or rendered original-game
claim. No production changes or broad-goal resumption.
