# First-session checkpoint

Next milestone after the current Former/controller/render integration. Gameplay
breadth waits until this path is usable and visually reviewed. This is not a
claim that SMAC or Alien Crossfire is complete.

## Acceptance

Exercise normal new-game setup, starting placement, movement/animation,
selection/stats, the Base screen, production/research, closing and reopening
screens, a new world in the same process, application close/relaunch, and actual
campaign disk save/reload. Retain screenshots or a short recording and compare
the relevant view with the original. Record executable revision, ruleset,
settings and asset identity; a SMAX observation is not SMAC rule evidence.

Use one cached Linux build and affected tests during fixes. Run the platform
matrix and native interactive checks on the combined renderer/session
checkpoint. Existing tests of world snapshot codecs do not establish working
disk saves or menu navigation.

## Concrete gaps from source inspection

| Path | Gap | Required outcome |
| --- | --- | --- |
| Setup | All six difficulties are selectable; canonical starting Base context currently requires Librarian. Faction selection is a TODO. | Every offered selection starts correctly; the selected faction is explicit. |
| Units | Former/Colony use temporary markers; conventional scale/pivot is port policy. | Verify placement, image, scale, movement and selection against reference captures; list remaining visual differences. |
| Unit controls | Former F/R/O/S/M, cancel C and Colony B lack a visible command/help surface. Saved work/order progress is not displayed. | Actions and current orders are discoverable without reading source. |
| Selection | Retention, wrap distance and record-order ties are being integrated from original controller evidence. | Work continues and selection changes correctly across movement, work and turn boundaries. |
| Base | Energy distribution, eco-damage and treasury presentation have unavailable fields. Production offers a limited unit set. | Verify worker edits, quantities and picker confirm/cancel; close/reopen without changing simulation values. |
| Research | Required-choice script passes with UI doubles; native rendered modal is unverified. | Selection, scrolling, confirmation, repeated owner requests and reset behave correctly. |
| Save/load | Load Game is a stub; SaveMap writes only terrain. | A local campaign envelope restores frozen setup, roster/local owner and the complete world without replaying first-turn creation. |
| Lifecycle | Latest retained interactive receipts predate this renderer. | New game, close and relaunch pass with the checkpoint build. |

Other live limits remain explicit: ordinary movement currently restricts damaged
or promoted units when Bases exist; conventional attacks are not integrated;
size-one Colony production has an unresolved choice boundary. These are not
completed features simply because a control is visible.

## Reference and platform routes

Original Windows reference: the installed SMAX PRACX shortcut, with its original
installation as working directory. The October 15 retained attempt crashed
after accepting the first Base name. On October 17, a new shortcut launch
produced a nonresponding process and window inspection timed out through the
documented recovery attempt. No game input or new gameplay observation was
obtained; the process launched for that attempt was stopped. Neither failure
disproves the user's successful setup. Resolve the launch discrepancy before
claiming a current visual comparison.

Windows assets: `C:/Program Files (x86)/GOG Galaxy/Games/Alpha Centauri`.
Honey assets: `/Users/jhurliman/Documents/Games/Alpha Centauri`.
Use matching `GLSMAC_data`, an isolated prefix, windowed mode and captured logs.
Start with the normal menus, then controlled Gaia and Sparta quickstart cases
on a 32 by 32 map. A fixed map seed does not pin every gameplay RNG stream.
The cached Windows and Mac executables at inspection were `dacac744`, so they
cannot validate the current renderer. Honey's earlier user-confirmed map,
Base, turn, new-game and close checks remain evidence for their old revision.

## Visual-defect record

Populate this from actual checkpoint screenshots, not source assumptions. Earlier
observations to recheck include jagged map edges, clipped Base badges and a Mac
texture warning. Do not label them fixed until the current session demonstrates
it. Save each issue with the screen, reproduction steps, expected/reference
behavior and the fixing revision.
