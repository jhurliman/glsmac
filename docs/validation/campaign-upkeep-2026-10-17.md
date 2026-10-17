# Campaign upkeep integration — 2026-10-17

The working integration advances the ordinary SMAC campaign through actual
production selection, manufacture, Colony movement and second-base founding.
Current SUPPORT drives mineral-shortage removals, including an unpublished
newborn; non-last-base starvation repairs homes and continues surviving Bases
once. This is not a complete conquest match or a golden release checkpoint.

## Checked source and scope

| Source | Check | Result |
| --- | --- | --- |
| `44b62007b4edd49046c2340ef5b1111cae0ad983` | Cached Linux Clang Debug, affected native filter, allocation tracking | 112 groups pass; normal exit |
| `0e25ab113d4d5a0425ba9d7414ff7a3d3b248b26` | Three complete Base-screen groups after Unit-retirement change | 3 pass |
| Same binary, script fixture corrected at `2250155540dbc9cceb3d3082ab0e21a8d8f067f5` | Actual bottom-bar modules, native wrapper retirement and same-tile survivor | 2 groups pass; normal exit |

The 112-group filter covers live founding and prepared removal, production
selection/switching/manufacture, support shortages, Colony growth, first-Base
upkeep, starvation, energy/economy and research submission, resource intake,
world/simulation codecs, and real Client download/restore fixtures. These are
native engine and scripted UI tests. They do not establish disk-save dialogs,
network convergence, GPU rendering or original executable equivalence.

Targeted LLVM 15 correctness analysis reports zero failures: five production
units for shortage/selection (6.60 seconds), four for starvation/controller/Base
retirement (4.50 seconds), and the frontend Unit manager after its retirement
fix (2.87 seconds). Some units overlap. Other unchanged component receipts are
retained; no full scan was repeated. No new Windows, macOS or full sanitizer run
is claimed. Platform checks remain tied to platform-sensitive changes, playable
milestones and release checkpoints under [the CI policy](../ci-validation.md).

## Behavior and remaining work

- Production picker choices use the actual owner's stored difficulty and reject
  stale confirmations before effects. Scout, Former and Colony choices can
  manufacture through the native upkeep path and retain their queue/baseline.
- [Running Colony founding](../smac-running-founding.md) consumes a real Pod and
  restores the new Base, grants, workers, homes, territory and knowledge.
- Current SUPPORT and ordered paid-unit losses preserve the original counter
  behavior; recalculation does not post a second turn of income.
- [Non-last-base starvation](../smac-starvation-removal.md) rehomes Units and
  retires Base/Pop wrappers. UI navigation, matching Base popups and same-tile
  unit rows are removed when their objects retire.
- [Saved economy](../smac-live-economy.md) owns treasury, rates, pending debit,
  research fraction and treaty inputs. Actual energy queries and private staged
  posting are checked. Default turn posting awaits the required research choice
  and discovery continuation; it is not advertised as functioning research yet.

The world codecs at this boundary are World28, Global14, Simulation9,
ResourceRules3, BaseManager10, UnitManager4, Knowledge3 and Technology1.
Initial failures exposed incorrect test difficulty/HQ assumptions, outdated raw
transport framing and GSE fixture syntax. Each was corrected; the final results
above replace those failed attempts. The later Unit-retirement review fix was
checked only in its affected UI groups after the broader gameplay run passed.

Research choice/discovery and Former orders are the next implementation slices.
Last-base disposition, broader founding, combat/capture/conquest, and full SMAX
rules remain separate work. Original assets and the researcher's uncommitted
evidence are excluded from this integration commit.

## Retained logs

Logs are under `/home/jhurliman/src/glsmac-rover-runtime/` on aleph0 Ubuntu/WSL.

| File | SHA-256 |
| --- | --- |
| `campaign-final-build.log` | `d11fbe164b16b5f3ff63d0e8f712dc0b3f5012c275575abfb35272450330bf16` |
| `campaign-final-tests.log` | `d76dd871faa811d6fdd0368e63129e0dd7e4b33da52e124fb7897bd7c75b2504` |
| `unit-retirement-fixed-tests.log` | `ef038b46da095a84ed48c8bf623171f845c50b889ec315e117c1831d568a5052` |
| `unit-retirement-analysis/summary.json` | `ad835be8653424543c97c2f3857868821d6244543d2df8c8f1deca33d6bfdf3f` |
