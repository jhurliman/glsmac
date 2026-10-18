# RE-SMAC-CITIZEN-FINAL-001: final projects and suppression

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 22 named cases, 1,800 deterministic sampled combinations and 512
complete-current-calculation combinations pass. The complete runs use POLICE
-2, which avoids police and pacifism effects; they do not establish those
effects or final results under other police ratings.

## Late project stage

Begin with the talent count T, drone count D and additional calming burden S
left by preceding stages. Project ownership benefits apply to all bases of
the owning faction; a rival-owned project contributes nothing.

Add 1 to T for Human Genome Project and 2 for Clinical Immortality. Both stack.
Then, if the faction owns Longevity Vaccine, reduce D according to the
resolved economics model:

| Economics model | Drone reduction |
| --- | --- |
| Simple | 1 |
| Free Market | 0 |
| Planned | 2 |
| Green | 1 |

This tests the economics model selection, not the ECONOMY rating. Arbitrary
supplied model indices outside the four ordinary choices take the one-drone
branch, except the exact Free Market and Planned indices. This boundary does
not establish how custom models become selectable.

No clamp occurs between those project additions/reductions and suppression.
Longevity can temporarily make D negative, and talent projects can temporarily
take T above population. Do not normalize after each project separately.

## Suppression and final conversion

If Punishment Sphere is present, or the resolved nerve-stapling counter is
nonzero, set both T and D to zero. The two routes have the same suppression
effect here, and applying both does not add anything. Project talents are
added before this suppression, so Human Genome Project and Clinical
Immortality do not restore talents afterward. Counter values 1, 10, 128 and
255 all activate suppression; this is a nonzero-byte test, not a signed
positive-value test.

After suppression, or immediately after projects if neither route applies:

1. Bound T and D separately to 0..population.
2. Retain S and apply the [shared cancellation rule](citizen-conversion-stages.md)
   against population minus the resolved excluded-population count.
3. Store the resulting T, D and S as this calculation's final values.

Suppression does not directly clear S. For example, with population 7,
excluded count 0 and S=3, it stores T=0, D=0, S=3: the normalization condition
is not triggered. If all seven people are excluded instead, S=3 exceeds
capacity zero, so normalization bounds it to D=0. S is an additional burden
field, not a third disjoint citizen group. These checks do not establish its
rendering as particular citizen icons when D is zero.

Clinical Immortality's two-point input is not necessarily two extra final
talents. At population 7, excluded count 0, pre-project T=1/D=5/S=0 and no
Longevity or suppression, Clinical alone produces final T=2/D=4/S=0 after
cancellation. Human Genome alone produces T=2/D=5/S=0; both projects produce
T=2/D=3/S=0. Keep the late stage distinct from earlier Paradise and psych.

## Evidence and open transitions

The [22 named cases](citizen-final-projects-and-suppression-cases.json) supply
pre-project counts and compare snapshots after projects, after suppression
and after final normalization. They cover ownership, stacking, economics,
temporary negative demand, population caps, excluded population, counter
boundaries and retained S. Another 1,800 sampled combinations cover the same
arithmetic with varied populations and counts.

The 512 complete runs execute from the original citizen routine's entry
through final count storage, including actual facility/project input
construction and the previously specified initial/psych/facility stages.
They combine populations 1/7, exclusion 0/all, all four economics models,
all ownership combinations of the three projects, Sphere present/absent and
nerve counter 0/10. Earlier inputs include social TALENT, faction TALENT,
bureaucracy, Commons, Paradise Garden and psych. Assimilation and faction
DRONE are neutral. No instruction patches or substituted helper returns are
used.

This establishes resolved-state arithmetic under the stated police condition.
It does not establish nerve-stapling legality, success/failure, duration or
aging; project capture/destruction; social-model transition timing; police
units or pacifism; full turn scheduling; riots or Golden Ages. The original
nerve-stapling command's counter access identifies the suppression input, but
its complete action behavior has not been emulated here. The police and
supplied-count pacifism stage is now separately checked in
[police and pacifism](citizen-police-and-pacifism.md); its upstream counter
production and complete combination with every late project remain outside
those project checks. Subsequent [Golden Age/riot admission](golden-age-and-riot-admission.md)
now has separate original-x86 coverage, with notification/map callback and
riot-consequence limitations stated there.
