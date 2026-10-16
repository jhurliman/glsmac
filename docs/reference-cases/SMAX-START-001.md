# SMAX-START-001: reference startup on aleph0

Status: **observed compatibility discrepancy**, not a gameplay-rule fixture.
Observed 2026-10-15, America/Los_Angeles, on Windows 11. The user identifies the
installed **SMAX PRACX** shortcut as previously working. This observation records
the current failed attempts without treating them as universal incompatibility.

## Identity

GOG Planetary Pack installation identified in [reference validation](../reference-validation.md).
Executable: `terranx_PRACX.exe`, SHA-256
`84432e3a1465a05f32b5bb70693f5c4099661d5c1511dbbf27233b4890071b1c`.

Other input fingerprints:

| File | SHA-256 |
| --- | --- |
| `alphax.txt` | `bb01e3ac23793da0a46713f4851d65e63b599c24aef8300ffe45e7e57b332c42` |
| `gaians.txt` | `6c25fe3492f83a9ac3a8da7c7e2ddc0a62134c46e244f8872e1e9067b2564cf2` |
| `maps/planet.mp` | `24b1a6b018f0aaddde0ecc5712cc302359c872228b0342f0f30b9004670cdb30` |
| `ddraw.dll` | `2f1399ef5e6cdba02495fbb66c731924c9a7b0b40b43b8b496dfb18993328039` |

The metadata-only post-attempt manifest is kept locally at
`tmp/reference/aleph0-gog-after-startup-20261015.json`, SHA-256
`e7ef0e123d085730932b0a5a78483350e5127391fc9e779239446c42fc6fabe3`.
The original inventory is preserved separately. Configuration changed through
normal game setup; the post-attempt manifest identifies the resulting file.
PRACX was selected explicitly. The presence of the DDraw DLL does not by itself
prove whether it was loaded.

## Procedure and observations

1. Launch SMAX PRACX. The second attempt used the actual installed `.lnk` from
   File Explorer: target `terranx_PRACX.EXE`, no arguments, working directory
   equal to the installation directory.
2. Choose Start Game, The Map of Planet, Librarian, Play With Standard Rules.
3. Select the original seven factions and Gaia's Stepdaughters; keep Deirdre's
   default name. This is still a **SMAX** campaign, despite the original factions.
4. Accept Planetfall. The map and initial base-naming dialog appear, showing
   Mission Year 2100 and energy 10. Starting positions differ between attempts;
   no seed or reusable save was captured.
5. Accept the default base name, Gaia's Landing. The process exits unexpectedly.

Windows Application Error event 1000 records access violation `0xc0000005` in
`terranx_PRACX.EXE` at offset `0x001269e4` at 23:46:37 and 23:50:28 local time.
The second event confirms that using the actual shortcut did not resolve this
specific failure. No base economy, pod consumption, or turn result was observed.

A separate direct launch of plain `terranx.exe` failed at startup at 23:42:20,
with event 1000 reporting `0xc0000005`, offset `0x001f7e90`. This does not test
`terran.exe`, the separate SMAC ruleset, or every supported launch environment.

## Next evidence needed

Reproduce the user's successful launch conditions, then capture a private
save immediately before a controlled gameplay action. Check random-map setup
and an existing known-good save separately to distinguish a map/setup-specific
failure from a general runtime failure. Do not change gameplay binaries or
silently substitute patched rules to obtain expected rule values.

GLSMAC coverage: none. This case tracks reference availability only; it does
not establish game-rule fidelity or satisfy a completed-game release gate.
