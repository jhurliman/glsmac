# Native unit drawing, stack markers and restored presentation

Ordinary SMAC stacks now draw one extra silhouette from the admitted
representative's raw flag mask, rather than up to eleven badges from individual
members. A thirteen-unit stack has the same overlay count as a two-unit stack.
The marker uses the representative faction's loaded alternate palette index,
without health or order text. Marker, primary flag, health and order share the
existing selection phase; hiding a body or changing a badge variant retires its
old marker instance.

The original UNITS art initializer derives chassis x anchors from marker pixels.
The production decoder now scans both rows in column order, retains the last hit,
and supplies the initialized offsets through the owned conventional actor
snapshot's chassis identity. Stock Infantry is13 and Speeder14. At native zoom0,
stacked primaries move six pixels right when the art offset is at most50; above50
the marker moves left instead. Native flag/health placement includes the ordinary
chassis x offset. Decoded offsets are cached per game, outside the blink loop.

Baked native CVR bodies and native map flag, health and marker sprites now bypass world lighting. The old
path applied another shader gamma/contrast adjustment after palette conversion;
these original software sprites perform direct palette writes.

Native worlds now admit the live startup CVR descriptor's legacy world-size
fields: native placement uses the returned model anchor and pixel scale instead
of rejecting those descriptors. Both selection and deselection rebuild the tile
presentation after the controller changes. An idle restored world schedules its
first actor snapshot when knowledge becomes ready, independently of already
drained Base/Unit spawn queues. None of these presentation steps replay gameplay
or visibility writers.

## Evidence and validation

Public receipt imports normalize line endings; numeric data are unchanged.

- The frozen original stack handoff47b8e09e verifies10 renderer controls, including
  mixed-owner visibility and thirteen members. Its other zoom/actionability
  receipts are reference work, not a claim that those profiles are all ported.
- Marker supplement15c96a23 verifies six actual software bitmap controls, six
  glyph/blink continuation controls and15 instruction guards.
- Anchor producerf4bdfd0a verifies the full original initializer's nine art bytes,
  six renderer contexts and23 guards. The production scanner also checks the
  owned UNITS asset, last-hit ordering, missing hits and invalid dimensions.
- All24 focused groups and all296 affected/integration groups pass. Actual frontend actor controls cover two/thirteen
  members, hidden tails, source colors/transparency, six-pixel geometry, the50/51
  branch, exhausted/reactivated variants, shared blink, unknown terrain, Hide and actual SelectUnit/DeselectUnit calls. The restore test drains initial actor queues, proves one authenticated snapshot, and checks unchanged authority/RNG with no repeated publication.
  Fonts/badge services in the CVR fixture are explicit doubles; the separate art
  scanner check reads the owned file. These are not framebuffer pixel-parity tests.
- Ten changed/affected production TUs pass correctness analysis. All15 focused
  groups pass with nine production and three test TUs instrumented under
  ASan/UBSan; remaining objects are normal and leak detection is disabled.

A normal1024x768 Linux/software-rendered SMAC Map of Planet game held its starting
Scout and produced a second Scout at Morgan Industries in2105. Save Campaign
wrote the owned world; Load Game restored it with both Units and the correct
Base population. The native stack sprite was created through the live path.
Two eastward fungus attempts spent movement without displacement; after the
research-priority continuation, the selected Scout moved north from(34,68) to
(34,66) in2107. The singleton flag/health remained at the destination without an
orphan stack marker at the Base. Normal window close exited0 and reported no
tracked OpenGL leaks. Heap allocations/audio and hardware performance were not
checked. Captures are actual working-tree frames based on8b70e41e plus this PR.

## Remaining boundaries

Current native map placement is the existing zoom0 profile. Complete camera/zoom
integration, stock y-anchor producers, native/special graphics-row selection and
full Unit artwork remain open. Chassis identity is not a substitute for a special
rendered graphics row: native records do not receive fabricated ordinary row
identity. The explicit revealed inspector and synthetic legacy layouts retain
separate presentation paths. Terrain black/grid, palette/lighting, coast and
relief integration remain open. This is not a full gameplay screenshot match.
