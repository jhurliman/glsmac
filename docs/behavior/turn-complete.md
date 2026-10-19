# Original SMAC Turn Complete control

The original uses the complete **`turn.flc` indexed animation**, not a crop of console_x2_a.pcx. It has233×23 frames,19 declared frames and a20ms header speed. The actual interface caller advances it on a requested100ms service and on specific redraw events; it does not use that20ms header speed or an opacity oscillator.

The [numeric cases](turn-complete-cases.json) and [portable reference](turn_complete_reference.py) cover8 display states,42 consecutive real-asset frame decodes,5 interactions,7 edge hit tests,6 timer gates and4 local activation controls. All match the native outputs. Optional external-art replay also matches all42 complete native indexed frame hashes:

```
python docs/behavior/turn_complete_reference.py
python docs/behavior/turn_complete_reference.py --turn-flc PATH/TO/turn.flc
```

The second command reads the user's original asset; no art/palette payload is embedded in the cases. The decoder is bounded to this pinned asset and its observed chunks.

## Artwork, placement and text

For1024×768 with console origin(0,511), the full233×23 image/control rectangle is console-local **[783,61,1016,84]**, screen[783,572,1016,595], half-open. Source rectangle is[0,0,233,23] for every decoded frame. No normal/hover/pressed PCX crop bank is selected by this native control. Its animation writes indices directly with key0/background restoration; it does not fade the whole control. The portable decoder preserves a supplied backing image for source0 and verified the measured index100 backing in all42 frames. Other backing profiles are not covered by that pixel comparison. Apply the established active common palette after indexed composition; the embedded FLC palette chunk does not replace it in this route.

The label is uppercased through the native string operation. Font is **Arial bold700, requested height-14**, not Arial Narrow17. Current GDI resolves text height16, ascent13, descent3, internal leading2. The original font object's centering/draw height is11; preserve that distinction from requested height and full text height.

| Label | Whole-string width | Console X / baseline Y | Screen X / baseline Y |
| --- | ---: | --- | --- |
| TURN COMPLETE |119|840 /78|840 /589|
| WAITING FOR OTHERS |154|822 /78|822 /589|

Native horizontal placement is `left+trunc((rectWidth-textWidth)/2)`. Vertical text origin is `top+trunc((rectHeight-11)/2)`; the primitive adds11 for the baseline. Color is game index101, measured Gamma100 RGB **(228,104,24)**, transparent text background, baseline alignment24. No label color/alpha change occurs in the measured hover or completion states. Glyph raster identity depends on the host font; the fixtures verify current-GDI widths and native layout, not historical font pixels.

## State and animation gate

The painter does not scan unmoved units itself. It reads existing turn/command state. In ordinary local mode it advances the animation only when **scenario flags bit2 is set AND the command-loop pending latch is nonzero**. Otherwise each redraw rewinds and displays the initial frame; the label remains TURN COMPLETE. Treat the not-ready flag state as a static display, not a disabled orange-opacity variant. Automatic production of the flag from a complete unit-selection loop is outside this receipt; connect the engine's established readiness/command-state authority.

The portable reference retains exact numeric gates for network modes too. Let complete=flags&2, pending=command latch, network=nonzero network mode, simultaneous=network flags&16, mask=completed-player mask:

```
animate = complete
          && (!network || !simultaneous || activeFaction == viewer)
          && (pending || (network && mask != 0 && mask != 255))
waitingLabel = network && complete && !simultaneous
               && mask != 255 && (mask & (1 << viewer))
```

WAITING FOR OTHERS is selected only by waitingLabel; all other measured states use TURN COMPLETE. The cases distinguish zero/nonzero/all255 masks and simultaneous own/other active faction. A distinct initial view/mode admission guard can suppress the entire painter; its full UI-mode lifecycle is not newly closed here.

## Hover, activation and disabled behavior

Hit region5 is exactly the image rectangle: left/top included, right/bottom excluded.

- Entering the region sets a hover latch and calls the ordinary status redraw. Moving again inside leaves the latch set and causes no additional status redraw. Leaving clears the latch and redraws once. When animation is active, each such redraw advances one frame. Both measured redraws retain the same label/font/color; there is no measured0.5 hover alpha.
- The activation callback inside region5 dispatches the original End Turn command; outside does not. That callback publishes no separate pressed artwork/color/alpha state. The traced route supplies no0.8 pressed-alpha contract. This does not claim that all Windows press/release dispatch or other UI widgets have been surveyed.
- Command availability is checked by the command handler, separately from painting. Outside its active command loop, activation leaves completion/pending state unchanged; no distinct disabled-art branch appears in this painter.
- For an active local command with completion clear, the handler runs the selected-unit admission/confirmation path. In the bounded admitted prompt controls, Cancel preserves flags and the pending command; Confirm sets bit2 and clears the command-loop/pending fields. When bit2 is already set, the handler skips that prompt and clears those fields. Unit admission predicates and cleanup helpers are explicit supplied services in these activation controls; the complete gameplay command loop is not reimplemented or revalidated here.

## Timing and phase

The shared interface service requests100ms, independent of the selected-unit150ms blink. Modal-dialog readiness A/B, hit/effect hold, shutdown, or the global interface hold prevent the callback body: no status-counter increment and no frame advance. Each admitted callback increments the service counter once and invokes the same status painter. Delivery uses the already established pending-message/coalescing mechanism; elapsed delay does not cause frame catch-up.

An active painter invocation advances exactly one frame; an inactive invocation rewinds before drawing frame0. Direct status redraws and hover entry/exit can therefore advance independently of timer delivery. Repainting the port's whole screen at60fps must not accidentally count as60 native status-redraw events per second.

Initial playback consumes physical frames0..18. At declared count19, the original seeks to the **second physical frame**, resets its counter, and decodes there. Steady playback consumes physical frames1..19; frame19 is the empty16-byte ring frame and holds the previous pixels. The42-step receipt covers both wraps and that hold. With only uninterrupted100ms deliveries, a steady cycle takes19 deliveries, nominally1.9 seconds; direct redraws change its phase. Hover does not restart a fresh elapsed-time fade. Header speed20ms is not the scheduling authority of this caller.

## Evidence boundary

Original console geometry, font construction, hot-region registration, FLC loading/decoding, full status painter, timer gate and local activation branches execute. Native window creation/background setup and final presentation are supplied services; TextOutA is observational. Indexed artwork is compared before actual glyph stores. This is a controlled software-profile receipt, not a naturally played session, physical monitor capture, historical-font match or live stopwatch measurement. Other resolutions, localized text metrics, global UI scale and full network command/turn transitions remain unsupported. Broad RE stays paused.

Frozen evidence: `tmp/reference/turn-complete-evidence-20261019`.
