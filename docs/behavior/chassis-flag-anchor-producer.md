# Chassis flag-anchor producer

For the stock installed art, **Infantry=13 and Speeder=14** at byte9b7146+144*chassis. Both are<=50, so a stack marker uses the unshifted flag anchor and the primary flag shifts right by6 at zoom0 (9 at zoom8). This is an art-derived horizontal anchor, not a triad or TXT rule.

The complete original45f550 initializer was executed, including native PCX loading/pixel reads and its45f3a0 flag-sprite initialization tail. A guest-memory observer recorded the actual writes at45f9cc. The fields started at zero and ended:

| Chassis | Record x-anchor address | Value |
|---|---|---|
| Infantry0 | 9b7146 | 13 |
| Speeder1 | 9b71d6 | 14 |
| Hovertank2 | 9b7266 | 15 |
| Foil3 | 9b72f6 | 17 |
| Cruiser4 | 9b7386 | 12 |
| Needlejet5 | 9b7416 | 11 |
| Copter6 | 9b74a6 | 12 |
| Gravship7 | 9b7536 | 13 |
| Missile8 | 9b75c6 | 11 |

Source authority:45f738 pushes string6a8228=`UNITS.PCX` and45f742 loads it. The producer block45f96d..45fa3e scans x918..1017; for chassis i its effective marker rows are1+77*i and2+77*i. It compares native loaded pixel index against5, then writes x-918 as a byte. Native palette rebasing adds10 modulo256, so this marker is encoded as raw PCX index251. There is one marker hit per stock chassis: Infantry at(931,2), Speeder at(932,79). Independent owned-asset decoding reproduces every initialized value. UNITS.PCX SHA256 is1da9f262b4a90c6d26378e1421cc7bcc9dc561491337c1d8c111793a82b8285d.

The native traversal is column-major, testing both rows for each column; a later matching column overwrites an earlier one. If art has no match, this block leaves that byte unchanged. The stock all-nine run starts with zero bytes and records exactly one write per chassis. The native getter also performs an earlier unused read for each tested row; only the later read is compared to5. Do not introduce a default50 or infer this from land/sea/air.

570090 reads the same field to shift the initial flag anchor horizontally by trunc(field*(zoom+16)/16), before the stack-marker branch.5702f6 compares the **unscaled field** to50. For<=50, marker=initial anchor and primary=marker+gap. For>50, marker=initial anchor-gap and primary=initial anchor; gap=trunc(6*(zoom+16)/16). Positive global scale overrides use the corresponding numerator/denominator instead. Sprite-internal offsets are then added by the drawing helpers.

Six separate actual main-painter/renderer controls supply zero vertical anchor, negative design/weapon graphics overrides (so the normal chassis record is selected), and model/font/bitmap sinks. With input anchor(100,76):

| Input | Marker | Primary |
|---|---|---|
| Infantry13,z0 | (113,76) | (119,76) |
| Speeder14,z0 | (114,76) | (120,76) |
| Infantry13,z8 | (119,76) | (128,76) |
| Speeder14,z8 | (121,76) | (130,76) |
| Boundary50,z0 | (150,76) | (156,76) |
| Boundary51,z0 | (145,76) | (151,76) |

These absolute y values belong to the supplied zero-y-anchor fixture; no new stock y-anchor claim. If the bridge already applies the chassis x offset, add only the relative marker/primary gap, avoiding a second13/14 shift.

Renderer row selection has explicit art overrides: ordinary chassis comes from design+36, but a nonnegative design graphics byte+46 selects row9+that byte; the Infantry weapon graphics path can also select a special row. Earlier generic stack fixtures used zero-filled design records and therefore exercised a special graphics row with zero anchor. Their measured relative gap, raw silhouette and shared blink conclusions remain valid; their absolute anchor100 is not a stock Infantry anchor. The new six controls explicitly disable these graphics overrides and exercise rows0/1. Choose the rendered graphics row when applying anchors; do not substitute triad.

Pinned executable SHA2560a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01. `check_chassis_flag_anchor.py --verify-only` independently decodes owned art, verifies nine native initialized-byte writes, six renderer controls and23 instruction guards. The complete art initializer uses memory-backed Win32/FILE services; its field writes and pixel getter are not replaced. No executable/PCX bytes are bundled; no foreground or production interaction occurred.
