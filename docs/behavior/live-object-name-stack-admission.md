# Live main-map Unit, Base, name and selected-stack admission

The new [99 numeric controls](live-object-name-stack-admission-cases.json) execute96 complete native main-painter calls (`476da0 → 4764f0 → 473290/4737b0 → 4743e0`) and3 direct object calls that deliberately separate viewer argument from display faction. The original roster lookup, selection validity, linked-stack choice and name admission execute. Geometry/mouse projection, font/presentation, optional Base-status query and final body raster are supplied boundaries. This is an admission receipt, not a sprite/font pixel capture or a natural startup run. [Independent finite reference](live_object_admission_reference.py).

Pinned executable SHA256 is `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`. Frozen bundle: `tmp/reference/live-object-name-stack-admission-evidence-20261020`. Numeric cases include68 literal instruction guards and eight complete source-function hashes. Earlier evidence is unchanged.

## Fields required by the live bridge

**Stored visibility masks alone are insufficient.** Ordinary own Units bypass their stored mask, so an explored startup Scout with mask0 is visible. Requiring the viewer bit on every Unit hides it incorrectly. An own Unit is exceptional when **`(word(unit+8)&0x440)==0x440`**; it must then pass the same stored-mask test as a foreign Unit. Bits`0x40` or`0x400` alone do not trigger the exception. Controls cover0/40/400/440/441 with mask0/8.

Retain the native16-bit flags separately from DWORD status+4. For a conventional port Unit whose authoritative flags are known to be0, the ordinary ownership bypass is valid; do not claim all conventional/native Units have zero flags. For imported/native Units, preserve the actual word+8. A derived boolean for this exact combination can implement this one reader, but retaining the raw word avoids discarding other state.

| Input | Native field | Consumer purpose |
|---|---|---|
| Tile exploration | byte tile+4 | Ordinary Unit/Base body tile admission |
| Live occupancy/tag | DWORD tile+8 bits1/2; low nibble byte+2 | Native roster lookup and Base/garrison suppression |
| Unit owner | byte unit+14 | Ownership bypass, human selection, stack preference |
| Unit special flags | WORD unit+8 | Own`0x440` mask/selection exception |
| Unit stored disclosure | byte unit+39 | Foreign/exceptional-own disclosure |
| Unit status | DWORD unit+4 | Selected/actionable checks, mask`0x4200` |
| Unit order | byte unit+17 | Blink eligibility and actionable garrison |
| Stack links | signed WORD unit+48 next, +50 previous | Head normalization and selected/domain representative |
| Design chassis/domain | design+36, chassis metadata+1 | Stack rank; special override chassis8 substitution |
| Base owner/disclosure/name | base+4 / +10 / +19 | Body and independent name gates |
| Published roster counts | `959fd0` Units, `959fd4` Bases | Lookup and name iteration; publish first Base before repaint |

View selection/control/pending/blink state and preferences are also inputs; they cannot be reconstructed from object stored masks.

## Ordinary bodies and names

First admit the render lifecycle: **`944608!=0 || 9ba55c!=0 || (rules&0x80)!=0`**. Object and name readers test this independently (`473296..4732bc`, `4737cb..4737e4`, `4743e9..474405`). Editor visibility alone is not an alternative to this lifecycle gate.

In the ordinary view, before body admission, view flag`0x1000000` suppresses objects; the object reader requires some bit from`0x802000c0`; view flag`0x1000` also suppresses objects. These are not name gates. Special high-bit views permit the appropriate Base path but suppress ordinary Units. Actual controls demonstrate independent names surviving both object-suppression flags.

For non-override ordinary actors, tile knowledge is **explored for the viewer argument OR faction revelation`0x200` OR editor`0x80`**. **Unity Survey does not admit bodies.** Faction revelation bypasses tile exploration, but does not waive foreign stored disclosure. Editor bypasses both.

After tile admission, ordinary Base body requires Base-display flag`0x80` (or special filter bit`0x200000`), a successful actual Base lookup, and owner==global display faction OR Base stored bit for that display faction. The Unit reader uses the analogous ownership/exceptional-own rule above. Its normal unselected path requires Unit-display bit`0x40` (or special filter bit`0x200000`). An admitted selected Unit can still draw when ordinary Unit-display bit`0x40` is absent; the controls distinguish that path.

For a normal Unit-only square or Base-only square, without selection/overrides:

| State | Own body, mask0 | Foreign body, mask0 | Foreign body, viewer mask set |
|---|---|---|---|
| unknown | no | no | no |
| unknown + Survey | no | no | no |
| explored | yes | no | yes |
| unknown + faction reveal | yes | no | yes |
| unknown + editor | yes | yes | yes |

