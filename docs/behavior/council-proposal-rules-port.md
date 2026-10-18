# Owned proposal requirements

`SmacCouncilRules` owns the eleven prerequisite columns of stock SMAC's common
`#PROPOSALS` table. Supreme Leader consumes row2, column2 through the frozen
technology catalog, not a hardcoded technology ID. The
[native reader receipt](proposals-requirement-reader.md) and literal controls
have SHA256 `51c0d34de6da28115fc58a13ae1e2ff224941ffa9d6470f8c70ad84bc210addd`.

The profile preserves each trimmed source token. The existing reference
resolver provides None/Disable, case-insensitive whole-key lookup and first
positional match. Only definition storage truncates keys to seven bytes;
lookup tokens are not truncated. Display labels and effect descriptions do
not determine identity and are not retained by this requirement-only profile.

Host binding stages and freezes the optional profile with the other source
rules. GlobalSettings V19 writes its bounded suffix. V17/V18 restoration and
older sources without `#PROPOSALS` retain explicit unavailability; loading
does not read today's installed file to invent older saved authority. A frozen
host rejects a changed or removed profile before publishing mutable settings.

## Ingestion boundary

The port deliberately uses strict table ingestion: exactly eleven records,
required prerequisite columns, nonempty bounded tokens and no unknown
references. The original unknown-reference notice can return−2; this patch
does not implement that diagnostic modal and does not relabel unknown input as
explicit Disable. Valid installed records follow the measured reference path.
Missing authority is distinct from a loaded None requirement.

## Validation

Actual installed `alpha.txt` supplies the complete technology catalog and all
eleven proposal rows; the new reader matches every native resolved requirement
`−1,41,77,61,61,66,66,62,17,17,−2`. This optional asset check is registered only
when `GLSMAC_TEST_ALPHA_ASSET` names an installed reference; CI's ordinary
source/codec tests do not need proprietary assets.

Native tests cover positional identity, case/whitespace, None/Disable,
untruncated overlong references, missing/duplicate/malformed tables, profile
roundtrip, frozen host rejection, and V17/V18 absence. Existing majority and
pressure controls remain green. Affected settings/host/Client rejection,
serialized restore, first-Base and resource/production/world-size checks pass.
Six affected production translation units pass correctness analysis. Focused
ASan/UBSan checks instrument the proposal and Settings implementations, with
other current objects normal and leak detection disabled.

This supplies catalog authority for the upcoming live strategy/dominance
caller. It does not implement council elections, proposal effects or SMAX.
