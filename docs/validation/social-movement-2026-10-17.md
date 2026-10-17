# Social and movement integration checkpoint — 2026-10-17

Validated source: `f0b1ee2e7802b984e60c5de1116ae74afea4a8dd` on
`codex/faithful-1.0`. This integration source contains independent work that has
not all landed on main. Each focused PR retains its own review and check gates.

| Check | Result | Elapsed |
| --- | --- | --- |
| Native Windows Debug CTest | 8/8 | 86.67 s |
| Linux ASan/UBSan/LSan CTest | 8/8 | 154.71 s |
| Native honey macOS arm64 Debug CTest | 8/8 | 63.86 s |
| LLVM 15 correctness analysis | 332 production units, zero failures | 251.00 s |

Linux retains leak detection and halt-on-error, with the previously documented
non-PIE workaround. Analysis uses two jobs and excludes the explicitly
Windows-only network implementation. Windows compilation/tests are separate
coverage; they are not a Windows static-analysis claim.

Honey used a new clean checkout under
`/Users/jhurliman/.codex-validation/glsmac-20261017-f0b1ee2e/`, from
11:51:54 through 11:53:55 UTC. No user's interactive checkout was reset.

## What this validates

Owned social state now joins simulation identity, technology and startup
designs. The actual first turn refreshes every admitted owner before turn
publication and entity/game upkeep hooks. Early retained snapshots remain
explicitly unknown; restored values are preserved without replay. The scoped
neutral-model implementation and its original-output boundaries are documented
in [smac-owned-social-state.md](../smac-owned-social-state.md).

Host settings and worlds freeze the raw movement road rate, with Q derived
from that single authority. Oversized movement and predefined-unit profile
frames reject before nested allocation. Separate source comparisons and
negative controls are recorded with those changes; this checkpoint combines
them with the existing gameplay RNG, combat, conventional stats and ground
edge/admission calculators.

The focused publication chain is #147 (social source metadata), #148 (frozen
movement profile) and #150 (owned social continuation), following #136/#141.
Source commits are `78723e35` (social), `1994a8e1` (movement frame cap) and
`7c704b1b` (predefined frame cap). Actual publication trees also have local
CTest receipts, distinct from the integration/platform runs above.

## Limits and next consumers

These tests do not activate live conventional commands, finish a conquest
match, implement disk savegames, or prove the original new-game RNG seed
chain. The ongoing Rover renderer and tagged conventional Unit work are not
part of this pinned checkpoint. The latest human interactive Mac smoke remains
`5cd78418`; these are automated native tests, not another GPU interaction claim.

Next, bind the owned Unit record to fresh stats and exact restored context,
complete the measured Spartan Rover renderer, then connect actual movement
commands to synchronous Unit/tile mutation and owned paired RNG. The verified
local command selector chooses the shared movement stream; original full
startup and load-to-command seed provenance remains a separate open fidelity
requirement. Any interim GLSMAC seed policy must be identified as such.

## Retained receipts

The private manifest is `out/social-movement-integration-manifest.json` with
SHA256 `1f77c8bb8e33d08425423e2e3669b208f6e7ace7a6c9bf6df6edd03da7f9947c`. It records source identity, platform results, binary and
bundle hashes, log locations and analysis scope. Original assets and researcher
reports remain outside Git.

Windows binary SHA256: `4ec32968d7cc3cb9ec2cdf2de68acb46bbc53b4b7da7e2c256b369149a9fe7da`.

Honey binary SHA256: `6347812ec83d062348fe5e719b8182fed3fea3e04e206113cacab8b80f00dedc`.
