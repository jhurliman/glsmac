# Native source and controls: sea landmark material admission

Pinned executable SHA256
0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01.
Public contract: behavior/sea-landmark-material-admission.md.

Raw +0xc snapshot EBP-84, loaded46fd99..46fd9e. Shared signed detail is
loaded only when mask103 is nonzero at46fe14..46fe26. Land height mask is
b03, as established in the separate admission supplement; 10 is not100.

Sea bypasses the alternate land triangle material choice at470a9b..470aa0
(sea boolean EBP-14), jumping470cba. Thus the landmark40 branch470b5a
does not apply to ordinary sea. Jungle setup4705cf..4705e8, with center
mask80000004, occurs in the land neighbor branch; later471711..471716
explicitly rejects sea before Jungle texture drawing.

Special art tests1/2/100 after47108a are not categorically gated by sea;
do not infer their absence solely from47ca10 returning sea height0.
Separate Uranium/Geothermal sprite branch471685..4716fb requires
renderer ah&2 (flag200). Tests are (word&80000008)==8 and
(word&80000400)==400. This branch is disabled in captured flags1.
Neither raw10 nor80 selects these branches or land-only40/4 branches.

probe_sea_landmark_material_admission.py reuses only the writer-free prefix
and per-case execution body of probe_sea_fungus_commands.py. It redirects
new private mask output to sea-landmark-admission-private-20261019, leaving
old mask directories and frozen archives untouched. Nine words, three sea
contexts, complete original46fd00 normal return. Whole dictionary equality
against each zero-word baseline checks anchor/gate/contour-samples/draws.
New receipt sea-landmark-material-admission-original-x86-20261019.json:
27cases,153commands; original final drawing sinks remain observation services.
No new raster stores or full-frame hashes are claimed.
