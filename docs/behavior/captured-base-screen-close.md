# RE-SMAC-CAPTURED-BASE-CLOSE-001: construction blocker before close input

**Close-without-edits and the final movement return remain unproved.** The
bounded continuation reproduces the [frozen captured-Base entry](undefended-base-entry.md)
exactly, executes the screen's native Base selection and recalculation, then
stops at an unconstructed window object's virtual call. No close input is
delivered, no window method is replaced, and no gameplay edit is supplied.
The [literal observation](captured-base-screen-close-cases.json) is a blocked
research result, not an expected successful move return.

The subsequent [static return map](capture-return-tail-static.md) traces the
post-screen caller path and identifies close/hide code without executing or
substituting the missing window. Its conditional result1 is not a new runtime
return fixture.

There is also a correction to the earlier interpretation of this boundary:
capture passes screen mode0. The inspected screen function skips its modal
wait loop when that argument is0. Therefore reaching this screen function
does **not** establish that capture must wait for a Base-management choice or
for the screen to close. Its construction/show path must first finish; that
is the missing part in the present headless fixture. Whether and how the
subsequently displayed screen is closed is a separate unresolved input path.

The actual continuation reaches, in order:

1. Screen prologue and original Base selection.
2. Native Base recalculation in mode0, through normal return.
3. Original presentation setup and window-state query.
4. The screen's virtual construction/show call, which resolves to an invalid
   null target because this fixture has no constructed Base-window object.
   The inherited invalid-PC guard stops execution there.

The existing presentation/logging service is used; no new display service,
close result, window-construction result or successful screen return is
invented. Passing this point would require establishing the original window
object/virtual methods and their construction dependencies, beyond the
existing headless services. This bounded request stops at that concrete blocker.

At the stop, all three complete Base records and all three complete Unit
records remain byte-for-byte equal to the frozen screen-entry state, despite
the actual recalculation executing intermediate writes. Other tracked fields
also remain unchanged:

| Observed state after recalculation, before failed window dispatch | Value |
| --- | --- |
| Actor position / spent movement / home | (20,16) /3 /Base0 |
| Selected / secondary Unit | Actor1 /none |
| Captured Base current /former owner | Attacker /defender |
| Population /assimilation | 1 /50 |
| Production /queue /minerals | Scout /empty /0 |
| Previous item /mineral baseline | Stockpile Energy /0 |
| Facilities | Perimeter, Bank, Node |
| Remote defender Former home | Remaining defender Base2 |
| Treasuries, attacker /defender | 200 /100 |
| Technologies | Ecology /Ecology |
| CRT /shared RNG state | 1951609278 /3 |
| Additional RNG calls | 0 /0 |

These are the last observed values, **not final post-close or post-movement
values**. The screen return, capture return, outer movement result, post-return
selection and any close-handler effects have not executed. There is no valid
movement return code to copy from this blocked run.
