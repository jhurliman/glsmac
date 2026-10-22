# Manufactured High Morale movement

The ground factory and design resolver admitted High Morale designs, but the
edge-cost helper rejected any ability word. A manufactured unit consequently
failed its ordinary move command before spending movement or changing position.

The cost helper now accepts passive High Morale (`0x4000`) while retaining its
existing checks for other ability bits. The [original executable
receipt](behavior/high-morale-movement-edge-cost.md) establishes that this bit
changes none of the checked Infantry/Speeder edge costs. Effective elite morale,
not the ability bit itself, adds the unit's extra movement point. Basic design
allowance remains the input to the positive-PLANET fungus cost branch.

Validation covers 32 literal native cost expectations and the existing terrain,
infrastructure, precedence and invalid-input checks. A live regression reproduces
the old rejection, then manufactures a High Morale guard with its bonus morale,
moves it into forest through the queued command, and compares uninterrupted
world continuation with the same command after actual saved Start. The move
spends six native movement units (two points), preserving home, morale and RNG.
The 15 native newborn cases and existing saved Workshop manufacture/move case
also pass. Targeted correctness analysis passes for the changed cost helper and
the live movement translation unit. Ownership and codecs are unchanged, so the
preceding factory/restore sanitizer results remain applicable.

This connects ordinary movement for already admitted designs. The separate
fungus-encounter and Scout-only combat admission boundaries remain in place;
it does not establish complete movement/combat support for every Workshop draft.
