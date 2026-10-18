# Dominance accepts consumed ranking scores

2026-10-18, stock SMAC. **Do not reject negative retained scores before the
dominance comparison. Native dominance accepts the normal consumed `−1` words.**
Six complete native predicate calls reuse the admitted late-game setup from
[the runner-slot0 receipt](dominance-runner-slot0-score.md): turn251, human
leader1, difficulty2, six owned Bases and retained Base count6, no victory
pressure, ordinary scenario tuple9999/9999/0. The retained score table is
explicitly supplied as `[0,-1,-1,-1,-1,-1,-1,-1]`; no score refresh runs.
[Literal cases](dominance-consumed-score-sentinels-cases.json).

| Ratio branch | Signed comparison | Leader−1 / runner2 score−1 | Leader−1 / runner0 score0 |
| --- | --- | --- | --- |
| Normal, Conquest enabled, Rivalry off | 2×leader >= 3×runner | −2 >= −3: **true** | −2 >= 0: **false** |
| Conquest disabled, difficulty2, Objectives Required9999>1000, Rivalry off | leader >= 2×runner | −1 >= −2: **true** | −1 >= 0: **false** |
| Intense Rivalry enabled | 4×leader >= 5×runner | −4 >= −5: **true** | −4 >= 0: **false** |

The predicate multiplies the retained words, then performs a signed comparison.
It has no nonnegative-score admission check or sentinel-to-zero conversion.
The positive runner2 controls declare active mask6; runner0 controls declare
active mask2. Runner activity is not used to substitute a score. Actual pressure
helper returns0 in every case, so all six execute the score comparison rather
than a pressure shortcut. The native return values are exactly1 and0 as shown.

All calls return normally, preserve all eight score words, make zero score
provider calls and consume no RNG. No helper return or comparison result is
supplied. This is a controlled late-game consumer test, not a newly replayed
full-ranking update or organically developed campaign. The separate
[continuous ranking-to-summary receipt](ranking-dominance-score-order.md)
establishes why consumed words are present at that boundary. Together these
receipts require preserving the retained table and allowing its signed values
through the predicate. The standing research goal remains paused.
