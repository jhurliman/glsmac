# Retained Base scaled-copy receipt

Pinned EXE SHA256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

Full actual5fe680 copy executes for loaded retained University land/pop1 sprite. Global ratio6b4fb8/6b4fbc is12/16 or8/16; cached ratio6b4fc0/6b4fc4 is invalidated explicitly per controlled call.604070 generates800 signed step entries at9bbfc0, compared in full with the portable recurrence. Scale is also saved as float at9beb0c; the tested ratios are exactly binary-representable.660678 truncates separately scaled retained dimensions and offset placement. The source starts at receiver+4; source pitch is receiver+c; retained width/height+10/+14 and logical offsets+20/+24.

The downscale branch5fea8c onward calls603ad0 for horizontal clip prefix sums and steps,603b60 for vertical. Compare/load PCs5fec4e,5fec6b,5fec88,5feca5,5fecc7,5fecd1,5fecdb,5fece5 observe every sampled ESI source and EDI destination. Native loops step ESI with horizontal entries after sampling; row tail5fecf2 reads the vertical entry and uses source pitch plus row correction to reset horizontal position. Key compare is before source0 test; source0 indexes the supplied destination remap, while other source bytes copy directly.

Probe `probe_base_sprite_sampler.py` composes the writer-free initialized Base loader prefix, then directly invokes5fe680 four times. It uses actual retained41×38 bytes at offset27,17 from original UNIV.PCX crop1,1,100,75. Those retained bytes equal a portable PCX decode with+10 index rebasing. Source has549 key9 and115 zero bytes. Lookup passed by prior native Base callers is uniformly0. Every sampling tuple and complete frame agrees with the portable reference. Total1576samples. Per-case attempt packing is `<HHHHB` destinationX,Y,sourceX,Y,index.

Prior selected hash e5d2754e3ec93cf99ab46e0a336b13e291c72db1cca953b6d12e130d5aa4c9e6 matches directly. Prior roster hash3893fff6465f6167cd58eb5b30a4e8bd010e258fa29d13e505b0b79158603a04 includes selected art and population border drawn earlier by4cb9f0; probe reproduces that baseline, using full native5fc003(rect,0,0). Existing TextOutA observation sink creates no glyph stores. This resolves the full-canvas hash comparison without attributing pre-existing pixels to the roster sprite.

Public scope and exclusions: `docs/behavior/base-sprite-scale.md`. Earlier13-case Base layout evidence is not rerun or expanded into unsupported sampler claims.
