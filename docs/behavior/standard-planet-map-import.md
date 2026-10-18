# RE-SMAC-PLANET-IMPORT-001: installed standard Planet import

Date: 2026-10-18. Scope: the installed SMAC `maps/planet.MP`, version 5,
SHA-256 `24b1a6b018f0aaddde0ecc5712cc302359c872228b0342f0f30b9004670cdb30`.
The complete original map-file loader returns success using memory-backed file
services. Its allocation, geometry, header/tile loading and tile postprocessing
execute. The final map-display reset is supplied. This is a loader and resource
query receipt, not a rendered menu-to-first-command campaign receipt.

The [cases](standard-planet-map-import-cases.json) bind one successful installed
asset load, 6,400 complete resource resolver calls, 9,600 ordinary N/M/E calls
and 9,600 controlled center N/M/E calls. No original asset bytes are distributed.
Existing [bonus resolution](resource-bonus-resolution.md),
[map reset](resource-pattern-initialization.md), [ordinary nutrients](ordinary-tile-nutrients.md),
[minerals](ordinary-tile-minerals.md), [energy](ordinary-tile-energy.md) and
[landmark predicates](base-square-landmark-predicates.md) supply the reused rules.

## Geometry, category and seed

The asset is **80 horizontal coordinates by 80 rows**, with 40 actual tiles per
row and 3,200 tiles. Valid coordinates have matching x/y parity. In row-major
record order, record `i` has `y=i/40` and `x=2*(i%40)+(y%2)` using integer
division. This is not 6,400 tiles. Horizontal wrapping is enabled; vertical
wrapping is not. The native layout scale is 56, the integer square root of
3,200, recomputed at load rather than trusted as an independent geometry input.

**The serialized world-size category is 4, and this loader preserves 4.** A
poisoned prior category 99 is replaced by that file value. Although the map's
dimensions match the generated Standard preset, that preset's category 2 must
not be substituted on geometric grounds. See [world-size identity](world-size-category-and-dimensions.md).
This establishes the accepted loader result; the entire subsequent setup
controller is not newly executed here.

The loaded **resource-pattern offset is 27067**. A prior offset 123 is replaced
by the header's value. Loading does not call the map-reset producer or generate
a fresh offset, and makes no shared/CRT RNG calls. Keep the full signed offset,
including its distinction between zero and nonzero multiples of sixteen. It is
not a general RNG state. The separate automatic-land-resources switch belongs
to campaign rules, not this map's header. Both switch values are checked here;
this receipt does not establish the Standard Rules menu's option producer.

## Decoded terrain input contract

Import separate semantic fields for altitude band, moisture, rockiness,
item/improvement flags, landmark memberships and shared landmark detail. The
format-specific mapping is in the private source audit. Preserve the original
contour and climate-code values as separate provenance; a rendering elevation
conversion must not become the authoritative gameplay altitude or rainfall.

| Decoded field | Meaning required by the checked consumers |
| --- | --- |
| Altitude band | 0 trench, 1 deep ocean, 2 shelf, 3..7 land. Land is band >=3. Shelf-only improvements require exactly 2. Bonus deposits are rejected below 2. |
| Moisture | 0 arid, 1 moist, 2 rainy. The installed file contains only 0..2. The encoded boundary 3 is not admitted as normal Planet terrain. |
| Rockiness | 0 flat, 1 rolling, 2 rocky. Installed file contains only 0..2. Encoded 3 remains outside this asset's admitted profile. |
| Climate code | Distinct low climate subfield; the file contains 0..2. It is not moisture. Its full display/climate meaning is not established by this receipt; do not rename it using an assumed temperature scale. |
| Contour | Distinct saved value, observed 0..125; not the resource-yield altitude band. Retain it for later terrain/sea-level rebuilding. No conversion into GLSMAC vertex heights is proved here. |
| Fungus / monolith / river | Pass the actual flags to the existing resource routines. Fungus can suppress a deposit's contribution without removing that resolved deposit. Land river energy is separate from landmark/resource bonuses. |
| Forced bonus presence | Bypasses automatic presence/phase gates, but still fails below shelf. It does not by itself force the marked type. |
| Type override / energy marker / mineral marker | Preserve separately and use the existing resolver's precedence. A marker alone is insufficient to infer a deposit. The fallback without either type marker is nutrients. |
| Landmark detail | One shared byte for all memberships. Interpret its high bit as suppression and retain the signed-detail comparison used by Crater/Planet; do not regenerate detail from a label, distance or visual elevation. |

