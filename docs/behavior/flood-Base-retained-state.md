# Flooded Base population, retained allocation and removal

For a surviving flooded Base, the class-change helper updates **population and
the Pressure Dome facility bit only within the Base record**. It also sets the
owner's flood-status bit and increments flood scratch. It does not immediately
repair workers or recalculate resources.

Whole-record comparisons of the frozen population12,4,40 and last-Base controls
show exactly those two changed fields. The existing-Dome and sea→land controls
change no Base bytes. New nonzero canaries confirm that a population12→6 flood
retains all of the following:

- Worked-tile bitmap, retained specialist count, provisional specialist count,
  and all sixteen packed specialist type slots, including unused padding.
- Accumulated nutrients and minerals, baseline production fields and queue state.
- Retained intake, support, surplus, economy, psych and labs fields.
- Other Base fields and the checked current-Base/resource-cache identities.

The canary starts with eight noncentral workers plus four specialists at
population12. After population becomes6, its stored allocation still describes
the former allocation. Do not silently normalize it as part of this original
flood transaction. The diagnostic provisional count and opaque Base-reference
words are also retained; those canaries are not a natural campaign admission.
The faction population aggregate likewise remains unchanged here.

Installing the Dome uses the facility bit setter, without a resource pass. An
already present Dome bypasses damage, installation and flood marking. The new
direct and complete deferred sea-step controls record no entry to Base-context
loading, support, intake/worker allocation, nutrient, mineral, energy or citizen
calculations. Both retain the same nonzero Base fields and checked cache keys.
The full caller still performs the established climate/region/coast/territory
work and frontend notification/refresh handoff. Headless presentation services
are supplied, so this is not a claim about every calculation caused by later
opening or rendering the live Base inspector.

## Protection from deletion

After applying population damage and installing the Dome, the helper queries
the original objective predicate. A positive result floors population to1.
For ordinary local campaign inputs with objective flags clear, that predicate
returns false **even when the Base has HQ**. If population remains nonpositive:

1. An owning faction with at most one Base retains this Base at population1.
2. With multiple Bases, ordinary local mode permits removal. The separate network
   branch also protects when its checked removal-permission field is zero.

These tests use retained owner Base count and the local/network authority;
they do not search for HQ, inspect a human-control bit or apply Do Or Die.
Objective protection is separate: it can come from the global all-Base rule,
the Base's explicit objective bit, the project-host objective rule, or the
enabled selected-facility objective. Source identities and exact flag bits are
in the private index. Six complete predicate queries distinguish those cases,
including an HQ Base with no active objective rule.

## When the Base is removed

The flood caller invokes the ordinary removal transaction. Reuse the established
home repair and dense Base deletion: own homed Units
rehome to the nearest remaining own Base measured from the lost Base, later
Base wins distance ties, and absence of a new home removes those Units. Foreign
Units with that home become unhomed. Higher home indices shift with Base
compaction. Already unhomed Units remain unhomed in that home-repair passage.

Removal also repairs references to Base indices, decrements owner Base/region
counts, clears the Base map marker, adjusts project-host references, and runs
ordinary radius/map/territory cleanup. It invalidates the resource-cache key
when that key names the deleted Base. The three retained Base-index reference
words are repaired by identity; they are not indiscriminately reset resource
values. Surviving Base resources are not recalculated in this transaction.

A new first-of-two flood control explicitly puts HQ on the Base that is lost.
The survivor retains population7 and its facilities unchanged, with **no HQ
relocation**. Capture's separate paid HQ relocation path must not be applied to
ordinary removal. In this control, ordinary Base removal destroys the native
Isle before the later class-conversion pass destroys the Mind Worms; Locusts
survive. This reuses the existing Base-before-Unit ordering and
[deletion accounting](native-deletion-accounting.md).

[Cases and retained values](flood-Base-retained-state-cases.json). Four new complete
flood/deferred callers and six objective queries supplement the frozen records;
the population4/40/last and home-repair sequences are reused, not rerun.
