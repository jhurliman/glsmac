# Native generation noise

`SmacGenerationNoise` implements the original constructor at `0x5d9d70` and
evaluator at `0x5d9e60`, for the contour-morph stage of native world generation.
Construction consumes 324 CRT draws, including the unused outer rows and columns.
The effective grid repeats its first row/column at index 16. Evaluation uses
the original integer interpolation, not a floating-point noise substitute.

The three original executable controls cover all 289 grid entries each, final
CRT continuation, and 135 samples around negative coordinates, interpolation
boundaries and the 4096-coordinate period. The native test also checks the
ordered constructor draws and that evaluation leaves both random streams alone.
Signed coordinate arithmetic is widened before subtraction, and floor division
replaces implementation-dependent negative shifts.

Evidence: executable SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`;
frozen `native-pretemperature-generation-evidence-20261020` manifest SHA256
`3ccbd8480e7ae54006bc663cb1336385046cf60e33d9897da2c097352d48c3d3`
(32 files verified before import); original compressed numeric receipt SHA256
`d2ee395a2ddc97886fd47267fa04cceb44810c231668edcf70cb5b0e132403dc`.
The compact fixture contains numeric observations only; no purchased artwork or
executable is committed.

This primitive is staged alongside native bootstrap and continent growth.
The remaining morph, region rebuild and retry orchestration must be connected
before replacing the live procedural generator. This change does not alter
current gameplay rendering; no new screenshot is warranted.
