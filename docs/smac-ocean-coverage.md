# Ocean coverage as a retained restart input

The original restart search uses the selected ocean-coverage category K=0..2,
as established by the [producer receipt](behavior/restart-input-lifetimes.md).
GLSMAC already represents the three menu presets as exact binary32 values
0.4/0.6/0.8. `GetSmacOceanCoverage` maps those saved representations to 0/1/2;
every other value remains unclassified. It never measures water tiles or rounds
an arbitrary continuous value into a category. No save-format change is needed.

SMAC random menu setup now chooses one of those three presets. Custom setup
and lobby choices already write the same values. Quickstart preserves the
selected SMAC coverage instead of replacing it with a continuous random value.
An explicit command-line ocean override still uses the requested fraction;
nonpreset overrides have no established original category. The session lock
rejects subsequent script/peer changes to coverage as well as world geometry.

The current generator and setup random streams remain GLSMAC policies. This
does not reproduce original world-generation or new-game RNG provenance, and
does not yet replace the live restart-placement approximation. It supplies the
configuration input for that integration without manufacturing historical data.

Validation: cached Linux Debug build; 20 affected native settings, menu/lobby,
first-Base and resource groups with memory debugging. The two menu groups execute
the actual GSE randomization module against native SMAC/SMAX settings; they were
rerun after correcting GSE's indexed-call syntax, reusing unchanged native
successes. Preset categories survive the existing codec, adjacent continuous
values remain unknown, and rejected locked updates preserve the prior bytes.
LLVM15 correctness analysis passed nine changed/affected production translation
units in 10.81s. No ownership changes or additional platform/sanitizer matrix.
