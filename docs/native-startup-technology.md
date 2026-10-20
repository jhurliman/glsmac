# Turn-zero named technologies and effective ownership

The generated startup composition now continues through Gaia's first named
technology award at `0x5c9e8d`, before automatic design/capability generation.
Production code uses the parsed faction profile's ordered named-technology
records, preserving the existing eight-record projection and repeated awards.
Numeric extra-TECH counts and other bonus kinds remain separate producers.

## Acquisition and query are distinct

`GrantSmacStartupNamedTechnology` implements the original turn-zero acquisition
path. Negative references return without writes. For references 0..88 it clears
a matching research target, stores the origin byte (positive source truncated to
a byte, otherwise zero), ORs the recipient's possession bit, and increments the
acquisition counter at original `0x95f1f4`, including repeated possession.
Other factions' possession remains. The counter is distinct from research
score at `0x95d504`. Full-record comparison includes unchanged Future Tech,
labs, score and cached-threshold fields.

`QuerySmacTechnology` implements the effective query at `0x5d17b0`. Owner zero
returns false even for the no-requirement sentinel. Positive owners satisfy -1;
other references outside 0..87 return false, including Future Tech 88. Disabled
first prerequisites reject; disabled second prerequisites reject unless the
first is -1. Only then is the owner's possession bit consulted. A startup grant
therefore need not make an ordinary query succeed.

The existing manual research picker now uses this query helper for owned
technology checks, retaining its definition-token validation and admitted-owner
guards. The startup helpers operate on complete owned raw faction records and
the separate global 89-byte possession array. They do not replace the live
versioned technology save codec or implement later research-acquisition variants.
They accept no RNG stream and do not construct automatic designs or publish a
partially initialized world.

## Original evidence and verification

Frozen bundle `independent-startup-named-technology-evidence-20261020` has manifest
SHA256 `fa74cbf047376003278dd6d7d827b4ebe8c3cb13d73f708d3f2d9c99709f9586`.
All 158 files were verified before importing compact numeric fixtures.
Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Purchased assets and full native capture buffers remain outside Git.

- 103 acquisition controls compare all 8,380 faction bytes and 89 possession
  bytes, with poisoned unowned fields. Coverage includes every catalog entry,
  owner zero/positive owners, prior own/other possession, matching/different
  targets, negative references and zero/negative/byte-wrapping origin inputs.
- 180 original effective queries discriminate owner/sentinel ordering, ordinary
  endpoints, Future Tech, disabled prerequisite pairs and absent/present bits.
- The zero-storage generated first-site test now dispatches Gaia's parsed TECH
  award, without captured intermediate state inputs. Complete faction SHA256 is
  `26e7a4b49621732754b49dfe56ea575eba4e4892a10f26804713684fb17b5458` and
  possession SHA256 is
  `89007795075a660518a8422021628d4fff5d29e0a27f6ba3ab15338f08f60edf`.
  Terrain remains byte-exact after exploration and RNG remains CRT 2011860586,
  shared 110. This boundary precedes the first Base or Unit.

The cached incremental build and 15 selected native groups pass, including
original research-pool controls and existing technology initialization/codec
callers. Four selected production translation units pass correctness analysis.
A focused five-TU ASan/UBSan build passes the same groups, with other objects
uninstrumented and leak detection disabled. No platform matrix was repeated.
The combined first-site/technology branch also passes 36 selected native groups
after integration with the merged faction and Unit startup helpers. Unchanged
production analysis and the separate seven-TU first-site sanitizer results were
reused.

Next requirements are automatic designs/capabilities, full Base/territory/resource
construction, full Unit factory/visibility and controller ordering for all seven
factions. The [generated-world integration plan](native-generated-world-integration.md)
retains live save/network publication and platform acceptance gates.
