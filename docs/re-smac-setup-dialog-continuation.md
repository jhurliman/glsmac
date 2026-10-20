# RE-SMAC-SETUP-CONTINUATION-001 audit

Date: 2026-10-19. Public contract:
[setup dialog continuation](behavior/setup-dialog-continuation.md).
Pinned PE SHA-256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

This receipt is static. It does not restart an emulator or execute a new
original control. `tmp/reference/extract_setup_dialog_contract.py` reads
the installed PE and text/PCX files and exports indexed function ranges from
the retained Binary Ninja index. The expanded receipt checks 33 literal instructions at the
documented portrait, input, codec, first-base and normalization sites.
The source JSON retains those functions including faction parser59d1d0,
hashes their original byte ranges,
decodes relevant PE strings, and records all seven faction TXT/PCX hashes,
identity header lines and BLURB/DATALINKS1/DATALINKS2 blocks. The PCX check
reads headers only; it neither decodes nor modifies images.

The TXT-binding addendum pins zero-based row/token mappings for the six
customized strings, F/f first-byte leader gender, parser versus UI length
limits, and Restore Defaults replacing the earlier accepted leader name.
The seven-file initial frozen bundle remains immutable; the expanded
eight-file v2 bundle includes the new addendum and parser source.

Source interpretation: setup row EBP starts95ae28 for slot1, so row strings
at EBP-40 and EBP resolve definition+384/+3c4; PE separators are vertical
bar, opening parenthesis, closing parenthesis. Portrait extraction has
seven stack arguments `(image,9,304,457,131,157,0)` in callee order,
matching helper RET1c. The rectangle/key contract does not assign a name
to that final zero parameter.

The dialog action scratch esp4840 is the reused stack dialog object at
esp1740 plus3100. Therefore customization's nonzero field is its own
secondary action, not a stale copy of the prior faction answer. 641090
copies the corresponding field to9c4550 for the leader-name wrapper.
The seventh edit registration clones the supplied scratch default in
62c280; the controller's eventual gender comparison still addresses that
original scratch rather than9c3ff4. This unexpected static result is
explicitly limited pending native input evidence.

FIRSTBASE occurs at5c620d inside5c5b30 after a live owned base has been
selected. Name storage is Base+13 (hex), not the leader identity record.
5c622c's EAX is not tested before copying global input and calling50cc80.
The existing first-command receipt supplied5c5b30, so none of this is
upgraded to measured modal behavior. Codec5be250's definition and Base
array transfer branches establish save inclusion statically, not a new
custom-name round trip. 5c8650's duplicate leader/noun logic can further
change identities; no universal preserve-exact-entered-string claim.

Validation runs the extractor once and again with --verify-only, comparing
the complete reconstructed receipt. The frozen manifest hashes all handoff
files, including this audit and the source index needed to reconstruct the
receipt. No production edits, staging, commits, installation changes or
rendered-layout claims.
