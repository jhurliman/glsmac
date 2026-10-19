# Keyed land raster receipt

Pinned terran.exe SHA256: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

Actual full raster entries: opaque shaded635af0, keyed shaded636990, keyed unlit635060. Shaded edge helper63a590 starts UV with8000 bias; unlit helper63f680 starts without it. Both use642890 fixed-point division and native x87 horizontal derivatives.

Sampling/store observation addresses: opaque63600d; keyed shaded compare636eb1 and store636eb7; unlit compare6354e5 and store6354e9. Shaded source compare precedes636eb5 ADD AL,CH. Source pointer is EBX+ESI wrapped32-bit; destination is EDI. The probe records every attempt, actual store, numeric span start/steps and complete target frame. Ordinary shade limits6b5564/6b5568 are-4/+4; key byte9c44c8 is9.

`probe_keyed_land_raster.py` reuses the writer-free full native TEXTURE loader/atlas initializer from the selector probe, removes selector observation sinks before raster execution, creates six native hill/coast producer plans, and reuses prior flat/UV plans. Forest direct636990 and wrapper63d6b0 keyed shade are replayed through636990; Fungus wrapper63e2e0 through635060. Native bitmap acquisition/release services remain initialized. Target1024×511 pitch1024 is supplied with background250.

Results:27frames,3332spans,100917attempts,71562writes. Portable reference consumes vertices/UV/lights/loaded crops, verifies every native span and packed attempt/write SHA, then complete frame SHA. Attempt record `<HHIhBB` is x,y,sourceOffset,signedShade,loadedIndex,willWrite; actual store `<HHB` is x,y,result. Private owned pixel streams are excluded from public cases. Rotation/shape cross-product is replay, not a new natural-session claim. Public scope and exclusions are in `docs/behavior/ordinary-land-keyed-raster.md`.
