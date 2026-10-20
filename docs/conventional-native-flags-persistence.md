# Conventional native visibility flags persistence

`ConventionalUnitState::native_flags` retains the original 16-bit Unit field at
+8 separately from the DWORD status at +4. The original actor admission reads
both fields independently: the flags pair `0x440` changes the ordinary own-unit
visibility exception. This storage must not be derived from accounting markers,
orders, source-pool links or facing.

Conventional Unit10 appends an optional word after the Unit9 facing block.
The writer selects Unit10 only when flags are present, including an explicit
zero. Existing Unit4/6/7/8/9 records retain absent flags and their original wire
format; migration does not invent ordinary-owned visibility authority. Reads
check the integer range before narrowing. A legacy state-codec request rejects
silently dropping a known word. State equality includes the optional word.

This is persistence infrastructure, not completed actor admission. Fresh factory
initialization, disclosure/upkeep mutation events, frontend transport and the
ordinary main-map consumers still need their separate integration. In
particular, absent flags do not authorize treating an imported unit as flags=0.

Validation: cached incremental build; five focused native groups covering actual
Unit save/load, all older supported conventional versions, zero, isolated 0x40
and 0x400, combined 0x440 and 0xffff, malformed -1/65536, old-codec refusal,
facing, stack and DWORD accounting independence. Correctness analysis passes for
both changed production translation units. The same five groups pass with
ConventionalUnitState.cpp, Unit.cpp and ConventionalUnits.cpp instrumented under
ASan/UBSan; other objects remain uninstrumented and leak detection is disabled.
No visual behavior is claimed by this PR.
