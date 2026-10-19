# Native ordinary land selector/palette execution

Pinned EXE SHA2560a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01. Native TEXTURE.PCX5f0be0 and45d8df..45dd16 execute all634010 atlas extraction calls (160 banks). Native46fd00 entry..472ba6 executes selector/composition with raster observation sinks. Thirty primary contexts and seven supplements; no closed climate/grid/span suite rerun.

Byte0>>3&3 moisture,byte5>>6 rockiness at46ff18..46ff37. Moisture neighborhood4705fb..470707 uses690ca8/690ccc ordered offsets, bit i reads offset[(i−1)&7], sea connects unconditionally, land connects if its moisture>=center.6a8454/55 provide256(pattern index,rotation) pairs.4707e8..470881 builds rotated UVs and crop selector. Base banks7866a8 dry,785fa8 moist,7858a8 rainy.470ac3..470b24 Farm mask8000/flag1 selects782ab8+70*(3*localBC+moisture).470c64..470cb5 interleaves7867f8+70*(localBC+2*rockiness−2) rock triangles.

633010 initializes local RNG;633030 sets it to the passed tile-record address (46fdc7).633050 LCG1664525/1013904223, low16 scaling, exclusive high bound;470791 observes actual return in supplemental controls. +10000 map-base shift preserves variation; +4 flips1→0 and changes actual Farm/rolling crops. This is an unresolved original allocator-base dependency, not a global simulation RNG or coordinate hash.

Forest47183e..47193e uses4 diagonal offsets690c80/690c94 and live forest bits to index7823b8. Fungus471943..4719fb uses neighbor mask8c/6a8454/55, land782ea8 and63e2e0→635060 unlit keyed shader.636990 and63d6b0→636990 are keyed shaded; key comparison precedes ADD AL,CH. Native draws retain all argument UV/light/flags and crop bindings.

Palette reuses original617e10(0) initialization,5f0be0(PALETTE.PCX,959aa8,10,236),5e1020 gamma1 and617690 upload from established title/data harness. Active959aa8 is unchanged by TEXTURE.PCX load with null palette destination.5f76f0 maps the same active table onto the actual indexed target DIB.236 common colors match min(source,235);256 DIB BGR entries reverse to native RGB. No full owned palette is placed in public files. Gamma formula authority reused from the prior title pipeline rather than re-surveying it.

Probe files: probe_land_texture_selectors.py and probe_land_texture_selector_supplement.py. Finalizer independently checks portable crops/UVs/order and palette conversions. Immutable evidence: tmp/reference/ordinary-land-texture-palette-evidence-20261019.
