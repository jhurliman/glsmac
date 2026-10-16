# Conventional-unit rendering: evidence and next slice

Read-only audit, 2026-10-16. Runtime and CVR source references below were
inspected at `20a643a35ac1163a30b20604350fcc4627863d4c`; the unit-table resolver
is also present in the audit checkout at `09863f011f6f1dd8899cc2d5c89f8a12d1da70f1`.
This is a proposed implementation sequence, not a format specification or a
claim that GLSMAC can already render a conventional unit.

The next bounded PR should decode the observed **simple and grouped geometry
records into owned values**. A simple-record-only implementation would leave
three objects out of the likely infantry asset. Original-renderer evidence
for model composition, coordinates and colors is a separate prerequisite for
calling the result a faithful Scout Patrol.

## What is already connected, and what is missing

[UnitDesignResolver](../src/loader/txt/UnitDesignResolver.h) returns an owned
record and copied chassis, weapon/package and defense metadata, retaining the
explicit `smac`/`smax` identity and source row. The actual armor table is named
`#DEFENSES`. It does not select a reactor or map metadata to art. The installed
tables resolve these exact component references in both rulesets:

| Unit | Chassis key | Weapon/package key | Defense key | Plan metadata |
| --- | --- | --- | --- | ---: |
| Colony Pod | Infantry | Colony Pod | Scout | 8 |
| Formers | Infantry | Formers | Scout | 9 |
| Scout Patrol | Infantry | Gun | Scout | 3 |

Weapon/package row indices differ between the two rulesets. Neither these
indices nor the unit `icon` field establish a CVR filename. `#UNITS` has no
reactor column or asset path. The existing [resolution contract](unit-component-resolution.md)
and its read-only 111-reference probe cover metadata, not playable designs.

[CvrContainerReader](../src/loader/cvr/CvrContainerReader.h) and
[CvrSceneReader](../src/loader/cvr/CvrSceneReader.h) already produce owned
chunks, ordered objects and frame arrays. Geometry remains an opaque byte
vector. Frame locations, matrices and flags are retained without applying
coordinate, visibility or attachment semantics. See the separate
[envelope](cvr-container.md) and [scene-record](cvr-scene-records.md) contracts.

The live unit path still accepts only `DT_STATIC` and `RT_SPRITE`:

- Backend [Render.cpp](../src/game/backend/unit/Render.cpp) serializes only
  sprite rendering; [UnitManager.cpp](../src/game/backend/unit/UnitManager.cpp)
  exposes only the existing static/sprite definition path.
- Frontend [UnitDef.cpp](../src/game/frontend/unit/UnitDef.cpp) constructs and
  returns sprites. [Unit.cpp](../src/game/frontend/unit/Unit.cpp) uses them in
  Show, Hide, movement updates and selection-panel mesh/texture generation.
- [Instanced](../src/scene/actor/Instanced.h) can already instance a Mesh or
  Sprite, and [Render](../src/types/mesh/Render.h) can store positions, UVs,
  tint and normals. Those capabilities do not establish how the original
  voxel renderer projects, shades or covers pixels.

## Installed-byte evidence

Only metadata was read from the user's installed GOG files. No original asset
payload was copied into the repository. The independently recomputed inventory
manifest matches the existing CVR audit:
`96f95e27c1677834d7b8e4c4e2c17d48095106dcbcc09827ce9849d137653f50`.
It uses the canonical filename/hash representation documented in
[cvr-container.md](cvr-container.md).

There are 140 files, 536 scene objects and 440 geometry-bearing objects.
The new metadata survey finds 340 simple geometry payloads (flags 0 or 8),
98 grouped payloads (flags 2 or 10), and two excluded variants (flags 1 or 3).
The following observations hold for all 438 simple/grouped candidates:

- The candidate header is 42 bytes: five four-byte fields, three vectors of
  three signed 16-bit components, then a uint32 count. The third four-byte
  field decodes to a finite positive binary32 value, observed from 2 to 5.
  Field names such as scale, origin or pivot are not established by this audit.
