# Nonhuman victim: retained Headquarters capture

Clearing the victim's human-membership bit produces the **same retained capture result** in the original game. Both paired executions have identical observed native writes, random draws, and recorded canonical state except for the deliberately changed human mask. [Numeric cases](nonhuman-victim-capture-cases.json) include complete Base/Unit bytes and sampled knowledge, regional presence and history fields.

This extends the [ordinary Headquarters capture](headquarters-capture.md). Captor1 remains human; victim2 has another surviving Base. Target Base1 starts as a population3 HQ, former owner2, timer0, facilities1/3/4/7/8. The paired inputs differ only at the human membership byte: mask6 versus2. Active mask6, turn6, Librarian difficulty3, healthy adjacent Scout, technology, treasury, world, and both RNG seeds3 remain fixed. The new human control exactly reproduces every recorded before/after field of the prior HQ receipt.

| Observed result | Human victim | Computer victim |
| --- | --- | --- |
| Target owner / former owner | 1 / 2 | 1 / 2 |
| Population / assimilation timer | 2 / 50 | 2 / 50 |
| Facilities remaining | 4,7,8 | 4,7,8 |
| Captor / defender credits | 220 / 80 | 220 / 80 |
| Captor / defender Base totals | 2 / 1 | 2 / 1 |
| Retained cached population totals | 1 / 4 | 1 / 4 |
| Defender Former home | Remaining Base2 | Remaining Base2 |
| CRT draws / final seed | 37 / 1951609278 | 37 / 1951609278 |
| Shared RNG draws | 0 | 0 |

The 120-credit loot uses the original population and cached defender population: `200*3/(4+1)`. Both controls remove HQ and Tanks, select Scout production through the same native call, copy the same governor donor, repair the same defender Unit home, and recalculate the captured Base. Both non-target Base records remain unchanged. There is no restart event. Recorded raw Base bytes, Unit bytes, target tile, visibility/remembered populations, regional presence, history stamps and paired counters all match across the role controls.

For this retained capture, the port can apply the existing human-victim canonical rules to the computer victim. This does not establish computer-captor behavior, a paid HQ relocation, or last-Base disposition. The victim's starting200 credits stay below the relocation gate.

Execution stops at the original captured-Base screen entry `0x420b10`, called from `0x521dc8`. That screen is unanswered; capture and movement have not returned. The existing `SEIZEBASE` display acknowledgment uses the prior verified unused-return service. No capture result, facility outcome, RNG result or new gameplay choice is supplied.

Verification: `tmp/reference/check_nonhuman_victim_capture.py --verify-only`. Immutable evidence: `tmp/reference/nonhuman-victim-capture-evidence-20261019/manifest.json`.

## Port integration

The live capture caller now admits the bound computer defender for ordinary
retained Headquarters capture with another surviving Base, below the paid
relocation threshold, and with explicit unchanged former-owner history. The
captor still requires local human command authority. Computer captors, paid HQ
relocation, liberation and last-Base disposition remain separate integrations.

The live regression adds a saved computer-controlled defender to the existing
capture fixture and checks ownership, population, loot, home repair, history,
RNG, actual Start restoration and the captured Base assimilation tail. This is
not evidence of computer startup, AI decisions or a complete faction scheduler.
A separate rejected paid-relocation control preserves the full snapshot, RNG
and frontend update queue. All eighteen prior human scenarios remain covered.

Validation: the twenty-scenario live capture group passes in the cached build;
Clang correctness analysis passes for BaseCapture, BaseAllocation and
ConventionalMovement. Focused ten-translation-unit ASan/UBSan instrumentation
passes the live capture and last-Base groups (other linked objects use the
normal build; leak detection disabled).