There are **14 forced-presence records**, but automatic deposits also occur on
records without that flag. Checking only the forced markers loses most bonuses.
With the loaded offset, the original resolver returns:

| Automatic land resources | None | Nutrient | Mineral | Energy |
| --- | ---: | ---: | ---: | ---: |
| Disabled | 3,111 | 34 | 28 | 27 |
| Enabled | 3,028 | 87 | 44 | 41 |

These counts include qualifying shelf deposits and are independent of whether
fungus or another yield branch subsequently uses their contribution. The
all-tile resolver call sequence leaves terrain and RNG state unchanged.

## Landmark preservation

Preserve all memberships and the detail byte, not only the eight memberships
that directly add N/M/E. These direct bonuses are boolean per resource: an
overlap does not add multiple landmark points.

| Membership | Nutrient | Mineral | Energy |
| --- | --- | --- | --- |
| Monsoon Jungle | Unsuppressed land | — | — |
| Freshwater Sea | Sea, even with suppression | — | — |
| Garland Crater | — | Unsuppressed, detail <9 | — |
| Mount Planet | — | Unsuppressed, detail <9 | Unsuppressed, detail <9 |
| Nessus Canyon | — | Unsuppressed | — |
| Uranium Flats | — | — | Unsuppressed |
| Geothermal Shallows | — | — | Unsuppressed |
| Pholus Ridge | — | — | Unsuppressed |

The installed Planet profile contains Crater, Planet, Jungle, Uranium,
Freshwater, Geothermal and Pholus memberships; it has no Nessus membership.
Other membership flags occur and must survive decoding even though the checked
yield predicates give them no direct bonus. The shared field also contains a
dirty/bookkeeping bit on every tile. Do not mistake that bit for an unknown
landmark that requires rejecting the installed map. Conversely, do not treat
every bit in that field as independent membership: the high byte is detail.

## Loader cleanup and unsupported boundaries

The map-file wrapper clears the per-tile visibility byte and base/unit presence
flags after loading, rather than manufacturing Base or Unit objects from map
records. On this installed file those fields already have the cleared values,
so **all 3,200 loaded tile records are byte-for-byte unchanged**. This is not
evidence that arbitrary scenario maps need no cleanup.

The loader also reads a 128-byte coarse map. It is not unexplained padding.
This task does not map its gameplay consumers. Keep its provenance or explicitly
rebuild the port's corresponding metadata; do not interpret it as more tiles.

The native Map of Planet menu subsequently invokes terrain/climate/river/region
preparation before the setup routine. That complete continuation is outside this
new execution receipt. Raw-loader field identities and the checks above must
not be advertised as a full original first-command world. Preserve contour,
sea-level and climate parameters so later producers can be integrated without
inventing neutral inputs. This contract does not prove original geographic
faction placement, landmark names/registration UI, arbitrary `.MP`/scenario
support, Huge Planet, SMAX, malformed-file acceptance, live climate changes or
full campaign save/resume scheduling.

For the bounded installed-map importer, preserve opaque item bits and unrelated
saved fields, and expose only the proven decoded inputs to yield calculations.
The installed file contains additional item bits whose full roles are not
named here; do not strip them, assign guessed improvements or silently use them
as yield modifiers. Existing direct yield routines ignore those additional bits
in this checked profile. Any later gameplay consumer needs its own supported
mapping or explicit unavailable boundary. A display approximation that drops
landmark detail or replaces automatic deposits with explicit marker lookup
does not satisfy this contract.

The N/M/E composition uses a declared neutral faction/Base query context:
installed resource rows, no cap technologies, economy/PLANET zero, fungal
capabilities 1/0/0, no facilities/projects/events/dust/solar event, and automatic
land resources enabled. Ordinary queries preserve the imported records. Center
queries temporarily overlay a valid Base marker/owner and restore it after each
triple; they are not 3,200 actual Base-founding operations. They establish field
interpretation against complete routines, not a particular faction's opening
yields. No production port code, original asset, existing evidence receipt or
standing research goal changed.