- The second field equals payload length. The fourth and fifth equal payload
  length and payload length minus one. All payloads end in five zero bytes.
- Grouped records have four three-component signed-16 vectors followed by a
  uint32 record count and uint32 group flag. All **557 observed groups** use
  flag zero and this 32-byte header. A suggested 36-byte variant in community
  code is not evidenced by these files.
- Each following record is three bytes. Its first byte's upper five bits
  range from 0 through 26. When geometry bit 8 is set, every group has exactly
  one code-26 record, at the end; otherwise no code-26 record occurs.
- Simple headers count all records, including that terminal record. Grouped
  geometry headers count the sum of records **excluding** terminal records.
  Group counts include their own terminal record. A uniform count comparison
  would incorrectly reject one of these observed profiles.

These are structural observations, not proof of a direction table, angular
normal encoding, palette indexing, world units or the original malformed-input
policy. The two excluded payloads are `VHTTp` in `VHTTp.cvr` (flags 3) and
`VLights` in `VLIGHTS.cvr` (flags 1); support must not silently skip their data.

| Candidate file | Embedded source-name bytes | Objects / frames | Observed geometry |
| --- | --- | --- | --- |
| `VI.cvr` | `INFANTRY.MAX` | 58 / 17 | 55 simple, 3 grouped |
| `Vw00.cvr` | `WEAPONS.MAX` | 1 / 17 | 1 simple |
| `VT.cvr` | `TERRAF.MAX` | 1 / 1 | 1 grouped |
| `Vwntu.cvr` | `TERRAF.MAX` | 14 / 1 | 13 simple, 1 grouped |
| `ACOLPOD.cvr` | `MB_Alien_Pod2.max` | 2 / 15 | 2 grouped |

The grouped `VI.cvr` objects are named `DUDEsuit06`, `W-Body-T04` and
`W-Body-B04`. Source names and matching frame counts make these useful
investigation candidates; they do not prove that Scout Patrol is `VI` plus
`Vw00`, which parts to attach, or matching frame indices. In particular,
`ACOLPOD` is not established as the human Colony Pod asset.

Local evidence is retained in the audit checkout under
`out/validation-cvr-render-plan/`: `survey_geometry.py` and
`geometry-metadata.json`. The script reads the installed files, traverses the
already documented nested records, and emits only hashes, names, counts and
header metadata. It does not call the original renderer. Earlier container
and scene probes remain under `/home/jhurliman/src/glsmac-cvr-container/` and
`/home/jhurliman/src/glsmac-cvr-scene/`.

## Proposed next PR: owned geometry-record decoder

Add `loader::cvr::CvrGeometryReader::Read(const std::vector<uint8_t>&)` for the
payload already owned by `CvrSceneObject::voxel_data`. Return `CvrGeometry`
by value, with the raw header fields, ordered groups and owned three-byte
records. Group metadata should retain its four signed vectors. Preserve all
three bytes of terminal records too; expose the observed code as a derived
bit field, not an invented vertex or established normal. Use neutral names
for fields whose rendering meaning remains unknown.

Support flags 0, 2, 8 and 10 only. Reject flags 1/3, unknown bits, unknown
group flags, unexpected terminal codes, malformed count/extent relationships
and unsupported tails with an explicit `std::invalid_argument` prefix. Finite
positive scalar and positive record-count requirements would be documented
GLSMAC profile restrictions; they are not universal CVR validity rules.
Retain signed vector components without imposing an unproved geometric bound.

Validate the complete input before allocating result storage, matching the
existing readers. Bound a record count by remaining bytes divided by three
before calculating offsets or resizing; never reserve using an unchecked
count. Validate late groups too, so no partially usable geometry escapes.
Keep source order, repeated records, signed component endpoints and original
float bits. Returned values must survive destruction/mutation of the source
scene and independent copy/move. No GC objects, renderer pointers or assets
are needed for this decoder.

