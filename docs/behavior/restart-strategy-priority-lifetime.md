# Current strategy priorities through nonzero restart

Preserve all five current strategy priorities on the supported nonzero-turn
last-Base restart, failed restart/elimination, and empty-owner disposition
paths. This includes current Fight. Do not copy faction-definition priorities,
reroll personality, or reapply primary rule `0x40` on these paths. An absent
legacy-port optional value remains absent; restart supplies no new authority
to resolve it from the header.

The original uses the **same initializer**, with a global-turn branch, rather
than a separate smaller initializer. `5c89bc` is an interior initialization
boundary in function `5c8650`, not a separately called function entry.

| Current field | Owner-zero address | Fresh definition-copy store |
| --- | --- | --- |
| Fight | `95d584` | `5c8a5e` |
| Wealth | `95d588` | `5c8a88` |
| Power | `95d58c` | `5c8a76` |
| Growth | `95d590` | `5c8a82` |
| Tech | `95d594` | `5c8a6a` |

Each address adds `owner * 0x20bc`. At `5c89be`, the initializer reads global
turn `959fdc`; the conditional jump at `5c89ef` goes directly to `5c8da5`
when that turn is nonzero. It bypasses every definition-copy store above,
the five randomized-personality stores `5c8c5d/74/8b/a2/b9`, and the
Fight=1 rule override at `5c8cc8`. The second initializer argument does not
select this branch. The three unconditional -1 stores before this jump
target `95d59c`, `95d598`, and `95d60c`; they are separate fields.

## Disposition call boundary

Disposition `5ca7f0` returns early for neutral owner0 (`5ca803..813`) or if
its scan finds any live owned Base (`5ca825..82b`). Otherwise it removes
remaining own Units, clears the owner's cached Base total at `5ca88c`, and
calls **`5c8650(owner, secondArgument)` at `5ca893`**. Success/failure is
tested only after that call, at `5ca89b..8a1`. Thus refusal/no-site failure
does not select a fresh personality prefix; it has already taken the
nonzero-turn branch. Disposition itself has no direct accesses to these
five priority addresses. The remaining initializer references to these
addresses are reads, not a later priority reset.

The empty-owner faction-upkeep tail `53bb60` tests its cached Base total
and counted Unit byte at `53bd99..da7`, then calls this same disposition
routine at `53bdab`. This is also the second disposition call observed
in the existing [last-Base refusal controls](last-base-restart.md).
Preserve the priorities across repeated calls; do not deduplicate unrelated
initializer effects such as the starting-energy addition.

For the port, keeping the existing four priorities in
`CommitSmacFactionDisposition` is consistent with this boundary. Apply the
same preservation to current Fight in `PrepareSmacNonzeroInitialization`
and disposition commit. Fresh turn-zero initialization retains its separate
definition/randomization/rule-override contract.

## Verification and limits

Four original instruction-prefix controls execute `5c89bc` through the
nonzero jump to `5c8da5`, with owners1/2, turns1/6/100/101, second arguments
0/-1/-2, and rule `0x40` set. Distinct positive canaries in all five fields
of all eight factions remain unchanged. These checks stop at the common
continuation; they are not new full search, elimination, package, or upkeep
replays. The caller/outcome connection above is source-only and reuses
the existing full restart receipts. This does not assert that later AI
policy commands or unrelated upkeep producers can never change priorities.

Private frozen packet:
`tmp/reference/independent-restart-strategy-lifetime-evidence-20261021/`.
Manifest SHA256:
`1028bf34ab0793c344e7c035dab3f90f64a97ffa6c08bb0af06e6c61c83df346`.
It contains the four scalar controls, source windows, and probe. Executable
SHA256: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
