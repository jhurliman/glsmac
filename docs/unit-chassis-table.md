# Chassis component table

`UnitRulesTXTLoader::ParseChassis(source, ruleset_id)` reads one `#CHASSIS`
section into a value-owned catalog. Callers explicitly select `smac` or `smax`.
The decoder preserves source order and exact names; it does not choose files,
load assets, assemble designs, apply movement rules, or activate live units.

The source audit used the same installed files and hashes recorded in the
[weapon parser document](unit-weapon-table-parser.md). Both catalogs contain
nine rows with nineteen fields, followed by an optional trailing comma:

| One-based columns | Metadata |
| --- | --- |
| 1–8 | Four alternating name/grammar pairs |
| 9 | Speed |
| 10 | Triad: 0 land, 1 sea, 2 air |
| 11 | Range in turns from base |
| 12 | Missile flag |
| 13 | Cargo factor |
| 14 | Cost factor |
| 15 | Prerequisite token |
| 16–19 | Two alternating large-name/grammar pairs |

The original summary comment omits the missile field; the actual records and
its separate field comment establish column 12. Shifting cargo/cost/prerequisite
left would corrupt the table. `ChassisComponent::names` stores all six pairs in
their source order, with the trailing large-name pairs at indices 4 and 5. The
first name is the lookup key used by `#UNITS`; exact duplicate primary names
across rows are rejected. Alternate names may repeat, including all six names
of a single row. Selection of a particular name during design naming is not
implemented.

The source comment describes `M1` as masculine singular, `M2` as masculine
plural, with `F` and `N` for feminine and neuter. This decoder preserves the
grammar string and accepts exactly `M1`, `M2`, `F1`, `F2`, `N1`, `N2`. The
installed English files use M1/M2; synthetic fixtures cover all six codes. This
is an explicit supported syntax policy, not evidence that the original loader
rejects every other token. Names are not normalized: the inspected SMAC row is
`'Copter`, while the corresponding SMAX row is `Copter`.

Triad is validated before conversion to its distinct enum. Missile must be
the integer 0 or 1 before conversion to bool; nonzero values are not silently
coerced. Speed, range, cargo and cost remain complete signed decimal int32
metadata, including zero, negative values and representable endpoints. These
structural rules do not establish playable-design eligibility, air endurance,
missile combat behavior, cargo capacity, or cost formulas. The original loader's
permissiveness for malformed or modified records is not reproduced.

Section/comment/whitespace/EOF/trailing-comma behavior follows the existing
[component parsers](unit-defense-reactor-tables.md). Names and prerequisites
must be nonempty and NUL-free. Required sections must be unique and nonempty;
unrelated duplicate sections remain permitted. Missing/extra fields, malformed
numbers, unknown grammar/triad/flag values and duplicate primary keys throw
`std::invalid_argument`; field errors identify section, data row and column.
No partial catalog is published. Nine rows are observed, not hardcoded.

Native tests exercise all nineteen positions, all grammar codes, all triads and
both missile values, signed boundaries, each required field, repeated alternate
names and separate catalog identities. A read-only Release probe parses both
installed nine-row tables and checks Infantry, sea/air/missile metadata and the
apostrophe difference. No original tables or assets are redistributed. A design
resolver still needs cross-table references, `#UNITS`/ability semantics and
original-backed initial-reactor policy; none is inferred by this parser.
