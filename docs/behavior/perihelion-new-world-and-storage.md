# Perihelion across new-world setup and ordinary save/load

2026-10-18, stock SMAC. **Accepted fresh-world initialization clears active
perihelion before the first faction initializer. Ordinary save/load restores
the saved active or inactive state.**
[Literal controls](perihelion-new-world-and-storage-cases.json).

The tested setup entry starts with turn90 and active perihelion. It retains
that state while constructing the first difficulty dialog and at the first
difficulty Cancel decision boundary. In the accepted difficulty3/faction1/name
path, perihelion is still active at whole-new-world parent entry, then inactive
at first faction-initializer entry, turn0. Clearing therefore belongs to the
accepted fresh-world transition; merely entering this tested setup dialog does
not clear it. This does not claim a complete Cancel cleanup or live menu loop.

Two separate actual whole-new-world prefixes start with perihelion off/on and
previous turn17. Both leave it off before the first owner1 initializer. The
native parent masks its campaign-state word and excludes the perihelion bit.
The resumed fresh child does not re-enable it. A nonzero-turn faction
initializer prefix instead preserves an active bit. Do not defer a fresh-world
clear until the next calendar maintenance, or infer a clear from cold zeros.

Two complete native ordinary save/load file-body roundtrips save inactive and
active states, poison the live bit to its opposite, then restore the saved
state. An unrelated retained state bit also restores. These use a declared
zero-Base/zero-Unit storage profile and memory-backed FILE/OS/heap services;
the save/load results are native, not supplied. This is a campaign-state
authority, separate from the serialized map-header sea/climate inputs.

Validation comprises two setup paths, two new-world/resumed-child prefixes,
one nonzero-turn child prefix and two ordinary file roundtrips. Initializer
prefixes stop at established boundaries; full two-game frontend replay,
all faction initializations and arbitrary scenario/network paths are excluded.
Existing [global maintenance order](perihelion-global-turn-order.md) and
[calendar transitions](perihelion-schedule.md) remain unchanged. Standing
research remains paused; SMAX parity is unestablished for this lifetime slice.