**Base names are a separate roster pass**, not children of admitted Base sprites. `4743e0` requires view flag`0x800`, display preference`959f9c&0x2000`, zoom>-11 and a nonempty Base roster. For each Base in the viewport, it requires editor OR own Base OR stored Base disclosure for global display faction. It does **not** test tile exploration, Survey or faction reveal before the primary name. Supplied controls consequently draw an own name or stored-disclosed foreign name on an unknown square while its Base body remains hidden. This is a discriminating reader control; normal discovery producers may keep those states consistent. Do not invent exploration changes to make this consumer pass.

The terrain viewer argument and global display faction remain different authorities. Three direct calls use viewer3/global display2 and prove that byte8 exploration admits the tile while stored disclosure requires bit4, not bit8. In the ordinary complete painter, the caller supplies its current display faction as viewer (or−1 for editor); the direct edge is not the ordinary full-painter configuration.

## Selected blink, Base occupancy and stacks

Normal selected id is`9430dc`. It is considered before ordinary square lookup only when its coordinates match, its owner is in human mask`959ff0`, actual `5d9980` accepts it, and status+4 lacks`0x4200`. That helper rejects the own`0x440` flags combination, so such a Unit can be visible through its stored bit while taking the ordinary lookup path rather than the selected blink path.

For a valid selected conventional Scout, off phase requires control`9430d8!=0`, blink phase`9430f8!=0`, pending command`9430e4!=0`, order0, no suppression`943100`, and the normal network conditions. Whole-Unit preference`959f9c&0x200` omits the entire Unit. Otherwise mouse parity-distance<=2 also omits it; farther away produces renderer flags`0x403` instead of ordinary`3`. The existing [blink contract](selected-unit-blink-timing.md) establishes that this draws the model without its badge/health/order overlay. These controls run the whole main-painter admission and actual stack-selection prefix; the final model/overlay pixels are not newly executed. Unknown/Survey squares are still rejected before normal selected blink handling.

On a live Base marker with valid tag nibble0..7, ordinary Unit bodies are hidden unless the Unit is selected, control/pending state is active and `5d9ad0` says it is actionable. In the bounded allowance3/untransported profile, order1/2/3 with status`0x4200` clear is not actionable. Controls show unselected garrison and selected order2 hidden, selected idle Scout admitted, and off-far model retained without overlay. Invalid tag15 does not apply that garrison suppression, even though coordinate-based Base lookup can still find a Base record.

Actual `5d7fe0` scans the roster and follows previous links to the head. The body consumer applies disclosure to its selected/lookup Unit **before** renderer helper`56fc40` chooses a stack representative. That helper normalizes to the head, then walks next links. Eligible selected own Unit wins immediately when control/pending state is active and status lacks`0x4200`. Otherwise rank is **chassis-domain value +4 for own, +0 for foreign**; strictly greater replaces the current choice, so first wins ties. This helper does not independently filter every stack member by its visibility bit.

The mixed-owner control has a hidden foreign head and an own tail: clearing the head's stored bit hides the whole ordinary draw before promotion; setting it lets the own tail win. This is a bounded consumer discriminator, not a claim that every such mixed-owner stack arises through ordinary producers. Do not replace native ordered head admission/selection with a generic per-member visible-list sort.

Three special pointers precede ordinary knowledge/disclosure/selection: `6ab1d4`, `6ab1cc`, `6ab1d0`. Matching`cc`/`d0` coordinates can request flags2 even on an unknown foreign square with mask0. For`cc`, design chassis8 substitutes the next stack link and omits when absent. `d4` requests flags3; renderer stack choice excludes that id and suppresses a remaining duplicate of it. A solitary matching`d4` therefore produces a renderer request but no body at this layer; with a second stack member it can draw that other member. Preserve these as explicit special presentation contexts, not as ordinary exploration exceptions for every selected Unit.

## First Base before the first port turn

Do not gate object/name publication on the port's turn id becoming>0. The checked native lifecycle words are independent render/command state; ordinary command-entry`527eb0` writes`944608=1` at`527ee4`. The primary name pass has no mission-year/turn comparison. Controls with`944608=0,9ba55c=3` still draw the own startup Scout with stored mask0; equivalent Base controls draw the first name. With both words0 and rules`0x80` clear, bodies and names are withheld. This proves the consumer gate, not a complete natural startup ordering run.

Once a first Base record is published and `959fd4=1`, the ordinary eligible name pass draws `First Base` immediately. Count0, name preference off, or zoom−11 omits it. A UI bridge should publish the founding result, roster count and selected/occupancy changes together, then repaint under the existing active render context. It should not wait for the simulation's next-turn callback merely to attach the first label.

## Reproduce

```
python tmp/reference/check_live_object_admission.py --verify-only
```

All99 independent model results match the native consumers:37 Unit body continuations,27 Base body calls and39 primary names, with68 literal source guards.96 cases execute the full main painter through return;3 deliberately call the complete object consumer directly. Conventional no-transport fixtures use native `5d9980/5d9ad0/56fc40` with supplied allowance3, fixed projection/mouse services and body/font sinks. Optional Base status/production labels, full graphics, producer/save-load lifetimes and the complete network/transport state space remain covered only by their separate contracts or outside this bounded receipt.
