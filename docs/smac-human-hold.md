# Human Hold and Skip commands

Original behavior is recorded in [the x86 Hold contract](behavior/human-hold-order.md) and its thirty literal cases. This implementation connects ordinary supported SMAC ground units to the existing local native event path.

- H assigns indefinite Hold; Shift+H assigns Hold with ten owner upkeeps remaining. Neither spends nor refunds movement. Assigning Hold replaces a terraforming order but retains accumulated work.
- Held units leave automatic selection and no longer prevent Turn Complete. Their movement display and flag availability still use actual movement. A directional move or attack requires explicit activation first.
- Clicking a held unit opens its command panel with **Activate unit**. Activation retains spent movement, the order parameter and accumulated work. It does not apply Former cancellation rollback.
- Space uses the actual conventional Skip event: spend the full movement allowance while retaining the active work order and progress. It does not assign Hold.
- Owner upkeep resets movement and decrements a nonzero signed 16-bit Hold countdown. Zero remains indefinite; reaching zero clears Hold. Word wrap is defined explicitly, avoiding C++ signed overflow.
- Unit7 stores the independent general order and signed countdown alongside the existing state-history fields. Unit4 and Unit6 migrate with no Hold. Records that do not need the new fields retain their previous format; legacy encoding refuses to discard nonempty order state.

The retained general order is distinct from the port's WorkOrder IDs (whose value2 means Road). Preparing commands validates the registered unit, accepted local owner, stable turn and unlocked current tile; it does not trust the original keyboard handler's supplied-selection assumption. The native menu-Hold transition preserves its existing countdown, but an original-style unit context menu is not yet implemented.

Tests cover actual ten-turn upkeep, indefinite and inherited negative counters, malformed codec fields, real world save/restore, next-unit selection, Former Farm2 -> Hold -> restore -> Activate button -> move -> Solar4, and Skip retaining work. The real command-panel callback is used with only DOM construction replaced. Frontend actionability checks keep movement separate from selection eligibility.

The map H glyph, original context-menu layout, Sentry/transport/repair branches, native-life orders and SMAX-specific admission remain follow-ups. This change supplies real Hold authority for the renderer; it does not claim a matched H overlay or complete original unit-order system.
