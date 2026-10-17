# Campaign save/load UI

GAME → Save Campaign opens a typed-path popup. It defaults to
`campaign.glsmac`, explains relative paths and same-name replacement, and calls
the native `game.save_campaign(path)`. An error leaves the path and dialog
available for correction. Success displays the returned absolute path and a
saved confirmation; Close exits without saving again.

Main-menu Load Game opens the existing menu popup with a typed `.glsmac` path.
It calls `glsmac.load_campaign(path)`, retains synchronous errors in the same
dialog, and dismisses only once the native loader accepts the file for normal
asynchronous initialization. Cancel calls no native file operation. The labels
explicitly distinguish GLSMAC campaigns from unsupported original `.SAV` files.

`campaign_file_ui.gls.js` passes against the actual menu and popup scripts with
DOM and native file APIs doubled. It covers edited paths, errors, retry,
successful save output, accepted load, reopening, and cancel/close behavior.
Native file atomicity, envelope validation, and claimed world restoration are
owned by the separate backend tests. No build or rendered UI claim is made by
this script-only checkpoint.

Interactive checkpoint: save a stable campaign, verify the full path, try an
invalid path without losing the session, return to the main menu and load the
saved campaign, then verify the restored turn/Base/Unit state. Capture the save
confirmation and load error/correction screens alongside the restored world.
