# RE-SMAC-CAPTURE-GOVERNOR-DONOR-001: governor settings donor

**The newly captured Base is explicitly excluded from the donor scan.** Its
current owner and population have already changed, but the scan skips its ID
before testing ownership or facilities. The [two executed comparisons](capture-governor-donor-cases.json)
confirm HQ weighting and first-record tie behavior.

For a surviving captured Base, the inspected selector:

1. Starts with best weight0 and no donor.
2. Visits Base records in ascending current record order.
3. Skips the captured Base itself and any Base not owned by the recipient.
4. Uses signed population as weight, doubled when the local HQ facility bit
   is present.
5. Replaces the donor only when this weight is strictly greater than the best.
6. Copies the chosen Base's complete governor settings word to the captured
   Base. If no positive-weight eligible donor exists, it performs no copy.

The signed read, zero threshold, ownership filter and no-donor behavior are
static findings. The two positive-population comparisons execute the actual
selector on the live frame reached by the original frozen capture caller:

| Candidate Base0 /Base2 | Weights | Copied settings |
| --- | --- | --- |
| Population3, no HQ /population2, HQ | 3 /4 | Base2's distinct word B |
| Population4, no HQ /population2, HQ | 4 /4 | Base0's distinct word A |

Captured Base1 is already recipient-owned and population1. It starts with a
third governor word C. Native candidate/HQ-query observations contain only
Base0 and Base2; Base1 is visited and immediately skipped. Only the captured
Base's governor word changes; both donor records remain byte-identical and
the selector makes no RNG calls.

These are controlled selector fragments, stopping immediately after the copy.
Base2 ownership/population/HQ and governor canaries are supplied read inputs
on the real capture frame; they are not a replay of another complete capture
or a coherent new multiplayer startup. No governor bits are assigned meanings
from the poisoned words, and no later governor behavior or UI is executed.

The earlier single-attacker-Base copy was therefore one outcome of this
selector, not a universal default. The captured Base cannot nominate itself,
even if it would otherwise have the largest population or HQ weight.

Static reuse check: capture subsequently calls the **same default-production
selection helper** that ordinary founding calls. This identifies shared helper
behavior, not identical complete founding/capture transitions. Capture has
already cleared production stores/queue and established its own transfer state;
the earlier case's selected Scout is not a universal production default.
