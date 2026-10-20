# Movement command inputs: controller mode, encounters and resolved sites

**The movement stream selector is ambient controller state, not an owner/AI/visibility property.** It is a DWORD at `0x944600`. Zero selects shared-low16 interval movement RNG; any nonzero value selects CRT raw modulo cost. The cold mapped image supplies zero from its zero-filled section tail. The normal local controller then overwrites it at explicit boundaries. [Numeric captures](movement-command-input-lifetimes-cases.json) bind 63 existing controller cases, actual standard seven-owner/global/native boundaries, twelve direct remote requests and three new complete original save/read roundtrips.

## Exact owning transitions

Local controller `53be70` initializes EBP zero and ESI one. It writes:

| Instruction | Boundary | Value |
| --- | --- | ---: |
| `53bf3b` | Before `53a530` global advancement | 1 |
| `53cae7` | Before `53bb60` faction upkeep | 1 |
| `53cb15` | After upkeep, before command dispatch | 0 |

The zero write precedes **both** dispatch branches: human `527eb0` at `53cb4a` and computer `58f6b0` at `53cb62`. A computer or offscreen Unit does not imply CRT mode. In the measured standard start, native-owner-zero command entry and return use zero, Gaians first human command uses zero, and all six positive computer commands use zero. Upkeeps use one. The connected actual University-through-Peacekeeper run again alternates upkeep one and computer command zero. Existing shared-stream human and actual AI command receipts remain valid.

The twelve remote direct requests use one because they resume pre-command snapshots: before global birth, immediately after global birth, or at first positive upkeep entry after the native phase. Each snapshot really contains one. The primitive is invoked directly before that continuation crosses `53cb15`; no selector result was substituted. Their actor is owner two, but that is not the selector's producer. Entering the ordinary command body through its controller would cross the zero write first.

The consumer at `5aefa5..5aefdc` reads this word. Nonzero uses `660a8d` and raw remainder by positive cost; width at most one suppresses that CRT call. Zero calls shared range `6330f0(0,cost)`. Encounter admission independently uses CRT modulo 100; combat and civilian fallback also retain their own CRT calls. Do not reroute every random operation using this selector.

The PE scan identifies six direct absolute writes: the three local writes above, `53ed28` writes one in the separate `53ea90` flow, and alternate controller `53eed0` writes one at `53eeef` then zero at `53f18e`. The alternate entry also seeds CRT from the map pattern. These are separate source branches, not a reason to apply that seed or their lifecycle to the measured local path. Indexed/indirect writes are not excluded by the literal-encoding scan alone; the executed captures and file checks establish the bounded lifetime.

## Save/read and legacy restore

Three new full ordinary file-body roundtrips save with selectors zero, one and 99. Before loading, independently supply 123, 124 and 222 respectively. The original reader leaves those current values unchanged. No file I/O interval includes the selector and no CPU write to it occurs in either file body. Thus ordinary original saves do **not** restore this word. It is ambient state later owned by controller execution, not a serialized per-Unit field. Full frontend/load-to-command execution remains outside these file-body controls.

For a port, preserve an explicit command phase/selector at deterministic replay boundaries. A legacy port restore that resumes an ordinary local command boundary should select zero from that authenticated boundary; a restore that resumes global advancement/upkeep uses one. An unknown mid-phase save needs an explicit migration/resume rule. Do not choose from faction ownership, human mask, renderer visibility, nonlocal UI mode, current seed, or the cold image default alone. This is port integration guidance derived from the proved transitions, not an assertion that the original stored a new field.

## Encounter-enabled input

The owning suppression input is the **full 32-bit primary rule word** at `0x959fa4`, mask **`0x00200000`**. At `5acc35..5acc3f`, a set bit bypasses this encounter stage. Within the already admitted conventional-land profile, the typed input is:

```
encounter_enabled = (primary_rules & 0x00200000) == 0
```

The measured fresh Standard world has full primary word `0x00001a0e`, so encounters are enabled. Standard menu selection changes low rule bits and can preserve carried high bits; do not truncate a loaded word to sixteen bits or infer this flag solely from the Standard display label. The new save/read control with bit `0x00200000` set verifies that this rule word, unlike the selector, is restored from the file. No particular menu checkbox label is authenticated by this packet; the numeric consumer and saved authority are explicit.

This flag is one gate. Keep destination fungus/band, tube, actual empty destination, same domain, ordinary design/project/ability admission, nearest-Base distance and reductions in their source order. Do not pass `encounter_enabled=true` as permission to skip those guards. The [remote continuation contract](remote-fungus-continuation.md) supplies the ordered admitted pipeline and capacity-failure behavior.

## Resolved-site input

`5acc25` calls the canonical `5a9570(x,y)` **before** nearest-Base and encounter RNG. A nonzero site result bypasses encounter. The same query has source-authenticated pattern/rules behavior in the existing site contract; `SiteSignal` can supply it if it implements that producer. Use the resolved result, not a guessed meaning of raw `0x40000000`, exploration, ownership or cached Colony quality.

Required inputs include coordinates, tile altitude/items, map pattern `9b8b00`, primary rules `959fa4`, and retained pattern coefficients `6aefcc/6aefd0` for the fallback pattern formula. For the admitted Planet controls these are pattern 27067 and coefficients 11/61. Suppression mask `0x102000` returns no site first; rule `0x2000` switches to explicit site bits `0x0c000000`; forced bit `0x10000000` then takes its branch; otherwise the actual automatic pattern formula applies. Preserve source ordering. The typed result can distinguish its original site values; the encounter consumer only tests nonzero.

With the actual map/rules, `(49,67)` resolves a site and skips encounter, while `(47,69)` does not. Both are fungal destinations; they still reach the later ordinary movement lottery when appropriate. The [resolved-site contract](resolved-site-pod-query.md) owns the full resolver and cache lifetimes; this packet does not replace it with a disconnected Boolean heuristic.

All original receipts and headless/omitted-human-command boundaries remain intact. The old 63 controller cases are controlled-world compositions; actual standard seven-owner captures provide the separate full-world boundary authority. New file tests use memory-backed I/O and an allocated empty map, with no frontend or rendered command loop. No production port code is changed here.

## GLSMAC movement kernel

The movement kernel now accepts an explicit SharedLow16 or CrtModulo selector.
The existing ordinary map-command caller explicitly supplies SharedLow16; no
owner or visibility heuristic chooses it. The legacy shared API delegates to
the same implementation. The selected primitive compares ten actual movement
stages from the twelve remote receipts, skipping the two successful births and
replaying any prior encounter CRT prefix before movement. This includes the
capacity-failure continuation, six actual movement draws and suppressed-draw
controls. This does not activate native births or claim complete live encounters.
The encounter suppression bit and resolved-site producer above remain required
inputs for that owning continuation. No new saved selector is introduced.
