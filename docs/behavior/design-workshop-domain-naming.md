# Workshop conventional sea, air and missile automatic naming

`workshop_general_reference.automatic_name` now supports ordinary conventional
land, Foil, Cruiser, Needlejet, Chopper, Gravship and Missile designs. The
independent `name_variants` producer replaces the previous land-only assertion;
`land_name_variants` remains a compatibility entry point. The subsequent
[PSI naming extension](design-workshop-psi-naming.md) adds signed negative
weapon/armor inputs and distinguishes generated names from saved aliases.

The source is original **594320**. Inputs include resolved component/language
fields, owner, raw selected design ID, requested component tuple and the design
pool. Native generated strings are comparison expectations, never naming inputs.
The engine can use typed component/state data; original addresses are provenance.

## Branch order and domain rules

1. Prefix the first nonempty ability abbreviation in ascending bit order.
   Artillery bit15 never supplies the abbreviation. Air Superiority bit5 is
   skipped on domain2, allowing a later ability such as Clean to supply it.
   The existing Amphibious abbreviation suppression remains in effect.
2. The Scout/Recon compact-name branch is land-only. Sea/air/missile speed does
   not cause names such as Scout Needlejet or Recon Foil.
3. For conventional combat mode below3, armor greater than offense selects the
   defensive branch, unless Artillery is present. This takes priority over the
   air-superiority branch. Otherwise domain2 with Air Superiority uses the
   defensive chassis word but prefixes the weapon name. This interceptor branch
   does **not** XOR weapon-strength parity into the variant selector.
4. Special chassis with signed offense at least99 uses reactor name, a space,
   then weapon name; it omits the chassis suffix. Thus the checked missile
   example is **Quantum Planet Buster**. Ordinary missile weapons retain normal
   weapon/chassis naming: **Missile Missile** at both Fission and Fusion.
5. Other conventional combat uses weapon plus chassis. Artillery and admitted
   infantry Amphibious suffixes retain their existing precedence.

The initial short/full selector is `((reactor-1) XOR owner) AND 1`. Ordinary
attack XORs offense parity; ordinary defense XORs armor parity. It is based on
owner ID, not faction personality. Collision handling can change the selector.

For an elite chassis word, compare `min(armor,offense)` **strictly greater than**
the threshold. Start with3 when armor<=offense, otherwise6; add1 for sea, or
add half the initial threshold with integer truncation for air. Both elite
chassis strings must be nonempty. Hence attack thresholds are land3, sea4,
air4; defensive thresholds are land6, sea7, air9.

Sea puts the elite word in the **short** alternative. Land/air put it in the
**full** attack alternative. For ordinary noncompact defense, the full word
stays the ordinary defensive chassis word, while sea's short alternative can
use its elite defensive word. Examples verified under explicit owner/reactor
inputs include **Chaos Megafoil**, **Antimatter Superfoil**, **Fusion
Thunderbolt**, and **Stasis Tactical**. Do not infer elite output from strength
alone without applying the variant selector and domain.

## Noncombat wording and retained reactor state

For noncombat modes, non-Infantry chassis normally prefix a reactor word when
reactor>1 and then a chassis word. Three exceptions matter:

- Foil Transport omits the chassis word: **Transport**. Cruiser Transport
  retains it, for example **Fusion Cruiser Transport**.
- Foil Colony Pod and Formers use localized label0x28c (**Sea**) rather than
  Foil/Skimship: **Sea Formers**, **Fusion Sea Colony Pod**.
- The existing land Former branch avoids an extra chassis word when its
  accumulated prefix already contains a space. Air Formers retain the chassis
  word: **Fusion Needlejet Formers** in the checked control.

The reactor disambiguation scan at5947c3..594847 reads the **retained selected
record's plan, chassis and reactor**, separately from requested naming operands.
It scans active intrinsic records, then the owner's bank. A matching plan/chassis
with a lower stored reactor sets a flag; the first matching record with reactor
at least the selected stored reactor clears it and stops. A freshly built active
selected record normally clears the flag itself.

Draft callers must preserve that retained-record input rather than comparing
only the requested tuple. With an inactive selected record and lower-reactor
history, checked sea controls produce **Impact Fusion** (reactor replaces the
attack chassis suffix) and **Fusion Hoverboat** (reactor replaces armor prefix).

Collision behavior is shared with the land reference: exact names in intrinsic
and own-bank records count even when inactive; foreign records and identical
component tuples are skipped. The checked both-alternative sea collision yields
**Impact Foil Mk2**; an inactive **Impact Penetrator** collision switches to
**Impact Needlejet**. Naming does not itself truncate to the saved31-byte limit;
the commit consumer performs that truncation.

## Validation and handoff

Additive frozen bundle:
`tmp/reference/independent-workshop-domain-names-evidence-20261021/`.
Manifest SHA-256:
`3aa76667481c298643ff268bfc630985e3f88f8d7ea55938c4a2447745774abc`.

All **43 native naming controls** match independent output. Six real441e20
commits cover Foil, Cruiser, Needlejet, Chopper, Gravship and Missile; complete
512-record design pools match independent build/name/store replay, other
retained arrays remain unchanged, and every saved design passes native
production eligibility. The previous40 builder/name controls,216 ability
compatibility controls,11 draft cleanups and7 commits continue to pass.

The public case index is [design-workshop-domain-naming-cases.json](design-workshop-domain-naming-cases.json).
The private bundle retains complete input pools, resolved text inputs, original
string-read traces, source disassembly, full commit observations, verification
scripts and the frozen reference. Those inputs distinguish a normal post-build
call from a draft using retained metadata. No full parent window or raster
matrix is required for this naming handoff. Alternate language/rules remain
separate work; PSI naming is covered by the linked subsequent extension.
