# Generated terrain through first site and exploration

The C++ composition now reproduces original seed-1 setup from zero terrain
allocation through the first faction's site selection and initial exploration
at `0x5c9dd5`. No captured candidate, site rating, position, explored footprint
or intermediate original record is used as a producer input.

`SelectGeneratedStartingSite` connects the existing search/refinement and site
quality implementations to complete generated 44-byte tile records. It derives
map scale, ocean category, region areas/wet counts and raw tile semantics from
the owned producer state; existing Base/Unit positions and faction presence
remain explicit caller-owned startup context. Resource bonuses and site signals
use the existing pure resolvers with original pattern, rule flags and supplied
site coefficients. No installed-Planet header is constructed.

The adapter honors the raw exclusion bit and cached high-nibble rating. Cache
misses evaluate current terrain, then preserve the low owner nibble when storing
the result. This state is detached startup storage. Refusal or cancellation can
leave staged cache writes; callers must not publish it as a completed world.
The completed search retains its CRT continuation for subsequent strategy,
exploration and package construction.

## Composed acceptance

The test generates complete terrain from initial RNG `(shared=1,CRT=1)` using
the original seed-1 settings and explicit generation clocks. It initializes
Gaia from zero faction/scratch storage, then searches and passes the selected
point and its computed region area into initial site preparation.

| Boundary | CRT | Tile SHA256 |
| --- | ---: | --- |
| Full terrain producer | 3551643837 | `17095ab34ed6a22f2e47429aa14d7eb0bb5d40001c6b21e9641d41cad9c733f9` |
| Faction prefix | 692949669 | unchanged |
| Search/refinement | 789993329 | `8c984f7803fcd7752e749a5c34a3487cd7be045196062942c4362237651e7950` |
| Strategy/exploration | 2011860586 | `9ad821cb1eb05894ff5e6b674ec79e4c6adff4999575a73c6cdb717cdde5002a` |

Shared state remains 110. Search derives `(25,35)` through 558 candidates and
37 uncached quality evaluations. Every candidate's post-pair CRT boundary and
every ordered rating-cache store (offset, old and new byte) matches the original.
This checks the pair boundaries, not a separately instrumented port trace of all
1,116 draws. Complete original faction and tile buffers match after search and
preparation. Preparation adds three draws and 42 tile stores; its complete
faction hash is
`5e88422c2eec75c0293037eb7474791c85a109d6106106536f1ffafd981cd1da`.

The expected hashes and numeric output journal come from frozen bundle
`independent-startup-unit-stack-evidence-20261020`, manifest
`9e01a3692c1ae31ea247f2856bf15e9bd1d333eb1c9d4b3c733896e1de92e778`.
All 149 files were verified before importing fixtures. Original executable
SHA256 is `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The complete native journals/buffers and purchased assets stay outside Git.

The cached incremental build and 22 selected native groups pass. Five production
translation units pass correctness analysis. A focused seven-TU ASan/UBSan build
passes the same groups, with other objects uninstrumented and leak detection
disabled. No platform matrix was repeated.

## Remaining live milestone

This establishes the first site, not full startup. Later factions must consume
Base/Unit factories, ownership/territory/cache effects and the evolving RNG in
controller order. Named technologies, designs, complete factory visibility,
seven-faction composition and generated save/network publication remain required.
The live generated-game branch still needs switching only after those stages
compose. The [integration plan](native-generated-world-integration.md) retains
the complete milestone, including ordinary runtime screenshots and native
platform validation. No new live framebuffer is claimed by this state test.
