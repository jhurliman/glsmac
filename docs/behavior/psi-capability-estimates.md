# RE-SMAC-PSI-CAPABILITY-001: retained capability PSI estimates

Fifteen controls execute the complete native component evaluators and capability
refresh. This supplies the formula missing from the earlier restart receipt.
[Cases](psi-capability-estimates-cases.json). These values are capability/rating
estimates, not combat odds or a complete combat-strength model.

Let A and D be this faction's **previous retained foreign available weapon and
armor maxima**. Let O:R be the configured land PSI offense:defense ratio
(installed SMAC3:2), and P the faction-definition PSI percentage bonus.

For any component with negative signed raw power:

1. Weapon: `v = trunc(D * O / R)`.
2. Armor: `v = trunc(A * R / O)`.
3. For either, apply `v = trunc(v * (100 + P) / 100)`.
4. For weapon, if the faction owns Dream Twister, add `trunc(v / 2)`.
   For armor, do that if it owns Neural Amplifier.

Each truncation is toward zero and occurs separately. A project anywhere owned
by this faction qualifies; a foreign host does not. The bonus applies to both
estimates. No current PLANET, morale, reactor, unit damage or live opponent
search enters these helpers. Nonnegative signed component power returns its
raw value without these modifiers.

Examples: previous foreign weapon9/armor8 yield weapon12/armor6. At weapon7/
armor5, the unmodified results are7/4; P25 yields8/5. Adding both projects then
gives12/7. Combining factors into one final division would give other answers.
Zero previous caches yield zero estimates, then the ordinary capability maxima
retain their independent floor1. Synthetic negative-input controls establish
signed truncation only; negative conventional capability caches are not admitted
ordinary campaign state.

## Placement in the complete capability producer

The [regional capability producer](regional-research-summary-producer.md) first
seeds separate PSI attack/defense caches when the predefined Mind Worm design
passes the actual design eligibility query with no Base context. Otherwise
their initial values are zero. It separately initializes the ordinary available
weapon/armor maxima to1 and scans eligible component definitions.

Eligible negative weapon/armor definitions use the formulas above, maximize
their separate PSI cache and participate in the corresponding ordinary maximum.
Native-design eligibility alone seeds **only the separate PSI caches**. A control
with Centauri Empathy alone obtains separate PSI12/6 but ordinary maxima1/1;
available Psi Attack and Psi Defense without the native design obtain ordinary
maxima12/6. Do not require additional advanced technologies for the native cache
seed, or promote that seed directly into ordinary weapon/armor maxima.

Only after rebuilding its own capabilities does the producer clear and rebuild
foreign maxima from qualifying peers' retained caches. Thus these estimates use
the previous foreign values, not the newly selected tier. No random calls occur
in these checked refreshes. Repeated refreshes can change results; do not add a
refresh to a read-only research query or inspector opening.

The earlier restart package (companion receipt restart-unit-package-selector.md) clears foreign
caches before its selector, explaining its zero PSI results. That special entry
condition does not justify zero estimates in the general producer.

Controls supply resolved ownership masks, project hosts and definition/rule
scalars; original eligibility and complete refresh execute. Technology awards,
project completion/capture, arbitrary custom faction admission and full foreign
diplomacy/routing remain separate lifecycles. Stock SMAC only; no SMAX/PRACX
inference. Native widths/overflow and zero ratio denominators are outside the
ordinary supported input domain.
