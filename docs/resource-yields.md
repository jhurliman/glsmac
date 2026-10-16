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

The base resource-panel formatter now preserves a leading minus sign rather
than placing zero padding before it. Its cell visualization remains bounded,
and its numeric labels show negative and larger amounts exactly. This change
does not expand the range of GSE arithmetic itself or implement new yield rules.

Automated tests exercise actual Tile resource conversion, Base resource callback
validation, the production base-intake script (-2 + 300 = 298), signed integer
boundaries at the callback boundary, and the real resource-panel formatter and
bounded cells. Presentation tests cover unavailable sprites and numeric fallback,
including INT64_MIN without negation or unsigned conversion.

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
$yieldUiOverride = $yieldUiAnchor + [Environment]::NewLine + "return { NUTRIENTS: 0 - 2, MINERALS: 300, ENERGY: 3 };"
[IO.File]::WriteAllText($yieldUiScript, $yieldUiOriginal.Replace($yieldUiAnchor, $yieldUiOverride))
```

Run the built GLSMAC executable with the usual original-asset path/options plus
--datapath $yieldUiRoot --windowed --quickstart. Open a base's resource page.
Verify -2 and 300 labels, the ordinary three-energy icon, base intake labels,
worker reassignment refreshes, reopening the base screen, window resize, and
normal shutdown. Use --memorydebug on a Debug build for the ownership check.
Do not advance turns during this synthetic presentation check.

For comparison, regenerate the override from $yieldUiOriginal with ordinary
values 2/8/3, then all zeros, then negative-only -2/0/0. Restart between variants.
The 2/8/3 case must retain the existing stacked-icon layout and all-zero must
retain the existing no-resource sprite. Keep screenshots/logs with the tested
commit and variant. The temporary override is presentation test data, not an
original-game behavior fixture; never commit it into default/.
