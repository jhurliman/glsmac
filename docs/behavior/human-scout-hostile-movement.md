# RE-SMAC-HUMAN-SCOUT-HOSTILE-001: local human Scout attack

The existing neutral flat-land Scout battle can be connected to hostile
directional movement **for the bounded local two-human Vendetta context below**.
Three focused cases pass the original keyboard gate, complete movement-wrapper
return and subsequent selected-unit keyboard gate. The healthy neutral
human-versus-human case matches its human-versus-AI control in all recorded
rounds, casualty, survivor, promotion, movement, selection and RNG outputs.
See [literal cases](human-scout-hostile-movement-cases.json).

Inputs are two actual allocated Gun/Scout-armor/Infantry/Fission Scout Patrol
records, healthy, unhomed, no abilities; flat land source(8,8), destination(9,9),
no Bases, full movement, local ordinary directional input, turn6, Librarian
global/stored difficulty, focus/control faction1 and mutual Vendetta. Diplomacy
display records are supplied already initialized. This is a supplied ordinary
combat state, not a replay of complete two-human game/contact/war setup.

The neutral comparison differs only in whether faction2's original human
ownership flag is set. Loaded image/map state, with that flag normalized, is
identical before command admission. Both start stored/effective morale1/1 and
the same two RNG states. Elite changes attacker stored/effective morale to6 and
uses a specified different CRT seed to exercise a survivor with movement left.

| Case | Rounds / CRT draws | Attacker after return | Next selected-unit direction |
| --- | --- | --- | --- |
| Both human, neutral; CRT seed3 | 19 /38 | Damage9; stored/effective morale2; spent3, allowance3 | Refused as exhausted |
| Human attacker, AI defender; same seed/state | 19 /38 | Identical | Identical refusal |
| Both human, Elite; CRT seed1 | 11 /23 | Damage1; stored/effective morale6; spent3, allowance6 | Admitted;3 movement left |

In all three, defender dies; surviving attacker remains **on the source square**.
Destination becomes empty. Wrapper return is0 even when the attacker wins.
Primary selection remains attacker0, secondary reference to dead defender
clears, surviving stack links are empty. The neutral attacker promotes1→2;
Elite remains6. Do not infer movement into the defeated unit's square from
the battle result or interpret wrapper0 as attacker death.

All three initial keyboard requests emit the original selected-unit/direction
arguments. Their complete movement wrappers then return normally. The subsequent
keyboard request uses the actual survivor without resetting unit/RNG state:
ordinary exhausted survivor requests one cannot-act notification, emits no
command and clears the pending-action latch; Elite emits another command.
Admission itself changes neither unit bytes nor either RNG stream. The Elite
second movement body is not executed by this receipt.

## Human-specific admission and diplomacy boundary

Human ownership is actually consumed. The human-defender path makes additional
directed diplomacy reads, including agreement/contact-related flags and a
reverse-direction exception flag. With the supplied mutual Vendetta and no
Treaty/Truce/Pact, both-human and AI-control paths converge without a decision
prompt. The original mutual relation setter still executes and updates
contact/war-history metadata; this receipt does not assign new semantic names
to all of its retained bits. It does not justify bypassing diplomacy checks
or treating an arbitrary foreign unit as an admitted hostile target.

No partial-movement, treaty-breaking or other decision answer is supplied.
Promotion and cannot-act notifications are supplied presentation services,
after the native promotion or refusal decision. Actual strength, rounds,
casualty/compaction, movement spending, morale, diplomacy helpers and actionability
execute original instructions.

## RNG and presentation limits

Round/promotion draws use the actual CRT stream. Neutral cases finish CRT state
1369276809; Elite finishes255755822. Each tested full wrapper also makes one
shared-stream draw during **first-use presentation setup**, advancing supplied
state3→1018897798. This is recorded, not replaced. It is not evidence for a
universal one-shared-draw-per-battle rule; the process-initialized/repeated
presentation lifecycle remains outside this receipt. Subsequent admission
makes zero draws from either stream.

Headless rendering/time, no-input presentation polling, promotion notification
and stack-local UI teardown retain the existing
[normal-return](scout-attack-normal-return.md) and
[focused-battle](focused-scout-battles.md) service boundaries. Animation gameplay
and its temporary relocation/restoration execute, but visible Windows rendering
and teardown bodies are not fully exercised. Keyboard execution starts at its
previously established decoder boundary; earlier window/event routing is supplied.

This handoff covers the healthy neutral pair and specified Elite attacker
continuation, not every human-versus-human outcome. Defender victory, other
morales/damage/terrain/reactors/abilities, Bases/capture, partial movement,
other diplomacy, alternate input modes, hotseat handover, network play and full
two-human startup remain separate. No gameplay C++ or original files changed.
