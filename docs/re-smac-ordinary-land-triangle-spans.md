# Native vertex-to-span translation

Pinned EXE SHA2560a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01. Portable generator translates635af0 +63a590 +642890.635bd2..635c18 strict min/max tie selection;635c60/635c7d initialize left/right chains;63a590 skips horizontal edges, rejects downward rises, uses quotient/remainder X error and signed fixed-point UV/light steps.635d01 snapshots current states before advancing both chains;635e13..635e37 computes current-row derivatives from x87 temporaries;635e7e..635f45 clips;635f80 shifts shade to8.8;63600d actual store observation.

FNINIT and rounding-control0xc00 give64-bit significand precision with chop; portable rational implementation rounds reciprocal and multiply independently. Signed16 FISTP invalid emits8000, verified by synthetic clamped-light overflow. General int32/FISTP32 overflow is excluded.

Asset-free probe runs actual635af0 and its63a590/642890 callees with memory-backed64x64 constant texture90 and32x32 canvas. Only5fc173 bitmap acquire and5fc2a3 release are supplied. All pixel writes execute.36 cases are observed at63600d and grouped into358 literal spans. Combined with immutable prior16 triangle/401-span receipts, all759 match generator values exactly; generated original spans also reproduce all8,967 indexed pixels/four hashes.

Reproduce probe_triangle_span_edges.py, then finalize_triangle_spans.py. Native binaries/assets stay outside production. Immutable evidence: tmp/reference/ordinary-land-triangle-spans-evidence-20261019.
