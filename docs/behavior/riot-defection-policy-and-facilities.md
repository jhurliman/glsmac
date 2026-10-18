# RE-SMAC-RIOT-POLICY-001: replay, policy and facility reconciliation

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 15 named, 1,100 sampled, 42 transfer-core-entry and 42 riot-entry
checks pass. Replay/policy/status code and the later facility-grant block are
executed as two phases. The intervening territory/unit processing is skipped;
this is not completed defection.

## Replay event

After the [initial transfer mutations](riot-defection-transfer-core.md), append
a base ownership-change replay event containing the recipient faction, current
turn index and base coordinates. Existing events remain intact and the event
count advances by one. The recorded turn retains the low sixteen bits; the
synthetic turn-65536 case records zero. This checks event append, not replay
rendering, save/load or buffer capacity.

The original append routine executes. Two diagnostic logging calls inside it
are omitted without side effects. No functional replay-write return is
substituted.

## Automation policy inheritance

Scan the live base list, excluding the transferred base and considering only
bases still owned by the old faction. Score each candidate as population,
doubled if it has completed Headquarters. Copy the entire stored automation
policy from the highest-scoring candidate to the transferred base.

Use strict improvement from initial score zero: equal scores keep the earlier
global base entry. A zero-population candidate does not become the source.
If no positive-scoring old-owner candidate exists, preserve the transferred
base's existing policy. Bases owned by the recipient or other factions do not
participate, regardless of size or HQ.

For example, a remaining population-3 HQ scores 6 and beats a population-4
ordinary base. It ties a population-6 ordinary base, so their list order
decides. This copies the policy of the old faction's selected base, not the
recipient's best base. The fixture uses opaque policy tokens; individual
automation options are not decoded here.

## Status cleanup

The subsequent status mask clears the riot and Golden Age flags. The checked
unrelated status marker survives. Several other markers are also cleared by
the original mask; their complete semantic identities remain open. This
handoff specifies the named status effects and preserves that mapping gap.

This cleanup occurs before the intervening territory/unit block and before
the later full base recalculation. It does not establish the eventual citizen
counts or whether a new status condition will arise on a later check.

## Both factions' explicit grants are added

In the later facility block, process the old faction's explicit FACILITY
modifiers, then the recipient's. For each, clamp its configured facility ID
to 1..38 and set the corresponding presence. Starting facilities remain
present through these grant loops. Duplicate grants simply set the same
presence again.

The result is the union of facilities entering this block and both factions'
explicit grant sets. The old-faction pass adds/retains its grants; it does
not remove them after ownership changes. A missing old grant is set as well.
This behavior is verified in runs from the original riot entry, using the
frame built by that handler.

With a Tree Farm initially present, an old Network Node grant and a new
Energy Bank grant, the resulting set contains all three. For custom-rule
boundaries, a grant below 1 maps to Headquarters and one above 38 maps to
the Stockpile Energy entry. The latter is a presence-field boundary, not
ordinary facility construction. The cases use 0 and 99 explicitly.

These loops concern explicit FACILITY modifiers. Other free-facility
mechanisms and any changes caused by the skipped territory/unit processing
are outside this family. Interpret the union relative to the state entering
the facility block, rather than assuming a complete transfer preserves every
initial facility.

## Acceptance scope

The [15 named cases](riot-defection-policy-and-facilities-cases.json) cover
policy scoring/ties/ownership, no source, status clearing, old/new/duplicate
grants, custom bounds and replay append/wrap. The 1,100 sampled cases vary
owners, source bases, facility sets and grants. Another 42 checks begin at
the initial transfer core for every distinct faction pair; 42 more begin at
riot entry and run failed facility searches, former-owner admission, transfer
core and replay/policy/status processing before the facility phase.

Original replay, policy, status and grant instructions/helpers execute
unmodified. Diagnostic callbacks are omitted. Riot-entry cases additionally
supply zero RNG returns and nonmutating notification/map callbacks. Every
case skips the intervening map/unit block before executing the grant loops.

Complete defection, territory and unit/design changes, unexplained status
markers, automation intervention, final recalculation/notifications and
full-game turn observations remain outside these two-phase checks. The
intervening [radius/unit conversion pass](riot-defection-unit-pass.md) now has
separate contiguous original-x86 coverage, with prototype-creation, map-field
identity and later cleanup limits stated there.
