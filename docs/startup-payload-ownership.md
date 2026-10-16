# Startup map presentation ownership

Before a successful `GET_MAP_DATA` response, the backend owns the terrain
texture, render mesh and data mesh. The response envelope may already contain
aliases of those pointers, and its destructor also owns any non-null presentation
fields. `Map` destruction itself does not release these three assets.

Reset and initialization failure now use one cleanup helper. It first detaches
Map pointers that alias the response envelope, destroys that envelope, and then
destroys any presentation still owned only by Map. This also handles a partially
populated envelope and failure before an envelope exists. Initialization failure
retains its previous order: release unpublished presentation before invoking the
error hook; subsequent reset is harmless.

Successful `GET_MAP_DATA` already clears both the backend envelope and Map's
presentation pointers. That transfer remains unchanged. The frontend consumes
the three pointers and clears the response fields before destroying the envelope;
cancellation of an unconsumed completed response instead lets its destructor
release the assets. Backend reset cannot reclaim those transferred assets.

Six native regressions allocate real tracked Texture/Render/Data subclasses and
observe their destructors through actual backend reset, failure, map-data request
and completed-response cancellation paths. They cover no envelope, a complete
aliased envelope, a partial envelope, initialization failure, frontend transfer
surviving reset, and cancellation owning the transferred payload. The original
production code fails the first three pre-transfer cases in the initial five-case
suite (2 passed, 3 failed); both transfer controls pass. The fixture cleans up
after those expected failures without leaving tracked allocations.

The source implementation `62c816fa13a834a2e8775713f4496f845b1fb751`
passes all six focused Linux Clang 15 cases with allocation
tracking and the full eight CTests (50.67 seconds). The separate ASan/UBSan build
passes the same tracked cases and all eight CTests with leak detection enabled
(107.69 seconds during concurrent local build activity). The configured
correctness checks report no findings for `Game.cpp` (11.18 seconds). These are
local measurements from that source branch.

Focused publication commit `7e5d7334efef1fa22ec4a59e1bffbf338c1c3599`
passes all five available Linux CTests in 22.25 seconds. Its test adaptation adds
only the fixture's isolated `ResetWorld` helper; it does not bring in unrelated
snapshot restoration code from the source branch. This older publication fixture
does not establish the source branch's allocation-tracking or sanitizer claims.

Combined integration `cd2613d12fce4613ec6e13ab4d2f3b9e50586f07`
passes Windows CTest 8/8 in 50.84 seconds, Linux ASan/UBSan/LSan CTest 8/8 in
49.10 seconds, and all 298 eligible production translation units under LLVM 15
analysis in 65.15 seconds, with zero failures. Windows-only networking is outside
that Linux analysis. No native macOS or rendered startup result is claimed.

These tests retire worlds within the fixture's existing script accumulation.
They establish presentation ownership, not native reset/collector exclusion,
staged startup synchronization, rendered map reload, or exception safety for all
map construction steps.
