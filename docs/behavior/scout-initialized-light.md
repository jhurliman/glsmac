# Initialized ordinary Scout: separate color and shadow lights

The original ordinary Scout uses **separate color and shadow light vectors**. Running its actual SYSTEM reader for `VEH_LIGHT_DIR_MAP=(6,4,15)` changes the color raster but leaves the entire222-pixel shadow mask identical to the default-light control. [Numeric cases](scout-initialized-light-cases.json) record native writes, vectors, copy boundaries and raster hashes.

The map-light loader reads integer6/4/15, converts them to binary32 and calls the native vector normalizer. The resulting color vector is `(0.36050504446029663, 0.24033670127391815, 0.901262640953064)`, bits `3eb8941e/3e761ad3/3f66b926`. The ordinary helper copies this vector into scene+0x48.

Independently, that helper supplies fixed shadow direction bits `bf13cd36/3f13cd36/3f13cd36`, approximately `(-0.57735002,+0.57735002,+0.57735002)`. It transforms this fixed vector by the inverse camera basis and stores it at scene+0x54. In this flat camera the projected direction remains `(-0.5773500204086304,-0.21090613305568695,-0.7887869477272034)` under both color-light inputs. Keep the existing [shadow-camera rule](scout-shadow-camera.md) for ordinary activation; do not substitute normalized SYSTEM map color light into shadow projection.

| Source raster at final-copy entry | Default color light | Initialized SYSTEM color light |
| --- | --- | --- |
| Shadow nonzero pixels | 222 | 222 |
| Shadow SHA256 | `3c26c4bbd927a4752e779a90f378c4ef9aade2b86755ca4d2aeb69f3aa4e9b20` | Identical |
| Color non-key pixels | 403 | 403 |
| Color SHA256 | `fa26b8ac45a18a91c2027fd619114f2e16e2bcca1754da4794716ccd7f57ab0d` | `c97309a3f81d7444cfe95fc5f662b423935968f6c8b944574967ea0e6dfda7df` |

Each execution draws59 original Scout objects and performs222 shadow-copy stores followed by403 color-copy stores through the native compositors. The rasters are sampled **at copy entry**; temporary buffers are cleared after use. The configured light is produced by the original file reader/token conversion/normalizer fragment, with inherited FILE services. It is not injected from a captured vector. This is a finite flat-camera facing2 ordinary-helper control, not a fresh OS launch or a new slope/perspective claim.

Verification: `tmp/reference/check_scout_initialized_light.py --verify-only`. Frozen evidence: `tmp/reference/scout-initialized-light-evidence-20261019/manifest.json`. Full owned-asset rasters remain private.

## Port regression

The installed-asset regression exercises the actual default and initialized
CvrScoutRenderer entries. Both complete256-square color buffers match independent
native digests (403 non-key pixels each); initialized and default full shadow
buffers are identical and match the native222-pixel mask. Expected64-bit test
digests were derived from private native rasters only after verifying each full
SHA256 against this receipt. No captured raster is a production input.

Eight affected initialized/shadow test groups pass in the cached build with the
owned assets. The immutable original checker also passes. No production logic
changed, so unchanged production analysis/sanitizer evidence is reused. This
resolves the initialized-light question; it does not activate map shadows before
destination palette-table production and indexed target composition are ready.
