# Ordinary Scout combat presentation

Profile: pinned stock SMAC, research 2026-10-18. Adjacent Gun/Scout Infantry/Fission Scouts, owners1/2 at(8,8)/(9,9), Vendetta, local display owner1, initial damage0, CRT seed3. [Numeric/event fixture](scout-combat-audiovisual-cases.json).

The original has conventional combat effects and sounds. In this control the visible strike/hit is a sprite effect on the damaged Unit's square. Do not attach a projectile1/projectile2 sound just because the weapon is a Gun.

| Event | Asset and frames | Sound request | Normal local pacing | Fast Battle pacing |
| --- | --- | --- | --- | --- |
| Nonfatal grouped hit, either Scout | `XI.PCX`, row0, frames0..15; crop `(2+45*f,2,43,47)` | `fx/energy1.wav`, once before the frame loop | 25ms minimum per frame, then conditional50ms tail | Same16 indices; per-frame wait0, no50ms tail |
| Fission Scout death | `XF.PCX`, row0, frames0..15; frames0..7 crop `(2+102*f,2,100,150)`, frames8..15 crop `(2+102*(f-8),154,100,150)` | `fx/boom.wav`, once during setup before frames | 50ms minimum per frame, then conditional50ms tail | Same16 indices and50ms per frame; no50ms tail |

Coordinates above are source `(x,y,width,height)`, zero-based pixels. The original loader first requests palette-index substitution253→255 on both sheets. Hit frames use translation bank3, death bank0 in this ordinary control; the actual indexed-to-output pixel conversion was supplied, so these are observed bank selections, not a new decoded-pixel receipt.

For the supplied unscaled map projection `(200,200)` with tile dimensions64×32, original placement is hit `(211,185)` and death `(182,82)`: center horizontally on projected tile center, bottom-align at projected y+tile height. The original zoom/scale arithmetic executes before rendering; these numbers are one admitted viewport, not a universal fixed screen position. A reciprocal hit uses the attacker's square, rather than keeping every effect at the defender.

The sound initializer binds the first request to the fixed `fx/energy1.wav` path, and the second to extensionless `fx/boom`; the loader's no-extension branch appends `.wav`. Both installed WAVs exist. Original sound dispatchers were executed separately with Sound Effects off/on: off reaches no playback sink, on reaches the selected sound's playback sink once. No waveform decoder, mixer, channel overlap, audio hardware or completion time was measured. Fast Battle does not suppress the observed requests. There is no additional per-frame sound request in these loops.

## Health and lifetime order

The complete command executes19 original combat rounds,38 original CRT draws and one shared draw. Attacker wins with damage9 and remains at(8,8); this control does not advance it into(9,9). Health changes are authoritative before the associated effect. This local Infantry branch groups nonfatal hit display by crossing multiples of three damage; the arithmetic compares integer quotients before/after the increment. It is not an instruction to animate every damage point or to infer a universal three-point rule for other chassis/modes.

| Completed round | Updated damage `[attacker,defender]` | Effect |
| --- | --- | --- |
| 5 | `[2,3]` | Hit at(9,9) |
| 7 | `[3,4]` | Hit at(8,8) |
| 11 | `[5,6]` | Hit at(9,9) |
| 12 | `[6,6]` | Hit at(8,8) |
| 17 | `[9,8]` | Hit at(8,8) |
| 18 | `[9,9]` | Hit at(9,9) |
| 19 | `[9,10]` | Death at(9,9), both records still present |

At grouped hits the battle-panel refresh request precedes the effect call. After the final death effect returns, original bookkeeping writes the losing record's damage13 and then deletes it. The extra write is observed bookkeeping, not another round or another health-animation event. Deletion reduces Unit count2→1; the dead record must remain available through the death effect. Keep stable presentation identities/snapshots across subsequent dense-record compaction. Opening Unit facing/display calls precede these hit events; their CVR body renderer supplies zero frames in this control, so a separate Scout attack-body clip is not established here.

## Timing, visibility and evidence limits

Six complete standalone effect controls cover the three observed effect variants × normal/Fast Battle. Every case renders all16 ordered indices, even though the supplied clock advances100ms per read and therefore exceeds both frame waits. Each iteration accepts actual now as its next timestamp: there is no elapsed-time catch-up or local frame skipping. These synchronous effect loops are separate from the shared timer pending-message coalescing established in the [timing inventory](animation-timing-inventory.md). The animated pass holds the existing shared effect inhibit until its tail finishes.

Reuse that inventory's measured Shift/network zero-wait branches. Source inspection additionally shows a network hit path skipping even frame indices; it was not executed by these new local controls. No full network-session, unfocused/minimized window or skipped-presentation policy is established. A separate conditional200ms battle-call-site pause is not a frame period and was not newly demonstrated on this complete battle path.

One complete command runs with real effect selection and112 sprite-render requests (six hits plus one death), with graphics/audio/event services explicitly supplied. A companion no-view command has identical rounds, damage writes and draw counts: no admitted viewport causes the original effect helper to return without frames or ordinary sound requests. The visible-view control admits a diagnostic viewport only during effects; original scene capture/copy, rasterization, redraw, projection, clock, event-pump and low playback services are supplied. This proves original scheduling/selection/record order, not a live GUI capture. Existing normal-command panel/promotion/teardown stubs are retained. No engine rule result, rand result, health write, casualty or combat instruction is replaced.

Asset loaders are admitted fragments with crop/load sinks, not full image decoding. Ordinary hit/death effect selection is now closed for this pair; other weapons, reactors, psi/native effects, artillery, multiple defenders and exact CVR body clips remain separate work. SMAX/PRACX parity is not claimed.
