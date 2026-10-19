# Startup site score, deficit and compensation

The original calculates compensation from **runtime site quality and spacing**, then credits configured starting Units. The [reference](startup_site_balance_reference.py) independently reproduces 28 scores and deficits, 161 ordered terrain-marker writes and all 12,800 final tile-item comparisons across four native balancing executions. [Cases](startup-site-balance-cases.json) contain numeric components and outputs; installed map tile inputs remain private.

Use this calculation after initial packages exist. Do not prescribe fixed Pod counts or coordinates. The unchanged replay matches the complete accepted [seven-faction Planet startup](seven-faction-planet-start.md), including final Base/Unit state and every map tile byte.

## Anchor and score

For each faction1..7, select its nearest Unit to `(0,0)` using the original wrapped map distance; equal distances select the later Unit ID. If its tile region byte is at least64, repeat once from that Unit's position while excluding that Unit. Skip the faction if no anchor remains. This is a single replacement attempt, not an unlimited land-Unit search.

For an anchor with region area `A`, find the smallest `trunc(3200*distance/map_tile_count)` among other anchors in the same region; initialize this minimum to9999. The distance is the original doubled-coordinate metric with horizontal wrapping governed by the map flag. The reference expresses its exact integer sequence.

The score is the sum of:

| Component | Exact arithmetic |
| --- | --- |
| Same-region spacing | If minimum<1000: `trunc(minimum/5)-3`; otherwise `clamp(trunc((A+31)/32),1,8)` |
| Nearby terrain | `clamp(trunc(C/2),0,8)` from the ordered inner21 scan below |
| Region size | `clamp(trunc(A/64),0,3)` |

For each valid inner21 tile, extract rain=`(byte0>>3)&3`, elevation=`byte0>>5`, rockiness=`byte5>>6`, and live items. Skip rain0. Set `h=elevation-2`. Add1 to C if `(h>0 or items&0x80)` and rockiness<2 and `(rockiness==1 or h>1 or rain>1)`. Independently add1 if rockiness==1 and h>1. Clip/wrap coordinates before reading. These are heuristic score terms, not actual tile yields. Preserve the literal low-elevation/item condition rather than replacing it with an assumed land-only rule.

## Deficit and Colony Pod

Let M be the maximum score, initialized to0. Process factions in ascending order. For each configured faction `UNIT` bonus, subtract3 when the referenced design's native role byte is8; subtract1 otherwise. Stock Colony Pod and Scout controls establish both credits. These credits come from the faction definition, not a live Unit census.

```
D = M - own_score - configured_UNIT_credit
grant Colony Pod iff difficulty>1 and
    (D>=3 or difficulty >= (human ? 4 : 5))
```

A grant calls the actual Colony Pod factory at the anchor, clears home to−1, and subtracts3 from D. A high enough difficulty can therefore grant a Pod even when D<3. For difficulty<2, replace the remaining D with `trunc((D+1)/2)`. The finite native controls here use difficulty3; other difficulty branches are source-mapped.

## Remaining terrain compensation

Every eligible faction consumes one CRT draw before the Pod check, including a faction with D0. Its low four result bits choose an order through the 16 outer offsets9..24: iteration i uses `9+((7*(random_nibble+i+1))&15)`. Wrap once horizontally where enabled; clip invalid coordinates. Invalid positions still advance i.

For a valid position, if the actual resolved site/pod query is nonzero and both live markers `0x04000000/0x08000000` are absent, decrement that faction's stored score. M remains fixed. This does not directly decrement D. Then OR a marker into live tile items: use `0x04000000` for at most two iterations when remaining `D-i>=3` and i has the same parity as the number of these special markers already granted; each such write additionally decrements D. Otherwise OR `0x08000000`. Increment i and continue while i<D. The mutable loop limit matters. These masks are terrain site/pod markers; they are separate from creating a Colony Pod Unit. The [resolved site query](resolved-site-pod-query.md) governs their later interpretation.

All four controls consume exactly seven CRT draws, seed531966586→1537134187. Native Unit construction and its visibility writes remain owned by the Unit factory; the independent reference verifies the compensation item writes and final live items.

| Control | Gaia score | Gaia UNIT credit | Gaia deficit | Total compensation marker writes |
| --- | ---: | ---: | ---: | ---: |
| Unchanged Planet | 9 | 0 | 10 | 39 |
| Clear Gaia inner21 rain bits | 1 | 0 | 18 | 47 |
| Colony Pod UNIT-credit descriptor | 9 | 3 | 7 | 37 |
| Scout UNIT-credit descriptor | 9 | 1 | 9 | 38 |

The UNIT controls change only a descriptor at the balancing boundary; they do not construct the descriptor's extra starting Unit. They isolate the compensation credit. The observed stock maximum19 belongs to Morgan's isolated-region anchor; it is not a faction constant.

## Site-search boundary

Initial search admission and final placement are distinct. Native search accepts `(16,58)` for Gaia, then its inner21 refinement selects `(17,61)`. Other changes include Hive `(35,37)`→`(34,36)`, Sparta `(29,27)`→`(29,25)`, Peace `(13,71)`→`(15,71)`. These are receipts, not prescribed coordinates. Refinement uses a strict greater comparison, preserving the first candidate on tied scores. The full search-admission/cache/refinement producer remains a separate audit; this handoff independently closes balancing arithmetic and terrain side effects.

Verification: `tmp/reference/check_startup_site_balance.py --verify-only`. Frozen evidence: `tmp/reference/startup-site-balance-evidence-20261019/manifest.json`.
