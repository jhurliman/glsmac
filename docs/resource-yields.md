# Signed resource values

Resource callbacks return signed GSE Int values. ResourceRelated still rejects
unknown resource IDs, non-object results, and non-Int amounts; omitted resource
IDs still default to zero. Native Tile::resources_t now carries int64_t so the
renderer receives exactly the same values as the script-facing Tile and Base
callbacks. Yield and growth calculations are unchanged.

The usual tile display uses the original exact sprite assets unchanged. If any
nonzero amount has no exact sprite, the tile switches to bounded rows. Each
exceptional amount has one identifying icon and its full signed decimal value;
ordinary amounts in the mixed view keep their exact stacked sprite. A defined
resource with no icons uses its ID alongside the number. Zero values remain
hidden. A missing resource definition remains an error, not a numeric fallback.

Fallback labels use the existing InstancedFont text-texture implementation.
GetTextTexture creates a fresh texture; Widget takes ownership and deletes it
on Clear. Its new fixed-size option prevents resize from erasing those pixels,
and defaults to the previous resize behavior for every existing caller. There
is no global cache indexed by amount and no allocation proportional to amount.

The base resource panel calls ui.format_resource_balance(profit, loss,
total_cells), a state-free presentation helper. It formats the original signed
amounts and their exact difference, including magnitudes through 2^64 - 1. It
orders the operands before unsigned subtraction, so it never negates INT64_MIN
or evaluates an overflowing signed difference. Ordinary labels keep their
three-digit zero padding and surplus sign; negative input amounts keep their
leading minus sign.

The helper bounds loss cells to clamp(loss, 0, total_cells), and profit cells to
the positive difference bounded by total_cells. The panel still paints loss
cells first when those ranges overlap. It returns counts rather than allocating
cells; the panel allocates its existing 58 cells per resource regardless of the
amounts. Invalid types, arity and negative cell counts are rejected. The shared
native callable factory is used by both UI::Wrap and headless panel tests.

Automated tests exercise actual Tile resource conversion, Base resource callback
validation, the production base-intake script (-2 + 300 = 298), signed integer
boundaries at the callback boundary, and the real resource panel with the native
formatter. Endpoint cases include MIN/2, MIN/0, MIN/MAX, MAX/MIN and equal
endpoints, checking exact strings, bounded cells, and loss precedence. Tile
presentation tests also cover unavailable sprites without negating INT64_MIN.

This is presentation safety, not a wider gameplay arithmetic implementation.
Other nutrient/economy calculations remain unchanged. The small fixed tile
preview fits long labels by shrinking them, so 20-digit values are not legible
at ordinary tile size; the existing summary text layout can also be too narrow
for extreme labels. These layout limits do not change the values or formatted
strings, and no full-range gameplay arithmetic or layout guarantee is implied.

## Native UI check with disposable data

Use a unique temporary copy of the open-source GLSMAC_data directory. Do not
modify the original-game installation or the checkout's default scripts. In
PowerShell, from the GLSMAC checkout:

```powershell
$yieldUiRoot = Join-Path ([IO.Path]::GetTempPath()) ("glsmac-yields-" + [guid]::NewGuid().ToString("N"))
Copy-Item -LiteralPath (Join-Path $PWD "GLSMAC_data") -Destination $yieldUiRoot -Recurse
$yieldUiScript = Join-Path $yieldUiRoot "default/resources.gls.js"
$yieldUiOriginal = Get-Content -LiteralPath $yieldUiScript -Raw
$yieldUiAnchor = "game.get_tm().on('get_tile_resources', (e) => {"
if (-not $yieldUiOriginal.Contains($yieldUiAnchor)) { throw "Yield callback anchor missing" }
$yieldUiOverride = $yieldUiAnchor + [Environment]::NewLine + "return { NUTRIENTS: e.tile.get_base() != null ? 2 : 0 - 2, MINERALS: 300, ENERGY: 3 };"
[IO.File]::WriteAllText($yieldUiScript, $yieldUiOriginal.Replace($yieldUiAnchor, $yieldUiOverride))
```

Run the built GLSMAC executable with the usual original-asset path/options plus
--datapath $yieldUiRoot --windowed --quickstart --ruleset smac (or smax).
Open a base's resource page.
Verify -2 and 300 labels, the ordinary three-energy icon, base intake labels,
worker reassignment refreshes, reopening the base screen, window resize, and
normal shutdown. Use --memorydebug on a Debug build for the ownership check.
The positive center allows the automatic first turn to initialize population
on builds predating the separate empty-base starvation guard. Ordinary worked
tiles still expose -2. Do not advance further turns during this synthetic
presentation check.

For comparison, regenerate the override from $yieldUiOriginal with ordinary
values 2/8/3, then all zeros, then negative-only -2/0/0 on non-center tiles.
Retain positive center nutrients for the negative-only variant. Restart between variants.
The 2/8/3 case must retain the existing stacked-icon layout and all-zero must
retain the existing no-resource sprite. Keep screenshots/logs with the tested
commit and variant. The temporary override is presentation test data, not an
original-game behavior fixture; never commit it into default/.
