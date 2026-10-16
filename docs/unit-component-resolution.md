# Unit component resolution

`UnitDesignResolver::Resolve(units, chassis, weapons, defenses)` resolves
already decoded metadata from [unit records](unit-design-table.md) and the
three corresponding component catalogs. It returns a value-owned
`ResolvedUnitTable`. Each entry contains its zero-based `#UNITS` record index,
the complete original `UnitDesignRecord`, and copies of its matched chassis,
weapon/package and defense. The result retains source order, including
duplicate display names. It remains valid after all input tables and source
strings are destroyed or changed; copying or moving it has ordinary value
semantics. No component pointers or externally owned indices escape.

The unit table must identify `smac` or `smax`, and all three component tables
must carry that same explicit identity. The resolver does not authenticate
file provenance or infer a ruleset from names. Component keys are exact,
case-sensitive strings:

| Unit record field | Component key |
| --- | --- |
| `chassis` | `ChassisComponent::names[0].name` |
| `weapon` | `WeaponComponent::short_name` |
| `defense` | `DefenseComponent::short_name` |

Alternate chassis names and full weapon/defense display names are not aliases.
The inspected SMAC `'Copter` and SMAX `Copter` remain distinct. Local component
row indices may differ from unit row indices and between catalogs. Resolution
uses keys rather than positional correspondence or known original indices.

Before resolving any unit, the entire component catalogs are indexed and
checked for empty, NUL-containing or duplicate exact keys. Ambiguity is an
error even if no unit uses that key, including when the unit table is empty.
Repeated alternate or display names are allowed. A missing unit reference or
invalid/mismatched identity also throws `std::invalid_argument`. Component
errors identify the catalog and one-based row; missing references identify
the field and the `#UNITS` data row, where row 1 is the count header. Inputs
are not modified and no partial result is returned. These are explicit GLSMAC
ingestion policies, not evidence of the original loader's behavior on malformed
or modified inputs.

The inputs are decoded table values. Apart from identity and key/reference
validation, the resolver does not repeat each parser's structural or numeric
checks on caller-mutated objects. It copies signed metadata without deriving
gameplay behavior. In particular, successful resolution does not establish
design eligibility, interpret the leading `*` in a name, map ability bits,
calculate mineral cost/carry capacity, select a reactor, load art, construct
`StaticDef`, or activate a runtime unit. Reactor selection remains an explicit
unresolved policy; it is absent from this API.

Native tests exercise real decoded synthetic tables, all retained fields,
source lifetime and mutation, value copy/move, source indices, duplicate unit
names, exact SMAC/SMAX spelling, mismatched tags, missing references and
whole-catalog ambiguity. A read-only Release probe resolves all 111 component
references across the installed 14 SMAC and 23 SMAX unit records, checks the
Colony Pod/Formers/Scout Patrol metadata, and exercises rejection outside
Debug builds. Original file hashes are recorded in the
[weapon parser audit](unit-weapon-table-parser.md); no original tables or assets
are redistributed.
