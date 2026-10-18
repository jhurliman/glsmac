# Riot transfer to an inactive former faction

Stock SMAC accepts a positive recorded former faction even when that faction is
inactive. The riot transfers the Base and eligible Units to that slot, but does
**not** activate it, initialize it, grant a restart package, or change its human
control bit. A port must therefore represent ownership by an inactive faction
without throwing or inventing an immediate revival.

The former-owner branch bypasses the geographic recipient search. Its admission
does not test the active roster, recipient Base count, or diplomatic relationship.
Ordinary transfer rules still apply: clear assimilation, update Base counts,
convert the eligible nearby homed Units, remove the remaining old-owner homed
Units, and recalculate resources under the recipient. The original Base upkeep
returns early after ownership changes, before the old owner's ordinary posting.
See [complete transfer rules](riot-defection-completion.md).

The normal and batch turn controllers both test the active roster before entering
an owner's upkeep. An inactive recipient remains skipped despite now owning a
Base and Units. This differs from an active recipient with zero Bases: that slot
is admitted. This finding does not establish a complete global-turn resume.

Loading has a separate roster repair. For each positive faction, it sets the
active bit if the live roster contains an owned Base or a Unit whose design role
is Colony Pod. It clears the bit otherwise. Other surviving Units alone do not
qualify. This repair preserves the human control mask and performs no faction
initialization or random draws. The controls execute the original post-load
repair fragment; they do not constitute a new whole-file save/load test.

Elimination and riot are separate operations. The tested prior elimination calls
the original disposition helper and its nonzero faction initializer, completes
the unsuccessful late-game restart search, and clears the active bit. The later
riot does not repeat those operations. Consequently it retains the recipient's
credits, labs, research target and fraction, technology possession, custom design
block, pending debit, diplomacy, and loss history. Diagnostic diplomacy/debit/loss
canaries confirm retention even for values an initializer would alter. Existing
[last-Base restart evidence](last-base-restart.md) describes the additional
disposition call possible in the complete former owner's upkeep tail; the new
elimination setup invokes one disposition helper directly.

Five complete original Base returns cover active-zero-Base, explicitly inactive,
actually eliminated human, actually eliminated AI, and retained-field canary
contexts. Twenty normal/batch gate controls and fifteen post-load roster fragments
check admission boundaries. The actual eliminated human and AI contexts consume
1,344 and 834 CRT draws during disposition, respectively. Their CRT streams
continue into the fourteen riot draws without reseeding. Riot gate/candidate
return values are deliberately forced to zero after each real draw; native raw
values and seed recurrence are checked. There are no shared-generator draws.

These are controlled two-faction, rainy-map, default-design admissions, with a
cloned resolved faction profile and headless presentation services. They do not
prove arbitrary inactive custom-design translation, a natural GUI campaign
history, or immediate playability of the inactive recipient. Literal states and
checks are in [the cases](riot-inactive-former-recipient-cases.json).
