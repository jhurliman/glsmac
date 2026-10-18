# Installed standard Planet campaign

The Alpha Centauri → The Map of Planet menu route reads the user's installed `maps/planet.MP` through a bounded native decoder and starts an ordinary canonical local campaign. It does not substitute the random-map Standard size or use the showcase `.gsm` adapter. The supported original map is the 80×80, 3,200-tile, horizontally wrapped v5 standard Planet profile. Its original world-size category is 4; geometry alone does not determine category.

## Authority and persistence

The loader preserves the complete original header, 44-byte tile records and coarse tail in a distinct InstalledPlanetV1 resource policy. Tile altitude, force/override bonus markers and landmark/detail/suppression fields feed the measured native resource predicates. The original resource-pattern value is a stored map input, not a byte offset or a draw from the campaign RNG. Bounded decode, unsupported-field validation, cancellation and staging finish before publishing a map.

An installed-map origin is separate from arbitrary map replacement and scenario identity. It joins the existing claimed Start, first Base, starting Unit, movement history, ordinary production/upkeep, workers and campaign save/restore transactions. Restore binds and checks the saved source policy, options and topology before use. Original game data and captured saves remain outside Git.

Standard/Current Rules now have explicit saved A/B/M/N authority, selected independently from the SMAC ruleset profile. Standard replaces the primary low word with `0x1a0e`, preserving the measured preference/high-word/secondary producer behavior. This enables Blind Research and Cooperative Victory and **disables automatic land resources**. Ordinary campaign identity/restriction reset is an explicit separate fresh-campaign operation. Current restores remembered low rules and personality/agenda flags, forcing Blind Research on Citizen. Existing saves without this optional authority retain the documented legacy port manual-research/no-stagnation interpretation; they are not relabeled Standard. See the public behavior receipts for the exact contracts.

## Inspector slice

The existing canonical inspector shows actual owner/name, citizen allocation, HQ/faction facilities, N/M/E intake and demand, stored nutrients/minerals, treasury, current production and the complete saved queue. Change opens the authenticated production picker with scrolling and the existing retool confirmation. Tile clicks use native worker commands. Unsupported governor/priorities, rename/navigation/nerve stapling, help/workshop, support and Psych details are visibly unavailable rather than dead buttons. Queue editing, Hurry and forecasts remain unavailable.

## Deliberate fidelity limits

- The source contour is retained, but displayed height and corner smoothing use an explicit GLSMAC contour adapter. Original climate/lighting geometry, river preparation and region continuation are not reproduced by this loader. No terrain screenshot parity is claimed.
- Starting sites use the port's existing clear-interior-land/ordinary-exit selection, not original geographic balancing or faction placement. The normal roster is one human; this does not add AI factions.
- The saved Standard Blind Research flag is authoritative, but the live research selection/value consumer is a separately owned integration dependency. At this checkpoint the existing manual prompt still appears. Other option consumers are not implicitly certified by storing the producer values.
- Complete growth/production forecasts, ordered support presentation, authenticated rename/navigation, specialist and queue editing, governor/hurry, citizen/riot/Psych, ecological consequences, treaty commerce and nerve stapling remain the inspector backlog. This vertical slice is not the full inspector or 1.0.

## Validation

Focused native checks cover bounded container decode/encode and malformed inputs, loader atomicity/cancellation and unchanged map RNG, source/options/world-header save authority, all-tile installed-asset resource counts, canonical resource/worker/production callers, and campaign restore/next-turn continuation. A parameterized actual claimed-Start fixture covers all seven stock factions × six difficulties on an imported synthetic Planet-format map; installed-asset rendered smoke is separate from those 42 cases.

With the installed original standard Planet asset, all 3,200 source tiles round-trip exactly; 1,374 are land and 1,826 sea. Standard disables automatic land resources: native bonus queries give 3,111 none / 34 nutrient / 28 mineral / 27 energy. Explicit automatic-resource mode gives 3,028 / 87 / 44 / 41. No original asset bytes are committed.

Correctness analysis passed the 17 changed production translation units and affected consumers, then the later Game restore query and ResourceManager restore fix. A focused ASan/UBSan overlay instruments the 11 importer/options/map/startup risk units; nine focused groups passed. Other linked objects reuse the cached native build, so this is not a whole-program sanitizer claim. Restore catalog/no-resource marker defects discovered by rendered load are isolated in PR223 and PR224.

Actual renderer receipts use original installed artwork, the feature binary and its matching scripts, GL backbuffer readback and SDL input events. They do not invoke gameplay callbacks or substitute a debug campaign. Gaia's menu-created campaign reached the inspector, changed production through the real retool dialog, saved via the menu, restored the same map/Base/Unit/stores/queue, changed a worker to Doctor and back through actual tile clicks, and advanced one turn. Restored food 2 → 4 and treasury 11 → 12; Scout Patrol production completed with 2 minerals retained. Evidence images/logs and the captured save are outside Git under the task validation directory. Final presentation-head smoke is recorded below.
