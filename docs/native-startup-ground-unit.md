# Ordinary ground Unit creation and first Scout projection

`CreateSmacStartupGroundUnit` composes the original positive-owner ground Unit
factory with canonical record initialization, stack placement and ordinary sight.
It operates on detached owned records. The generated composition now reaches the
first Scout boundary `0x5c9fb5` as a complete Unit/map projection.

## Allocation, counters and home

At 2,048 Units, return -1 without changing any storage or globals. Otherwise
append the complete canonical 52-byte record, increment the owner's design-count
byte with wrapping, and increment its role-count DWORD for plans <=7. Counts are
retained inputs; they are not reconstructed from current pool membership.

Nearest Base selection scans all owners and regions in record order. Later IDs
win distance ties. Home is assigned only if that selected Base is owned; a nearer
foreign Base prevents assignment to a farther own Base. A found Base updates the
global distance. No Bases preserve the previous distance. Cache invalidation
occurs only when the assigned own home equals the cached Base ID. Separate trace
events retain the original nearest-distance, home and cache store order and PCs.

Placement reuses the existing dense-pool insertion helper and preserves Base/
existing-stack tile affiliation. It retains the original occupancy, ordinary
presence and last-Unit-cache effects.

## Sight and admission

The existing ordinary `Explore` producer determines the ordered direct visits
and one-pass outer-hole reveals. The factory replays those visits over complete
raw records. Direct visits refresh remembered items even when already explored.
Newly explored tiles mark the landmark/dirty state only for the display faction,
outside editor/reveal mode. Each visit stores remembered items before exploration,
with distinct direct/outer instruction PCs. Later outer candidates see earlier
reveals through the existing producer; no fixed-point pass is added.

The admitted design profile is positive-owner ordinary ground plans 2/3/8/9,
without abilities, extended sight or sensor sight. Combat-plan late-AI creation
requires its separate producer. Foreign objects inside the first 25 probes or
their outer-neighbor probes reject before record creation; remote foreign Bases
and Units outside those probes are supported. Air/sea, artifact, extended/sensor
sight and foreign disclosure inside sight remain required future branches.

Observers are synchronous; failures can leave a staged factory partially changed.
No live registry or saved-world publication is performed here.

## Evidence and validation

Both frozen manifests and every listed file were verified before fixture import:

- `independent-startup-ordinary-factory-evidence-20261020`, 174 files, manifest
  `67a54d344f6515723405f14b077317987fb3412a1d4743fb8600cf5e71902a9c`.
- `independent-startup-remote-home-evidence-20261020`, 178 files, manifest
  `eb8e2cf09d454414cf6c8694aa944c826db0908ab222e7d58f6271cf4591bf4d`.

Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Full captures and purchased assets remain outside Git.

66 original factories compare complete tile, Unit, Base and owner-faction records,
allocation/global state and 1,711 ordered tile stores. Another 240 compare remote
own/foreign Base selection, ties, wrapping, cache sentinels and a foreign Unit
outside sight. They match all eight faction buffers, another 5,199 tile stores
and 432 ordered nearest/home/cache stores. Broader original record-write journals
are not claimed as independently replayed. Port-only rejection tests retain
storage when unclosed sensor/foreign sight is requested.

The zero-storage generated test composes generation, prefix, search, exploration,
named grant, Base seed/radius and territory, then creates stock Scout 2/plan 3.
It includes the Base-return dirty bit and separate startup caller `0x5c9f5d`
clearing the factory-assigned home from 0 to -1. All 52 Unit bytes and 140,800 map
bytes match original `5c9fb5`, as do the factory-owned faction counters and dirty
state. CRT remains 2011860586/shared 110. No captured intermediate producer output
is used as input.

This is not full intervening startup equality: Base resource passes and automatic
design/capability state are still open. The pre-resource Base contributes only
its coordinates/owner to this Unit projection; its later resource fields are not
accepted as correct by the gate.

Cached incremental build and ten selected native groups pass. Four production
translation units pass correctness analysis. A focused six-TU ASan/UBSan build
passes the same groups, with other objects uninstrumented and leak detection
disabled. No platform matrix was repeated. The complete seven-faction controller
and live publication/save/network gates remain in the
[integration plan](native-generated-world-integration.md).
