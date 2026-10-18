# Worker repair after a surviving flood

**Opening the Base inspector repairs allocation. A same-Base cached resource
query alone can leave it stale.** The inspector explicitly invalidates the
resource-cache key before making its query. Do not infer its behavior from the
query's unforced argument alone.

The original full resource query selects between two paths:

- Unforced and cached for the current Base: return without calculations or
  allocation writes.
- Forced, or cache key differs from current Base: support → intake/worker
  allocation → nutrients → minerals → energy, including citizen calculations.
  These calls write the retained Base resource/allocation fields.

One actual population12→6 flood retains its eight noncentral worked tiles,
four Doctor specialists, packed padding, provisional-count canary and cached
Base key. Independent snapshots of that same returned state establish:

| Next boundary | Observed repair |
| --- | --- |
| Unforced resource query with matching cache | No calls below the query and no Base writes; stale allocation survives |
| Fresh unforced resource query with invalidated key | Complete calculation repairs and stores allocation |
| Open inspector | Inspector invalidates the key, selects Base, and runs the calculation before presentation |
| Ordinary owner traversal into Base upkeep | First support/intake passage repairs allocation before the first nutrient calculation |
| Manual selected-worker click, initially matching cache | Preliminary query retains stale state; actual click changes bitmap and specialist count; visible post-edit forced query repairs allocation |

The first allocator writes in this admission clear provisional specialist count
91→0, reset the overallocated bitmap to center only, fill specialist storage,
and finally store the new retained specialist count. It ends with center-only
bitmap and six Doctors under the declared terrain/knowledge/rules inputs. This
literal allocation is not a universal post-flood formula; use the existing
worker producer's availability and scoring rules.

For the manual control, the preliminary cached query leaves bitmap `0x15555`
and count4. Clicking selected radius index2 changes them to `0x15551` and count5
before the forced calculation. That calculation then produces the literal
repaired state above. The original bitmap hit resolver and local edit lock
execute; the supplied hit-mask artwork is the established synthetic fixture.

The source-mapped specialist-menu wrapper follows the same ownership of repair:
an unforced query before its menu/permission passage, and a forced calculation
after an accepted edit when its presentation visibility checks pass. Existing
specialist commit/pipeline receipts cover the mutation rules. No new complete
post-flood specialist dialog or cancellation run is claimed here.

Nutrient/mineral stores remain23/45 in these controls. Opening or making a fresh
resource calculation does not post a turn. The upkeep control deliberately
stops after the first intake return, before nutrients/population/production;
it does not claim a full next turn. The inspector control stops after its
calculation and before window construction/display. The manual control stops
after forced calculation and before virtual rendering. Those boundaries execute
the actual native callers rather than replacing their allocation results.

Admission supplies installed specialist records and establishes owner territory
and explored knowledge before the flood using original producers. It does not
normalize or repair the returned stale allocation before any fork. The fresh
query's invalidated cache is explicit; the inspector invalidates its own key.
[Literal states and call/write order](post-flood-worker-repair-boundaries-cases.json).
Broad RE remains paused.
