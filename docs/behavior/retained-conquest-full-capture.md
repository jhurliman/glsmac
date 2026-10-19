# Full capture with retained conquest state

Two original owning movement/capture executions accept an existing former-owner/assimilation record and reach the captured-base screen boundary. The former-owner byte changes the actual capture behavior; it is not merely display metadata. Compare the complete contexts in `retained-conquest-full-capture-cases.json` with the earlier [ordinary HQ capture](headquarters-capture.md).

All three controls use the same local two-human, equal-technology, no-project, clear flat-land fixture: a healthy full-move Scout takes undefended defender Base 1 at `(20,16)`, population 3, with another defender base surviving. The target uniquely has HQ facility 1 and facilities 3,4,7,8. Defender credits are 200 and retained population aggregate is 4. Captor is faction 1, displaced owner is faction 2. Each new run freshly prepares the original fixture and changes only target former-owner and assimilation bytes before entering the real movement routine.

| Control | Before former / timer | After population | After former / timer | Remaining facilities | CRT draws |
| --- | --- | --- | --- | --- | --- |
| Ordinary first capture | 2 / 0 | 2 | 2 / 50 | 4,7,8 | 37 |
| Return to recorded former faction | 1 / 27 | **3** | **1 / 0** | **3,4,7,8** | **0** |
| Already conquered, third former faction | 3 / 27 | 2 | 2 / 50 | 4,7,8 | 37 |

The return-to-former path emits the original **`LIBERATEBASE`** notification at the existing acknowledgement boundary. It skips population loss and the random facility loop. It still removes HQ, but skips the ordinary forced clears of facilities 6 and 3. In this fixture facility 3 therefore survives. The third-former path emits `SEIZEBASE` and has the same complete final state and random sequence as first capture, despite starting with a nonzero assimilation timer and a different former owner. The displaced faction 2 becomes the new former-owner/art family.

**Loot is still 120 in both paths.** It is calculated from pre-loss population: `200*3/(4+1)`. Captor/defender credits change `[100,200]→[220,80]` in all three cases. Liberation does not bypass loot in this measured domain.

Both retained-state paths still copy the same governor donor, clear stored minerals 7→0, reset the production queue to empty, replace target 1 with native selected target 2, and reset baseline item/minerals to -38/0. Food remains 5. Ownership counts change `[1,2]→[2,1]`; the original home-repair/unit movement results exactly match ordinary capture. Other bases, active/human/controller masks, selected references, technology possession, and the retained total-population aggregate stay unchanged. The total-population cache is not normalized to the post-capture population here.

Both execute the normal **89-technology comparison**. The completed comparison records zero technologies unique to the defender, both owners' research points remain `[0,0]`, and no technology is acquired. Rules word is zero, so the alternative capture-tech branch is not entered. This establishes the equal-technology branch for these contexts; unequal technologies, choice-bearing tech acquisition and other rule configurations remain separate work.

The third-former path consumes the same 37 CRT values as ordinary first capture: one per facility 1..38 except 26, including absent facilities. Seed 3 ends at 1,951,609,278. Liberation consumes none and leaves seed 3 unchanged. The shared seed stays 3 in both. The receipt checks every raw random value, facility ID and setter call, not just final surviving facilities.

The original owning movement entry runs through native capture and its resource/citizen recalculation, then stops on entry to the captured-base screen with arguments `(Base1,0)`. No captured-base screen body or close action is executed. The inherited notification acknowledgement returns zero at the same established call site, with no new user choice or decision supplied. Existing preparation and headless services remain those of the accepted HQ fixture.

These are controlled retained-state inputs followed by full original capture callers, not a naturally played multi-capture history. They extend the earlier byte-write fragments for this concrete ordinary domain. Population-1 razing, other objectives/rules, defeated-faction restart, other facilities/technologies, foreign faction replacement and broader capture modes are not certified by these two cases.

## Port integration

The live local Scout capture transaction now accepts retained conquest history. It compares the existing former-owner authority with the recipient before changing ownership. Return preserves population, clears assimilation, skips all facility CRT draws and the forced facility-3/6 removals, while still removing HQ, transferring pre-loss loot, repairing homes and resetting production. Other former owners become the displaced owner with timer50 and retain the ordinary population/facility RNG path. The allocation publication boundary independently checks the same transition. No saved field or wire version changes.

The extended live capture group checks successful return, retained non-return and late rollback, including Pop wrapper identity, food, governor, homes, production, both treasuries, stored population totals, exact RNG, frontend refresh-before-selection, actual Start reload and subsequent assimilation. Its two-participant fixture uses retained former2 for the non-return case; the original three-former numeric receipt above independently establishes former3 as the same native branch. Existing first/last-base capture and riot transfer consumers remain covered. Original LIBERATEBASE/SEIZEBASE notification text and population-1 capture are not implemented by this patch.
