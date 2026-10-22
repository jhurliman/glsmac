# Production chooser Unit Help

The [original picker dispatch](behavior/design-workshop-entry-ui.md) maps
the Help button's raw callback 0 to final action 1. For a highlighted Unit
the Base caller opens Workshop with that exact design ID. This is separate
from the Workshop button's action 2, whose negative facility target enters
the owner's ordinary design-bank scan.

The live chooser now connects its Help button to that Unit route. It requires
an open editable Base, an active production chooser and an explicit Unit
highlight. It reuses the owner-bound queued Workshop opening, including its
stale-state, duplicate and asynchronous acknowledgement handling. Help does
not accept or change the Base production target.

Facility Datalinks and special chooser result 9999 remain unimplemented;
those targets never fall through to the Workshop/New path. The chooser's
complete original layout, labels and keyboard interaction remain separate
presentation work.

## Validation

Two focused scripts exercise the actual Help button callback and queued
opening with predefined design 2 and saved design 65, empty selection,
facility/special target rejection, read-only Base, closed Base and removed
chooser guards. Existing Workshop and transferred-Base tests pass.

A software OpenGL process loaded the saved Campaign Guard world through the
title menu, opened Morgan Industries and its production chooser, highlighted
Campaign Guard and clicked Help. The Workshop showed Campaign Guard, its
Synthmetal component and 1-2-1 tuple. Escape returned to the chooser; selecting
Scout Patrol and clicking Help showed its unarmored 1-1-1 tuple. No gameplay
commands or query values were injected. The process exited cleanly with no
tracked OpenGL leaks, and the original saved file remained byte-identical.
This is a live caller receipt, not full original-screen parity or hardware
graphics/audio validation. The final removed-chooser guard is covered by the
script; it does not change the captured normal path.
