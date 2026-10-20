# Native faction prefix and initial exploration

Production C++ now implements the turn-zero faction prefix
`0x5c89bc..0x5c9051` and post-selection strategy/exploration
`0x5c9ad1..0x5c9dd5`. They operate on owned complete original faction storage
and caller-owned terrain records with the supplied CRT continuation. These are
startup composition stages, not a completed seven-faction new-game controller.

## Preserved state and ordering

`SmacStartupFaction` retains the whole 8,380-byte record and separate 512-byte
scratch buffer. The prefix changes only original-owned fields, preserving other
bytes and string-slot padding. Scratch becomes FF; it is not the technology
ownership table. The 75 active and 25 persistent goal records are initialized
completely, including their absent type and zero payloads.

Personality defaults come from supplied faction definitions. Optional personality
randomization precedes the rule override. Humans retain global difficulty;
computer records receive difficulty 3. Relationship draws nevertheless use
global difficulty, with the original rule override and clamps. All eight draws
occur for native faction zero and inactive factions too. Network starting energy
is 100 plus the definition's ENERGY; ordinary energy uses the supplied rule.
Channel labels are supplied localized single-byte text, including the separator,
and their terminators must fit the original 24-byte slots.

Site preparation sets labs/psych defaults and retains the original conditional
strategy/status draws. Region gates at 250 and 400 and strict comparisons matter
to both result bits and RNG continuation. It then visits the original inner 21
offsets in order, clips vertical/flat edges and wraps horizontal coordinates on
round worlds. Exploration ORs the faction's bit; positive factions copy live
item bits into their own remembered-item word. No other faction's memory is
overwritten. This is initial site exploration, separate from later Unit sight.

The APIs use synchronous borrowed terrain storage. They do not publish a partial
world, choose replacement start coordinates, refill site ratings, create a Base
or Unit, grant technologies or serialize generated worlds. Shared RNG is retained
unchanged. Trace callbacks report CRT call sites and ordered tile stores.

## Evidence and validation

Frozen bundle `independent-startup-unit-stack-evidence-20261020` has manifest
SHA256 `9e01a3692c1ae31ea247f2856bf15e9bd1d333eb1c9d4b3c733896e1de92e778`.
All 149 files were verified before importing compact numeric fixtures.
Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Full original buffers and purchased assets remain outside Git.

- 35 prefix controls compare all record/scratch bytes and 300 ordered CRT events.
  Inputs poison every byte, cover all eight faction slots, difficulty 0/3/5,
  random-personality/rule/network combinations and inactive/removal paths.
- 81 site-preparation controls compare complete faction/tile buffers, 560 CRT
  events and 3,310 ordered tile stores. Cases cross region-area and personality
  gates, difficulty 1/3/5, 32 branch seeds and wrapped/clipped edge exploration.
- An integration test generates the complete seed-1 terrain from zero allocation
  and initial RNG, then initializes Gaia from zero faction/scratch storage with
  installed definition inputs. Its record SHA256 is
  `d926d12652cad0cd24fa23c2429679bad79bc418af34395c3baa2179f75a83f3`,
  CRT 692949669 and shared state 110; terrain remains unchanged. Original search
  and post-search state are not substituted as inputs to this test.

The cached incremental build and four selected native groups pass, including the
existing original site-search controls. The production translation unit passes
correctness analysis. A focused three-TU ASan/UBSan build passes the same groups,
with other objects uninstrumented and leak detection disabled. No platform matrix
was repeated.

Next: compose the actual search and quality/cache adapter over generated raw
terrain, then feed its selected point into site preparation. Base founding,
technology/design grants, complete Unit factory visibility, controller ordering
and full seven-faction state remain required by the
[generated-world integration plan](native-generated-world-integration.md).
