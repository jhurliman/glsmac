# SMAC RESOURCEINFO parser

`loader::txt::SmacRulesTXTLoader::ParseBaseSquareRules(source)` returns the
Base Square, Bonus Square and Recycling Tanks nutrient/mineral/energy triples
for the existing SMAC center calculator. This is a pure source-text parser:
it does not select a file, read assets, cache rules, change session settings,
or activate the calculator in gameplay. It makes no SMAX compatibility claim.

RE-SMAC-BASE-001 identifies a nine-row RESOURCEINFO table with four signed
32-bit values per row. This bounded parser requires the canonical English
labels and order and decodes only the three needed triples. It rejects reordered,
missing, additional or duplicated rows rather than silently assigning a moved
row a new meaning. Localized labels and other layouts remain unsupported.

The required three values are signed decimal int32 numbers, with optional
minus/plus signs and surrounding whitespace. Negative values and representable
endpoints are preserved. Empty fields, `*`, partial numbers, embedded nulls and
overflow are rejected with row/column diagnostics. The unused fourth column,
annotations after the triples, and the numeric fields of unrelated rows are
not interpreted or validated. In particular, unrelated `*` fields are not
converted to zero. The required triples may omit the unused fourth column.

The targeted `TXTLoader::ReadUniqueSection` helper accepts LF/CRLF, trims line
whitespace, skips blank lines and full-line `;`/`##` comments, and requires one
`#RESOURCEINFO` section. Another section header or EOF ends the table. Duplicate
unrelated sections and translator comments elsewhere do not affect it. The
existing faction loader and its cache keep their previous behavior.

Synthetic native tests exercise the production parser and pass custom parsed
rules into the production center calculator. They include malformed tables,
every required numeric field, signed boundaries and section isolation; CI
needs no original assets. These tests validate this ingestion contract, not
the original game's permissiveness for malformed/custom files. Host-authoritative
loading, settings serialization and world-snapshot binding are separate work.
