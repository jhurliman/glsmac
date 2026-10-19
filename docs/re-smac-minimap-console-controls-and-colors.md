# Private audit: minimap console controls and coarse colors

Pinned GOG `terran.exe` SHA256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`. Public handoff: `behavior/minimap-console-controls-and-colors.md` and cases JSON.

## Source and executed boundaries

`4660d0..4668ab` constructs1024×768 console geometry. Main console base is`7cb7c0`; +cfc is minimap rectangle`[780,88,1019,207)`, +d0c is lower information rectangle, +a8c/+a9c are the54×32 World/Detail button rectangles.

`probe_minimap_buttons.py` reruns that prefix, then admits original`466a20..46719b` atlas construction and`46719b..46726b` secondary geometry. It skips native window creation between prefixes. Some unrelated initial atlas extractions in this admitted fragment have stale register inputs because earlier skipped window/geometry instructions would set them; **they are not accepted as evidence**. The six relevant later map sprites set all their inputs explicitly and match the installed atlas and port crops. The probe then admits`4676ef..4677e1`, observing `621620` window/button construction and`6219a0` tooltip services as sinks. All rectangle, control-ID, sprite-pointer, and tooltip argument construction executes original instructions.

Map sprite extraction is the original loop`466f98..467000`: x532/587, y143/176/209,54×32, key9, from interface.pcx. Receivers are console+21c74/+21ca0/+21ccc and +21cf8/+21d24/+21d50. Original`467721`/`46779a` construct1006/1007 at904,214 and960,214. Their tooltip handles are labels+4c8 (`World Map`) and+f20 (`Detail Map`). State-sprite pointers are assigned at`467744..467768` and`4677bd..4677db`.

`4699b0` group handler receives3 arguments; input group1 with third arg1006/1007 takes the ordinary map selection branch after subtracting1000. It calls complete`4d99b0` for World or`4d9970` for Detail. These write global overview object`8f3c68`+239cc=0/1. Group selection`643c00`, detail-view virtual show/hide through`7f0b58`, overview show`606850` and hide`606b50` are presentation observation sinks. A supplied fake detail-view virtual interface models only those presentation boundaries. The complete refresh`4d99f0` takes both retained values and reproduces the same selection/visibility pair. Four sequence calls and two refreshes return normally with exact original stack cleanup asserted. No gameplay decision result is substituted. An early exploratory hide-service cleanup was wrong; the accepted final probe uses no argument cleanup for hide and checks ESP, superseding that failed run.

The full constructor's established pair is not evidence for the requested four-control reference. Installed labels+91c/+920/+924/+928 are Physical/Climate/Rainfall/Political strings; discovery does not establish their console binding. Do not map them to these two controls. No changed executable is assumed without identifying it.

## Native color and knowledge decisions

`probe_minimap_palette.py` loads original rules/labels and explicit80×80 retained world backing using the console harness. It executes the original primary/secondary slot-color producer`45f6b9..45f72f`, including PALETTE.PCX loading and native pixel samples/stores into`6aeeb4` etc. This replaces initial exploratory manual table sampling. Primary slots come from x580+8*slot,y23. Host software palette/gamma100 comes from established title/native palette harness.

The28 accepted calls execute full`46fd00(target,view0,zoom−14,viewer3,0,flags,x8,y8,anchor10,10)` to normal return. Only final polygon fill`5f3480` is an observation sink. Target vertex/color arguments and loaded RGB entries are recorded, and cdecl stack cleanup is asserted. Coarse base flag80000000 makes its flat-color branch execute. Inputs add10h for the moisture palette or10000h for territory. These are supplied renderer controls, **not executed button-command bindings**.

Important source locations:

- `46ff00..46ff6f`: retained band, moisture, and viewer-visible improvements backing.
- `47072f..47077b`: initial survey/revelation/exploration admission; unknown ordinary tile can return0 without filling.
- `470944..4709b6`: political color eligibility, faction status200h and tile exploration bit, then actual `4f97a0(viewer,x,y,0,0)`.
- `470d1f..470d87`: eligible positive territory→primary slot table; otherwise coarse moisture (`149+2*bits`) or altitude (`2*(14-band)`).
- `47102f..471089`: known coarse sea→index190; final polygon fill and complete early return.
- `4f97a0..4f984f`: signed tile+7 claim; nonpositive→−1; own positive→owner; ordinary foreign requires runtime relation mask08000008; editor80h bypasses recognition. No nearest-base query is requested because the optional output pointer is0.

Tile+4 is the viewer exploration mask. Tile+7 is supplied current retained claim, already established by full land/sea territory rebuild contracts. Faction runtime status at`95d1a8+viewer*20bc` bit200h supplies all-map revelation. Relation entry is`95d1c0+viewer*20bc+owner*4`. Initial-survey primary rule bit100h is in`959fa4`; standard menu provenance is independently completed. Renderer/editor byte959fc8 bit80h is supplied only for its named diagnostic case.

Results discriminate own/foreign/unclaimed/native claims; each recognition bit alone vs both; unknown/no survey; initial survey without territory disclosure; all-map revelation without foreign recognition; editor recognition bypass for known territory; all eight physical bands; moisture0..3 (3 diagnostic); political/rainfall sea. These are explicit fixtures, not natural diplomatic/history producers or whole-frame screenshots.

Overview draw`4d9ef0` separately source-maps its default view+1dd70 assignment to80000000 and its bitmap sizing/copy path. The public contract deliberately does not claim that native World/Detail clicks select10000h or10h. Whole bitmap scaling/presentation and exact four-button row remain open.

## Evidence

Frozen audit snapshot: `tmp/reference/minimap-console-controls-and-colors-evidence-20261019/manifest.json`.

SHA256 `e45ceb67957951be2b5e9e36abde0257a970b986e59fac1cd879cba6de220d94`.

Thirty files: new probes/receipts/logs, immediate harness sources, relevant original listings, finalizer/assertions, public cases. Nested earlier harness dependencies remain in the working tree/prior immutable bundles; original owned executable/art assets are not copied. Original pixel colors are observed, not invented. No production source, old frozen bundles, territory producers, or broad paused goal changed.
