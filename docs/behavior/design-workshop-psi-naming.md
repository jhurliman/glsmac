# Workshop PSI automatic naming and save aliases

The independent `workshop_general_reference.name_variants` and
`automatic_name` now accept negative weapon and armor strengths. This extends
the [domain naming reference](design-workshop-domain-naming.md) without a
separate PSI-name lookup or generated-name input. The source is original594320;
the original441e20 save path is checked separately from the naming routine.

## Exact signed rules

Three branches require **nonnegative signed weapon strength**:

- Scout/Recon compact naming: the signed test594628..59462a rejects a negative
  weapon even when chassis speed is greater than both component strengths.
- Armor-led defense:594881..594883 rejects a negative weapon. Positive armor
  being greater than a negative weapon does not produce an armor-prefixed name.
- Air Superiority/interceptor wording:594a83..594a85 rejects a negative weapon.
  The air ability abbreviation is still skipped by the earlier prefix scan.

Negative armor has no corresponding rejection. It participates in signed
comparisons, so ordinary weapons with PSI armor can still enter compact names
or interceptor wording. The existing signed minimum prevents elite naming when
either combat strength is negative.

| Declared owner1 Fission inputs | Original automatic name |
| --- | --- |
| Infantry, PSI weapon, ordinary armor | Psi Infantry |
| Rover, PSI weapon, ordinary armor | Psi Speeder |
| Foil, PSI weapon, ordinary armor | Psi Foil |
| Needlejet, PSI weapon, Air Superiority | Psi Needlejet |
| Needlejet, PSI weapon, Air Superiority + Clean | Clean Psi Needlejet |
| Infantry, Gun, PSI armor | Scout Infantry |
| Rover, Laser, PSI armor | Recon Rover |
| Infantry, Impact, PSI armor | Impact Squad |
| Needlejet, Impact, PSI armor, Air Superiority | Impact Tactical |
| Infantry, PSI weapon and PSI armor | Psi Infantry |

The name comes from resolved component text. Installed weapon12 has signed
strength-1 and text **Psi**. The algorithm does not replace every negative
strength with a generic PSI label. Its normal attack variant still XORs the
weapon's low bit into `((reactor-1) XOR owner) AND 1`. A controlled strength-2
override on the same component retains its text but changes owner1's result to
**Psi Squad**. Thus testing only `strength == -1` or taking an absolute value
would be incorrect. A strength-2 armor control still gives **Scout Infantry**.

Ability/suffix precedence also uses signed comparisons. With PSI weapon and
ordinary armor, the Amphibious abbreviation remains and its special attack
suffix is not selected: **Amphibious Psi Infantry**. With Impact and PSI armor,
the abbreviation is suppressed and the attack suffix is used: **Impact
Invaders**. Artillery with PSI weapon gives **Psi Artillery**. Police compact
naming with Gun/PSI armor gives **Police Infantry**. These are direct controls,
not inferred strings from the strength-text formatter.

Owner/reactor selection, retained-record reactor disambiguation and collision
handling stay shared with domain naming. Owner2 changes the basic PSI Infantry
alternative to **Psi Squad**. Inactive records colliding with both alternatives
give **Psi Infantry Mk2**. All naming controls leave the design pool unchanged.

## Commit alias versus generated name

The generated name is not always the final saved name. An ordinary Infantry,
PSI weapon12, PSI armor9, Fission reactor1, abilities0 tuple matches intrinsic
design **8**, whose stored name is **Mind Worms**. Real441e20 returns that design
instead of creating slot127. The existing name and **all retained arrays remain
unchanged**. The independent check verifies the existing record's entire
component tuple and its stored name; it does not substitute the alias name
into the naming algorithm.

Five other controls create slot127 and match independent build/name/store
replay across the **complete512-record design pool**: PSI weapon Infantry,
Gun/PSI-armor compact Infantry, Impact/PSI-armor Infantry, Clean PSI Needlejet
with Air Superiority, and Impact/PSI-armor Needlejet with Air Superiority.
Other retained arrays remain unchanged. All six returned designs pass native
production eligibility. Preserve exact-tuple prototype reuse before applying
new generated names in the port's Apply response.

## Frozen evidence

Additive private bundle:
`tmp/reference/independent-workshop-psi-names-evidence-20261021/`.
Manifest SHA-256:
`1e033e74480dd2f0449f1e623ac5e7eb13ebdd3e2ceecfe453523ca0ac62e990`.

All **25 original naming controls and six real commits** pass independent
checks. The prior43 domain controls and six commits,40 original builder/name
controls and seven commits,216 ability checks and11 cleanup checks also pass.
Only the focused PSI controls required new original execution; no raster or
full-parent matrix was repeated.

The bundle retains resolved rule/text inputs, explicit strength overrides,
complete retained pools, original string-read traces, commit observations,
source disassembly and the frozen reference. The public
[case index](design-workshop-psi-naming-cases.json) lists the expectations.
The checks use the installed English rules and explicit human caller state;
they do not establish arbitrary alternate language/rule tables, AI-specific
Scout-category overrides, or whole-parent initialization.
