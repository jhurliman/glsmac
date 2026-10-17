# Frozen SMAC predefined unit host profile

The real host preparation path now parses the selected `alpha.txt` component,
ability and `#UNITS` tables and invokes `SmacPredefinedUnitCatalog::Build`.
The result joins the existing five frozen rules profiles. Clients receive owned
metadata and checked initial records; they do not read their local rules files.
SMAX and joining preparation retain an absent SMAC unit profile and perform no
new source reads. SMAC still reads `alpha.txt` once and the seven faction files
once. Repeated preparation does not reload a frozen session.

This supplies immutable inputs, not live conventional units. It adds no runtime
`Def`, faction design registry, technology ownership, buildability, workshop,
prototype surcharge, renderer or startup placement behavior. Source prerequisite
`UNKNOWN` remains distinct from `NONE`, `DISABLED` and a known technology ID.
The producer's evidence and arithmetic restrictions remain those documented in
[the predefined catalog contract](smac-predefined-unit-catalog.md).

## Owned codec and supported ingestion

`SmacPredefinedUnitRules` owns all six raw tables and the final records. Nested
format V1 writes the six explicit `smac` table identities, ordered chassis,
weapons, defenses, reactors, abilities and source designs, then the derived
records. Every modeled raw field is included, including all six chassis
name/grammar pairs, unused source metadata, leading-zero binary spelling,
empty ability abbreviations, descriptions and unresolved prerequisite text.
Derived fields include source/component indices, reactor, abilities, automatic
plan and calculated caches, overridden bytes, prerequisite classification and
optional technology ID, active/prototyped state and faction exclusion.

The network ingestion domain is deliberately bounded:

- Exactly 9 chassis, 21 weapons, 10 defenses, 4 reactors and 24 abilities;
  0–64 ordered source designs and the same number of derived records.
- Names, keys and prerequisite text are at most 128 bytes; descriptions are at
  most 512 bytes. NULs are rejected. Required strings cannot be empty.
- Chassis grammar, triad, weapon/armor modes and booleans are validated before
  publication. Signed source integers retain signed32 values. Binary spellings
  are nonempty and at most 11 ability-flag or 24 selected-ability bits.
- Source plans use the existing parser's -1–12 policy. The separate typed
  producer's plan-256 override experiment does not widen text/wire ingestion.
- The complete nested profile is at most 24 KiB. Decode checks this before
  constructing its `Buffer`; exact row counts are checked before growing their
  result vectors. Trailing data, wrong versions and forged derived fields fail.

Validation reconstructs the final records with the existing production builder
and compares the complete encoding. There is no second cost/capacity formula.
Codec equality also compares the complete encoding, preserving order and raw
metadata that happens not to affect a derived value. Output owns its storage and
retains no source buffer or caller-container pointers.

These caps are GLSMAC admission policy, not a claim that the original executable
rejects the same malformed or unusually large source files.

## Global settings and packet admission

Global settings advance from V6 to V7. An explicit presence boolean and framed
unit-profile payload follow the faction profile. SMAC requires the profile;
SMAX rejects it. Old and unknown global versions are unsupported. Hosts stage
all parsed profiles and the existing settings tail, validate the serialized
global size, then commit and freeze. Receivers decode into owned local values
and compare a locked profile before updating settings. Reset and joining clear
the old catalog. Ordinary lobby updates with unchanged frozen metadata remain
permitted.

SimpleTCP retains its signed-`int` length prefix and complete serialized packet
in the same 65,536-byte receive buffer. `Network::MAX_PACKET_BYTES` therefore
equals `65536 - sizeof(int)` (65,532 on the validated platforms). The actual
`State::Serialize` path constructs the complete `PT_GLOBAL_SETTINGS` envelope,
including both GlobalSettings and FactionManager, and rejects an oversized
packet before Server queues it. State decode performs the same aggregate check
before parsing either component. Global decode bounds the supplied buffer before
allocating nested profiles; the nested unit codec separately bounds its bytes.

`GLSMAC::InitGameState` checks that complete packet after the authoritative
`configure_state` callback has supplied faction metadata, before host setup
returns to its caller. Joining returns before this host-only step. Rejection
uses the existing GSE game-error path. This is packet admission, not a claim
that the existing arbitrary FactionManager decode is transactional after every
possible later failure or that network I/O has been redesigned.

World-download binding is a separate companion change; this host commit does
not change the V7 world header or claim a new disk-save format.

## Validation

Source base: `929ffd197bce7e4254eed66fb9bd917fa56524a0`. All receipts below are
for the isolated source tree, not a separately prepared publication ancestry.

The actual native State → Server → Network test pads faction metadata until the
serialized packet plus real length prefix exactly fills the receive buffer. It
observes that exact payload queued by the real sender. One additional byte
throws before any additional send; a matching oversized State decode preserves
the prior settings and faction bytes. This test fails against a copied
production TU with only the aggregate check disabled.

Pure tests cover owned lifetime/copy/move, all 18 derived fields across 14 rows,
representative raw fields and endpoints, unknown prerequisites, every truncation,
framing/count/version/range errors, text and aggregate budgets, and a valid
in-range forged final icon. Host tests cover staging/retry/reset and frozen
equality. Existing real Client tests add four unit-profile failures and require
unchanged settings/factions and no notification; the accepted state callback
observes the received catalog and its prerequisite classification. Existing
SMAX, joining and rejoining tests remain active.

Full normal CTest passed 8/8 in 64.36 seconds. Full ASan/UBSan/LSan CTest with
`detect_leaks=1` passed 8/8 in 110.28 seconds. These full runs preceded one final
test-only strengthening of the accepted Client callback assertion. The final eight focused tests passed with memory tracking in both normal and
ASan/UBSan/LSan builds, including that callback assertion.
Four explicit changed-production translation units passed correctness analysis
with zero failures in 2.34 seconds. The shared-header full scan covered all 312
eligible production translation units with zero failures in 116.68 seconds.

Copied-TU negative controls remove only derived rebuild equality, locked unit
profile equality, or aggregate packet admission. They fail the corresponding
actual native assertions with exit 1. Intended production files remain intact
during these controls.

An opt-in ignored installed-source measurement used the actual GlobalSettings,
FactionManager and Packet serializers. `alpha.txt` SHA-256 is
`38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
The supplied faction ID/name/PCX metadata matches the repository's seven SMAC
roster entries; color and geometric numeric values have fixed-width frames and
were not loaded/rendered. With explicit Standard selection, default difficulty
and an empty game name, GlobalSettings is 35,482 bytes, FactionManager 2,289,
State 37,783, Packet 37,803 and the framed message 37,807 bytes. The unit profile
is 13,249 bytes and contains all 14 installed rows. This measures serialization,
not an interactive host join, rendered world, or complete gameplay startup.

Retained evidence is outside Git under
`/home/jhurliman/src/glsmac-unit-weapon-parser/predefined-host-*` and
`predefined-envelope-stock.log`; no installed assets or private original reports
are committed.
