# Native tile assembler validation

`tools/reference/verify_native_assembler.py` compares the production
`NativeTerrainRenderer::Render` against 486 original full-viewport indexed
frames: 358 named quad/marker contexts, 76 Jungle/Dunes contexts and 52
resource/Pod/Monolith contexts. ASan/UBSan passes.

Unlike captured-command replay, this probe supplies original tile inputs and
calls production geometry, material, road, river, sprite selection and ordered
batch assembly. It injects no captured draw commands. The original resolved
nutrient totals remain explicit inputs; nutrient calculation and actual backend
snapshot transport have separate native/backend tests.

An injective diagnostic palette recovers exact palette indices from the public
RGBA result. It affects only the final palette conversion. The retained native
shadow lookup remains unchanged. Both complete frame hashes and attempted/stored
pixel counts must match the independent original records.

The original offscreen captures initialize their indexed destination to 250.
The probe supplies that same initial index explicitly. Four mineral cases have
shadow pixels outside the terrain polygon and distinguish this state from an
initial index of zero. The inspector's existing zero-initialized transparent
layer remains unchanged; coverage continues to identify submitted stores.

Run with purchased assets outside the repository:

```
python3 tools/reference/verify_native_assembler.py --asset-directory PATH --cxx clang++ --sanitize
```

Use `--output-dir PATH` to retain generated inputs, the probe executable and
indexed frames for diagnosing mismatches. No original asset pixels or binaries
are committed. The separate captured-command replay covers all 499 frames,
including the 13 smaller-clip controls.

This proves these bounded single-tile assembler contexts. It does not prove
whole-screen parity, original display-option persistence, backend gameplay
nutrient calculation, complete standard-Planet cache traversal, alternate
style/fog or scaled pixels. In particular, indexed shadow composition over
unsupported legacy/GPU fallback regions still lacks an authenticated indexed
backdrop; this receipt does not close that integration boundary.
