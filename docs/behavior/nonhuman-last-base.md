# Computer victim: population-one last-Base disposition

The exact [population-one last-Base fixture](population-one-last-base.md) takes a different ending path when only the victim's human-membership bit is cleared. Captor1 stays human, victim2 loses its only Base, and only captor1 remains active. This is a finite two-faction control; a computer-faction elimination with more survivors need not end the game.

The first independent execution stops unanswered at the original `CONQSING` notice, caller `0x5ca7bc`. Through restart refusal it has the same recorded canonical state as the human control except human mask2 versus6, and exactly the same 2,050 CRT draw rows. Both executions remove the population-one Base and its homed Former, give the captor 100 credits of loot, add 10 starting-energy credits to the defender before restart refusal, and clear victim active membership. Credits become200/110; only Base0 and two owner1 Scouts remain. Stored population totals remain1/1.

The source distinction occurs at restart refusal `0x5ca48c`. A human victim in this local profile takes the early path that sets ending state7 and game flag bit1. A computer victim enters the general deactivation/survivor-count path. With this fixture's sole remaining faction, the native caller reaches the single-faction conquest notice while ending state is still0. The first control stops there without supplying a response.

At `0x5ca7bc`, the display's return value is discarded: the caller immediately loads the existing ending state from memory. If it is zero, native code writes ending state4, then ORs game flags with `0x2001`. This source check justifies a separate display-acknowledgment continuation; it does not supply a victory decision.

The first notice-boundary snapshot also differs from the human control in target-tile remembered-items bytes16/17: human2/0, computer1/16. Those bytes are recorded without deriving a general visibility-update rule. All non-mask fields at the common observed stages through restart-refusal entry match the human control. Complete movement-wrapper return remains outside the accepted human boundary and must be stated separately for the computer continuation.

The separately checked continuation now executes that source-verified acknowledgment. Native code writes **ending state4 and game flags24833 (`0x6101`)**, returns from restart and disposition, and stops at the same unresolved virtual UI cleanup call as the human control: PC0, return address `0x6045a8`, stack delta−21664. No complete movement-wrapper return is claimed. The existing capture and wipeout display acknowledgments are reused; `CONQSING` is the one added display-only acknowledgment.

At this common final boundary, **every recorded field matches the human control except human mask, ending state and game flags**. The target tile now also matches. Both have final CRT seed124333717, unchanged shared seed3, credits200/110, Base totals1/0, cached population1/1, identical remaining Base bytes and identical two-Scout state. Do not retain the human-victim ending7 branch for this computer-victim single-survivor configuration.

[Numeric cases](nonhuman-last-base-cases.json) include both boundary receipts and final paired state. Verification: `tmp/reference/check_nonhuman_last_base.py --verify-only`. Frozen evidence: `tmp/reference/nonhuman-last-base-evidence-20261019/manifest.json`.

## Port integration

The private population-one capture/disposition path now accepts the bound
computer victim in the measured two-faction failed-restart case. Human command
authentication is unchanged; computer restart placement and larger survivor
configurations remain separate integrations. A failed computer disposition with
a sole human survivor commits an explicit conquest winner, independently of the
existing human-defeat request. This is a semantic port outcome, not a claim that
all original ending flags, scoring, movies or final movement-wrapper cleanup
have been reproduced.

Simulation format V33 saves the winner. V11..V32 remain readable and never
acquire a victory by recounting survivors. Validation rejects inactive/unknown
winners, nonhuman winners, multiple survivors and simultaneous defeat/victory.
The actual capture caller emits the ending event after commitment; later queued
gameplay and direct writes are blocked. The existing outcome UI shows conquest
without describing the winner as eliminated, including after actual Start.

Validation: all40 selected native capture/ending/simulation-codec groups pass,
including the real capture ending notification and blocked reentrant command.
The actual outcome and capture-screen GSE scripts pass. Correctness analysis
passes for seven changed production translation units. Focused thirteen-TU
ASan/UBSan passes the same40 native groups; other linked objects are from the
normal build and leak detection is disabled. The immutable original-game
checker passes independently. No complete AI match or native ending presentation
pixel parity is claimed.
