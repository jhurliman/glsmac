# Original SMAC small dialog frames

Cold-launch SMAC faction selection and leader-name entry use the original plain
mode0 dimensions and positions on the logical1024x768 canvas. First-base naming
uses the console-visible mode0 frame at238,255, with its single OK control and
the two visible console rails. The first-base backend introduction gate remains
the one introduced in #414: naming completes before first upkeep or human orders.

`ui/parts/smac_dialog.gls.js` supplies these three layouts. Other popup templates
and SMAX setup retain their existing paths. Faction selection defaults to the
first configured faction and omits the extra Random row in this SMAC template;
quickstart/random admission remains available through its existing paths.

The frame API retains the original indexed representation until compositing is
finished. Source key9, destination mask7, literal center copies, clipping, tile
phase and overlapping corners follow the recorded original primitive. Loaded7
is not globally transparent. `smac_frame` copies bounded native integers out of
GSE; `smac_frame_phase` restores parent-list pattern phase for separate row surfaces.
The loader caches the immutable original atlas/common-palette source. Surface
rebuilds its owned texture only when frame calls, phase or logical size changes.

## Evidence and validation

- `behavior/small-dialog-frame-pixels.md` and its cases record the original
  primitive evidence. Eleven controls cover43,431 indexed pixels, including a
  shifted origin, clipping, overlapping corners and an initially7 destination.
- The synthetic native test distinguishes source9, destination7 and literal
  center7/9. Asset controls run when `GLSMAC_DIALOG_FRAME_ASSETS` points to the
  user's original asset directory. No game assets are committed.
- Four affected GSE scripts pass: faction selection, faction preview, faction
  identity and first-base introduction. These check functional flow and lifetime;
  mock scripts alone do not establish rendered parity.
- The cached incremental build and scoped analysis of the four changed production
  translation units pass. Focused ASan/UBSan instrumentation covers those four
  units and the new frame tests; two frame groups and three existing UI
  input/modifier groups pass. Other objects use the normal build; leak detection
  is disabled in that focused sanitizer run.
- Actual SDL/OpenGL captures at1024x768 compare with recorded original parent
  DIBs: faction frame18,973 pixels and name frame20,600 pixels have zero
  differences. First-base frame/visible rails add124,518 pixels with zero
  differences. These masks exclude caption ink, child contents, buttons and
  source-key corners. This is component parity, not a whole-screen pass.

## Remaining fidelity work

Faction name/descriptor columns now use the measured alignment and baseline;
see [the column follow-up](faction-row-columns.md). Portrait/quotation panel layout,
button text/borders, edit selection
background/caret behavior, source OTHER handling, and Change Gender followed by
the original customization continuation are not yet exact. The existing Faction
Details and Customize controls remain explicitly labeled as their current
functions. SMAX templates require their own measured contracts.

Main-map unexplored visibility and default zoom are separate open rendering
tasks. The canonical4:3 canvas is already1024x768, with a257px lower console;
that does not make the map's zoom or all console details correct. The guided
tour is not a1.0 release blocker per the user's request.
