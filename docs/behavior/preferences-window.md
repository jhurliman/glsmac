# Live SMAC Preferences window

The SMAC game menu and Ctrl+P/O/W/A/I/M open the original six main
preference pages. The warning page's More/Back control exposes its second
page. All 67 checkbox rows use the installed text catalog and a detached
draft; Cancel/Escape discards it, while OK/Enter saves once. A failed save
keeps the draft open. Restore Defaults uses the original reset variant.
Pending gameplay dialogs and the Workshop exclude this modal.

The frame, console rails, checkbox artwork, caption table slots and shared
button skins follow the original-source contracts in
[initialized theme](preferences-initialized-theme.md),
[label bindings](preferences-label-bindings.md) and
[preference semantics](preferences-semantics.md).

## Runtime verification

On 2026-10-21 a software OpenGL session loaded a real saved campaign,
opened Preferences through Ctrl+P, toggled ID8 by clicking the row away
from its checkbox, navigated Warnings and More, canceled and reopened,
then toggled again and accepted through a separate mouse press/release on
OK. Reopening verified persistence. The process exited cleanly without
tracked OpenGL leaks. The four checked/unchecked ID8 row captures each
match all 10,968 pixels of the original target row.

For comparison only, this run seeded packed words `(16384,0,0)`, matching
the explicit original offscreen target; this is not a fresh default profile.
At 1024x768, the inner pane `(192,157)-(878,533)` differs at nine of 257,936
pixels. Five are existing Arial Narrow 16 synthetic-bold glyph differences;
four are two Arial 12 caption glyph positions (Advanced and Audio/Visual).
Those font differences remain unresolved. Transparent frame corners and
rail keys expose different real-map backing and are outside that comparison.

![Live initialized Preferences window](https://raw.githubusercontent.com/jhurliman/glsmac/af6f771d44b61bf4371fa9ba6d732212fa2d1ede/docs/pr-media/4eea2208054fb947c977d59625eb38628be375ff/preferences-initialized-live.png)

## Remaining behavior

The existing Workshop suggestion and edge-scroll consumers read these
preferences. Other stored options still need their live consumers. The
Audio/Visual checkboxes persist, but playback controls, audio levels and
Gamma correction remain unimplemented. Gamma reports that limitation.
SMAX Preferences remains unavailable. The 800x600 placement branch exists,
but its full pixel parity has not been established. This is a working
configuration window, not completion of every option's gameplay effect.