The PR should contain synthetic native cases for both count conventions,
multiple groups, each observed code, terminal placement, signed fields,
finite/invalid scalar values, truncated prefixes, huge counts, unsupported
variants, late bad groups and lifetime/copy/move. Its installed-file Release
probe must call the production readers and report **438 accepted geometry
payloads plus 2 explicit unsupported variants**, not 140 fully renderable
files. All 59 geometry payloads in `VI.cvr` and `Vw00.cvr` should pass this
profile. Run the native tests under ASan/UBSan/LSan and focused static analysis;
use a copied-source count-validation negative control. No extra full-suite CI
job or original payload fixture is needed.

This PR deliberately does not integrate a live unit. It creates the bounded
owned input for a renderer without committing uncertain visual semantics to
backend definitions or snapshots.

## Evidence required before the first faithful Scout

In parallel with the record decoder, obtain original-executable evidence for
one named Scout design, separately identifying SMAC and SMAX executable/hash:

1. Trace the actual design-to-model selection and attachment/composition path.
   File-open observations alone are insufficient if the program preloads all
   models. Record selected component keys and model/object/frame choices.
2. Establish step-code deltas, the initial point of each group, scalar/vector
   meaning and application order of frame location/matrix. Check representative
   simple and grouped objects against the original routine, not just bounds.
3. Establish color/material lookup, faction colors, packed normal interpretation,
   transparency, depth coverage and projection. Then implement the smallest
   palette/material profile the selected assets actually require.
4. Capture repeatable original outputs for fixed facing, frame and faction.
   Compare the production decoder/render path at matched dimensions, including
   silhouette, component alignment and colors. Keep redistribution-sensitive
   captures local; commit synthetic test inputs and reproducible metadata.

The consulted reverse-engineering author's
[CVR-Colorizer source, commit 1174833e](https://github.com/uaEquals42/CVR-Colorizer/blob/1174833e288ad39f5cf03654344a23abb8b6fc3b/src/CVR.py)
and contributors' [Caviar notes](https://alphacentauri2.info/wiki/Caviar.html)
are leads for these checks. They describe third-party interpretations, include
uncertain geometry/color handling, and do not provide original-game equivalence
evidence. No GPL source or tables from that tool are copied into this plan or
proposed implementation. The structural survey was independently written.

## Renderer ownership and the first vertical slice

After those checks, add an explicit render descriptor/mapping for the verified
design. Keep ruleset/catalog identity separate from installation type and
filesystem case handling. Shared filenames do not prove shared SMAC/SMAX
selection rules. Model references must resolve through the existing local
ResourceManager path, with a clear missing/unsupported-asset error.

A narrow loader/cache can own decoded scenes, geometry and generated textures.
The existing [Mesh actor](../src/scene/actor/Mesh.cpp) deletes its mesh and data
mesh, but borrows its texture. Therefore use a single documented owner for
each cached texture and hand off each tracked mesh to exactly one deleting
actor; do not borrow from a temporary decoded scene or share a raw owned mesh
between actors. Test repeated create/destroy and scene teardown with the real
memory tracker. Any GPU/cache handle must have a lifetime independent of unit
snapshot records, which should carry validated descriptors/identity only.

Two implementation routes remain possible: original-style rasterization into
cached facing/frame textures, or native mesh/voxel rendering. Choose after the
projection/shading evidence; generating cube triangles and arbitrary lighting
would not by itself reproduce the original. The existing sprite map/preview
path makes cached frames a smaller integration candidate, not an established
fidelity decision. A diagnostic viewer may help compare decoded geometry but
must remain clearly separate from a golden-game claim.

The first live slice should show the verified Scout in both map and selection
preview, preserve selection/badges, move it, and restore the same descriptor
through actual unit snapshots. Extend the render codec's explicit version and
validation when adding a new descriptor. Do not silently convert unresolved
reactor, movement, cost or combat metadata into a playable design; establish
those particular rules separately if the live slice needs them.

Colony Pod and Former then follow as separate vertical slices: verified art
mapping plus authoritative role/action state; colony founding and consumption;
former work targets, progress and completed terrain effects; production and
turn integration. Each needs real event/restore coverage. Parsing their names
and packages does not implement those mechanics or establish golden 1.0.
