# Ordinary zoom0 terrain view order and variation seed

The original full-view terrain pass was traced at its actual per-tile renderer call boundary. The numeric cases contain1597 ordered tile/flat-center submissions across six controls. `terrain_view_order_reference.py` reproduces every ordered coordinate, flat center and record offset. The viewport is1024×511, world80×80, zoom0, ordinary full redraw with dirty-region restrictions disabled. Source map is the pinned installed Planet.

Run `python docs/behavior/terrain_view_order_reference.py`. This check needs no owned map or pixel bytes. `terrain-view-order-cases.json` contains complete ordered lists rather than a row-order conjecture.

## Native terrain pass

The native view initializer supplies the first column/row. Their measured pairs are:

| Control / camera | First X,Y | Submitted tiles |
| --- | --- | --- |
| Interior40,40 |28,28|288|
| Round west0,40 |68,28|288|
| Round east78,40 |66,28|288|
| North pole40,0 |28,-2|265|
| South pole40,78 |28,60|230|
| Flat west0,40 |-4,28|238|

Iterate25 candidate rows in increasing row index r=0..24. For even r, iterate12 columns; for odd r,11. The unwrapped doubled-X coordinate is `firstX+(r&1)+2*column`; Y is `firstY+r`. Round maps adjust X once by world width when below0 or at least80. Flat maps retain out-of-range X, then reject it. Reject any X/Y outside the world; rejected candidates produce no renderer call. Valid candidates retain their original row/column order.

The flat center F uses the **unwrapped visible copy**, not the wrapped map coordinate:

```
F.x = 48 * (unwrappedX - firstX)
F.y = 24 * r
```

These cases have zero view offsets. An interior even row has F.x0,96,...1056; an odd row has48,144,...1008. F.y reaches576 despite the511-pixel viewport. Preserve the native padded submissions; culling to flat centers inside the viewport changes the trace. At the west seam a row advances68,70,...78,0,2,...10 while screen X keeps increasing. At the north pole the first two candidate rows are rejected; the first valid row has F.y48. At a flat west edge, rejecting negative X leaves the first valid tile at F.x192 rather than shifting it to0.

The native coordinate projector also supplies the terrain anchor `P=(F.x-48,F.y-24-meanOuterHeight)` for each submission. The receipt records both native F before height subtraction and actual P passed to the renderer; their relation is asserted for every call, including the raw Planet relief. This does not substitute an approximate height model.

The terrain pass precedes the separate object pass. Per-tile terrain rendering still owns its previously closed base/Rock interleaving, Forest/Fungus and improvement continuations. This receipt observes the call boundary with that per-tile renderer declared as a sink; it does not independently establish fog/material admission, whole-scene pixel occlusion or object/name/UI pass order. Incremental dirty-region redraws, other zooms/world sizes/viewports and tiny-map centering remain unsupported.

## Exact address mapping and variation input

Valid tiles use doubled-X parity: `(x+y)%2==0`. The native tile record address is:

```
recordAddress = mapBase + 44 * ((worldWidth/2)*y + (x>>1))
seedLow16 = recordAddress & 65535
variation = (((seedLow16*1664525 + 1013904223) & 65535)*2) >> 16
```

For the standard80×80 map this is3200 records in a contiguous140800-byte allocation. The normal map loader frees the previous buffers, allocates this block and stores the returned pointer directly. No64KiB alignment, fixed address or low16 normalization appears in the map-allocation code. Its CRT large-allocation path calls the imported Windows `HeapAlloc`; a different heap address with different low16 bits can change material variation. Changing only the upper address bits cannot.

The executable reference exposes `material_variation(base,x,y)` with an explicit base. As a derived diagnostic, tile40,40 has variation1 for base0x40100000,0 for the also16-byte-aligned base0x40100010, and1 for0x40110000. These are formula checks, not observations of Windows allocation variability. Do not claim that an invented coordinate hash reproduces the original. A coordinate/record-offset implementation is equivalent only when its explicitly chosen base low16 matches the target native run.

### Allocation observation limit

Two complete executions of the original normal map loader were captured with the existing deterministic bump-allocation and no-op-free services. The public receipt labels them **controlled allocator observations**: bases0x40100000 and0x40147e80, with identical loaded-map hashes. They prove direct propagation of allocator return through the normal loader path. They do **not** constitute two natural Windows game loads or establish real run-to-run base stability. Existing receipts replace allocation, and no live terran process was available for natural-load observation. Actual Windows game heap base low16 remains unmeasured.

For integration, preserve an explicit reference variation-base-low16 input alongside the proven address layout. Matching a captured native scene requires measuring that run's base. A deterministic port convention may choose one explicitly, but its run-to-run pixel identity with the original remains unproven. No change to the selector's supported-domain claim is implied.

Frozen evidence: `tmp/reference/terrain-view-order-evidence-20261019`; earlier archives remain unchanged. Broad reverse engineering remains paused.
