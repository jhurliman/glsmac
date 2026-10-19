# Detail minimap painter evidence

Public contract: [Detail painter](behavior/detail-minimap-painter.md).
Pinned terran.exe SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

Actual constructor46f110(1), layout4d9a90 and full47d630(detail,1,0)
initialize flags80000082, zoom-7 and rectangle780,88,1019,207. An explicit native
5f0010/5f0470/5f76f0 bitmap provides239x119 indexed pixels with pitch240 and active
palette. Full476da0(0) returns normally through4764f0,46fd00,5f3480,63fd90.
No added draw sink supplies terrain geometry or flat-raster results. Inherited
window services supply native backing dimensions. Initial backing is explicit.

The high flag bypasses ordinary5ee070 clearing. Unknown geography leaves backing
unchanged. Normal viewer comes from9430d4; terrain admission includes tile+4
exploration, initial-survey959fa4 bit100, or faction95d1a8+viewer*20bc bit200.
Polygon edge helper63fbe0 and filler63fd90 retain signed integer edge phase,
strict-positive error carry, and right/bottom minus-one exclusive clipping.
The terrain receipt checks11 frames,410 fills,8569 spans,217303 CPU byte stores.

Base supplement removes inherited5719b0/56fd50 supplied-return hooks and replaces
the inherited TextOutA stub with the generic current-Windows GDI service. Full
45eb70(3) and45ee50 load/bind UNIV sprites; native palette-slot producer and
shadow-table initialization execute. Full51fc50 facility checks and5719b0 execute.
The unrelated actor/cache4297e0 presentation boundary remains supplied.

473290 requires active game944608. With80000000 and80 it calls4737b0, then473373
returns before the unit-model path. 4737b0 uses actual4f9320 Base lookup. Its
geography gate excludes survey; ownership or Base+10 display-faction visited bit
is independently required. 4743e0 name flags800 and473f70 overlay flags40000 are
absent. Normal display/renderer viewer3 only is claimed.

Base arguments are bitmap,Base0,projectorX,projectorY-14,zoom-7,flags0. Full5719b0
selects actual sprite metadata. Explicit95a030=-1 prevents a PE-default0 CDF false
positive; own_cdf explicitly sets0. Diagnostic implicit-project and pre-clipping
receipts are superseded, not accepted. 5fe680 at5fe9bb..5fe9e5 adds anchor to
scaled retained offset before660678 truncation. Negative-coordinate cases expose
the difference from anchor+trunc(offset*ratio). 604070 supplies9/16 sample steps.
Seven admitted Base calls,2663 source samples and seven text requests match all
13 complete indexed frames. CPU stores are captured diagnostically; this
supplement claims per-sample and full-frame equality, not complete CPU-store order.

Native population requests retain Arial LOGFONT, baseline/clip/color/alignment.
overlay_host_gdi.py handles actual TextOutA against an8bit DIB with SaveDC,
IntersectClipRect and RestoreDC. Layout decisions are native; host font pixels
are an explicit service boundary. No historical Win98 font-pixel claim.

Verifiers: tmp/reference/check_detail_minimap.py and
tmp/reference/check_detail_minimap_bases.py, both --verify-only. Inputs, source
exports, harnesses and owned pixels are private under tmp/reference. Public cases
contain numeric behavior and hashes. The terrain and Base claims remain separate.

Frozen evidence: tmp/reference/detail-minimap-painter-evidence-20261019,
67 files, manifest SHA256
`f6c0980cab38c3832f2427e381e2c6b6383d9b9450234de9fbdea78cbafc5f76`.
This pointer was added after freezing and identifies the captured source versions.
