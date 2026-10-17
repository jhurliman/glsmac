# SMAC RESOURCEINFO parser

`loader::txt::SmacRulesTXTLoader::ParseBaseSquareRules(source)` returns the
Base Square, Bonus Square and Recycling Tanks nutrient/mineral/energy triples
for the existing SMAC center calculator. This is a pure source-text parser:
it does not select a file, read assets, cache rules, change session settings,
or activate the calculator in gameplay. It makes no SMAX compatibility claim.

RE-SMAC-BASE-001 identifies a nine-row RESOURCEINFO table with four signed
32-bit values per row. RE-SMAC-BASE-ROWS-001 connects the original positional
loader to the center calculations: positions 2, 3 and 5 supply the three needed
triples. Labels do not select identity. Renamed or repeated labels are accepted;
moving a complete row changes its meaning. The previous English-label guard
incorrectly rejected these inputs and has been removed. Missing or additional
rows and empty labels remain ingestion errors.

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
the original game's permissiveness for malformed/custom files. The separate
[host-rules integration](smac-host-rules.md) handles session loading and settings
serialization. World-snapshot binding is documented in the separate
[source-pinned world-header contract](https://github.com/jhurliman/glsmac/blob/445ee2094331babdab7e82528eb00f028a65ff26/docs/world-snapshot-rules.md).

The positional correction has two direct regressions: arbitrary/repeated labels
preserve all triples, and swapping complete Base/Recycling rows changes their
identities and the nutrient-resource calculation. Against the previous parser,
the eight-group selection passed six groups and failed those two assertions,
with exit 1 and no tracked leaks. The corrected selection passed all eight
under ASan/UBSan/LSan with allocation tracking. The research handoff reports
240 original-executable comparisons; this focused implementation check does
not claim to rerun all 240 or to establish malformed-text behavior in the
original reader. Initial test-authoring compile errors and an uncaught rejection
in the first negative harness were corrected before these reported runs.

The final correction, including empty-label rejection coverage, passes the full
integrated Linux ASan/UBSan/LSan CTest suite: 8/8 in 65.43 seconds. This broader
integration result is separate from validation of the focused publication tree.

The exact focused publication tree `e1c8396e` also passed full Linux CTest 5/5
in 26.65 seconds. Full integration correctness analysis checked 307 production
translation units with LLVM 15.0.7: zero failures in 238.80 seconds (two workers).
That scan includes the integration's additional technology code; it is not a
claim that the smaller publication contains those systems. The publication
script's final log-display command initially failed on a CRLF path after CTest
had returned success; the completed test log is retained and the display script
was corrected without rerunning unaffected tests.
