# Rover unit presentation adapter

Explicit `cvr_spartan_rover` unit definitions now use the measured
[Spartan Rover renderer](cvr-spartan-rover-renderer.md) through the existing
UnitDef, scene sprite, and unit-preview path. This is an idle presentation
adapter, not automatic conventional-unit spawning, movement, or combat.

The shared immutable descriptor retains its `CvrScoutRender` source name and
adds a fixed composition enum. Scout is profile0 / `RT_CVR_SCOUT`; Spartan
Rover is profile1 / `RT_CVR_SPARTAN_ROVER`. Existing Scout encoded bytes stay
unchanged. The outer render tag and inner profile must agree when decoding.
Unknown profiles, oversized frames, invalid views and invalid placement reject.
No serialized asset path, arbitrary composition, or faction filename is added.

Both definitions require explicit facing, zoom, scale numerator/denominator,
world width/height, and image center. These are the existing bounded port
placement inputs, not a claim of original map camera scale. The Rover loader
resolves its fixed ten CVRs plus system/palette/Spartan PCX files through the
normal installed/mod resource resolver. The underlying renderer still admits
only its measured Spartan reactor1, flat idle composition and system-light
profile. Other factions and unit assemblies are separate work.

The scene-owned immutable image cache dispatches by composition. Pixel and
placement keys include that composition, so even definitions with the same
name and view cannot alias Scout and Rover images or actors. Distinct UnitDefs
still share identical pixels while retaining their own actor instance counts.
The existing64-view scene budget and shared texture lifetime continue to apply.
Unit preview widgets can retain their image after Unit/manager/scene teardown;
removing the widget releases it after its final borrowed actor is gone.

The distinct render tag intentionally leaves the first conventional Unit
implementation's Infantry-only admission unchanged. Matching a Speeder's
checked design and faction to this fixed Spartan presentation requires a
separate integration change. Default world scripts are unchanged.

## Validation

Source base: `9e502e49`. Full Linux CTest passes8/8 in81.25 seconds. All22
Scout/Rover native groups pass with allocation tracking. The new group invokes
the actual GSE definition API, complete UnitDef wire roundtrip, frontend Unit,
scene registration, real widget binding/rebinding, and retained-preview teardown.
It checks wrong tag/profile combinations, frame caps and anchor rejection, plus
coexisting same-name Scout/Rover cache entries. Synthetic owned assets keep CI
independent of original files.

The same real Rover preview fixture also passes with allocation tracking using
the local GOG installation. Graphics are Null in this fixture; this confirms
installed asset resolution and the production renderer/actor/widget ownership
path, not interactive graphics or a playable conventional-unit match.

Two separately copied descriptor translation units remove the composition
cache key or tag/profile check. Each fails its intended real-preview assertion
with exit1; production checkout files are never temporarily changed. Peer
production review found no actionable issue. Full LLVM 15 correctness analysis
passes all 335 production translation units with zero failures in 251.26 seconds
(two jobs, excluding the Windows-only network implementation). No new sanitizer,
native Windows/macOS, or original executable comparison is claimed by this
adapter checkpoint.

## Focused publication

Publication code `b04a970ec913b117c027b04ae007525e0d7b503a` applies only source
`5c70b3ee` onto Rover renderer PR #152 at `83501815`. Its stable patch ID matches
the source change; no absent conventional-Unit or rollback prerequisite is
imported. The actual publication passes all eight CTests in 73.38 seconds,
all 22 Scout/Rover groups with allocation tracking, and the installed-asset
Rover preview group. Read-only peer review found no actionable issue.

The final publication commit adds only this receipt. GitHub Codex review of
this head is still required; the draft remains unmerged while that review's
usage quota is exhausted. The source analysis receipt above and the separately
validated combined Windows/Linux/macOS checkpoint are not substituted for it.
