# RE-SMAC-KNOWN-AUTHORITY-001: known-count inputs and reset/load lifetime

The regional counter (reference receipt `regional-lifecycle-and-territory.md`) reads a retained
per-faction flag word and the tile's saved item/exploration fields. Its two
flag predicates have distinct real producers; neither should be inferred from
human status or simply classified as a scenario-only override.
Their bit indices in that flag word are9 for map reveal and29 for Search objectives.
[Numeric controls](resolved-site-pod-query-cases.json) include initialization,
editor/technology fragments and actual file round trips.

## The two faction predicates

| Counter predicate | Established authority |
| --- | --- |
| All-tiles known count | Retained map-reveal flag. A technology with its reveal-map definition flag sets it; save loading reconstructs it from orbital counts/available owned reveal-map technologies. |
| Special-marker-only known count | Retained **Search objectives** AI strategy setting. The native accepted strategy-editor commit sets/clears it from that checkbox. |

The second predicate wins when both are set: count only tiles with the special
item marker, regardless of ordinary exploration or map reveal. Otherwise map
reveal counts every tile; absent both flags, use that faction's tile exploration
bit. The marker is item-word bit20 from the
[site query bindings](resolved-site-pod-query.md), **not** a faction exploration
bit, Monolith or a resolved nonzero pod result.

The installed technology definition for Secrets of Alpha Centauri carries the
reveal-map flag. The native award fragment tests that definition flag, ORs the
actor's exploration bit into every map tile, updates Base visibility metadata
when present and sets the faction's retained map-reveal flag. The new dynamic
control admits no Bases and runs that actual fragment; it is not a full research
award or technology acquisition UI test. Other award effects remain separate.

The strategy control executes the native accepted editor commit fragment with
Search unchecked/checked. It preserves unrelated map-reveal state. The preceding
editor choice is supplied; no editor dialog was operated. This establishes an
actual setting writer without asserting that the editor is its only producer.
Broader AI policy updates of strategy and the complete bit20 marker lifecycle
remain unknown here.

## Fresh, restart and saved-game boundaries

Cold executable memory gives both tested positive faction words0. Actual fresh
turn-zero initialization clears a poisoned complete flag word0 for each tested
owner, leaving its peer alone until that peer is initialized. These prefixes
end after the reset section and before placement; they are not full seven-faction
frontend starts. Later strategy generation can set strategy flags separately.

The same actual initializer prefix on a nonzero turn preserves both poisoned
known-counter flags. Do not clear them merely because an empty faction restarts.
This agrees with the separate regional-array restart lifetime.

Ordinary save writes the faction flag word unchanged. Load reads it back and
then, for positive factions1..7:

1. Clear the map-reveal flag.
2. Set it when **any** of the four retained orbital counts is nonzero.
3. Also set it for any currently available owned technology whose definition
   carries the reveal-map flag, using the existing native availability query.

The Search objectives strategy flag survives this normalization. Loading a
supplied map-reveal flag without an orbital/technology authority therefore
clears it, while the same file with an orbital count or an owned qualifying
technology restores it. Controls cover each of the four orbital fields, an
unowned qualifying technology, no authority, and a synthetic negative nonzero
orbital value. That last case measures the native nonzero test, not admission
of negative physical satellite counts.

Do not generalize file-load reconstruction into a per-read or per-turn rebuild.
The new receipt does not trace every live orbital completion/destruction update
of map reveal. Preserve the flag's actual phase lifetime. Tile item flags and
the resource pattern restore independently, and the special marker is not
recreated from the faction's reveal state.

Ordinary fresh-campaign admission may safely require the two known-counter
flags clear **at the verified fresh reset boundary**. It cannot silently use
that assumption for arbitrary loaded games, later technology/orbital states,
AI strategies or scenario/editor states. Fail unsupported contexts or provide
their saved/live authorities; do not replace unknown providers with guessed0.
