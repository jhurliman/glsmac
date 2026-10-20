# Single-player faction identity setup

Faction selection now opens leader-name entry for configured definitions with
setup metadata. An optional Customize form edits Name, Formal, Noun, Adjective,
Title and Description, with Restore Defaults. Enter submits either form;
Cancel/Back retains the selected faction. DOM text is materialized before a
popup is replaced, rather than retaining references into removed controls.

The original field mapping and byte limits come from the independently
verified [static setup contract](behavior/setup-dialog-continuation.md) and
[TXT bindings](behavior/setup-identity-txt-bindings.md): formal39 bytes,
other editable strings23. Accepted fields replace the selected faction's saved
identity before world startup. They do not change its stable rules ID, bonuses,
or the controller's separate roster/account name. The lower console reads the
same retained identity. Input currently supports nonempty printable ASCII.

Faction snapshot v3 saves the additional noun/adjective/description/default
gender fields. Versions1 and2 remain readable and preserve absent metadata;
restoration does not fabricate current installed defaults. Invalid v3 metadata
is rejected before replacing a destination definition.

## Validation and actual captures

The cached build,11 focused native groups, four affected production analyses
(zero findings), and focused six-TU ASan/UBSan checks pass. Tests cover source
mapping including lowercase female markers, accepted startup binding, native
limits, old versions, malformed v3 atomicity, form submission/Enter, default
restoration, rejected input and faction-selection history. Existing title and
ruleset navigation checks remain applicable to unchanged source.

A real1024x768 run selected Morgan Industries, changed its leader to Morganb,
accepted customization with Enter, and saved at the initial research question.
After returning through Start New Game and loading that campaign, the console
still showed CEO Morganb and resumed the research question without replaying
setup. Campaign SHA256:
`43025b4c052ccf4b4bec271799d46db94d568eb3188e537fea924852b9d79c61`.
A subsequent real run verified Back retains Morgan selection and ordinary
name-only acceptance starts the campaign. Final captures below are framebuffer
pixels; the reload image is from the separate save/load run. Both successful
runs used normal Quit, exit0, with no tracked GL leaks.

![Actual name entry](https://raw.githubusercontent.com/jhurliman/glsmac/32f61ba9df7c3f0130375c9dec057681bd71134d/faction-identity-setup/name.png)

![Actual optional customization](https://raw.githubusercontent.com/jhurliman/glsmac/32f61ba9df7c3f0130375c9dec057681bd71134d/faction-identity-setup/customize.png)

![Actual campaign after reload](https://raw.githubusercontent.com/jhurliman/glsmac/32f61ba9df7c3f0130375c9dec057681bd71134d/faction-identity-setup/reloaded.png)

The cancellation/default follow-up passed a cached incremental build, three
targeted native setup groups, the identity script with the real popup manager
and Escape routing, existing faction-selection/ruleset navigation scripts,
and changed-production analysis of GLSMAC.cpp (zero findings). A real SDL run
accepted Morganb, escaped customization, and reopened Name with Morganb;
Restore Defaults followed by Cancel and reentry showed Morgan. It exited
normally through the title menu (exit0, no tracked GL leaks).

![Name retained after cancelling customization](https://raw.githubusercontent.com/jhurliman/glsmac/5b3abb01ac26bd7502189c98ec6d8eeea2561e3f/setup-name-boundaries/retained-name.png)

![Defaults retained after cancelling customization](https://raw.githubusercontent.com/jhurliman/glsmac/5b3abb01ac26bd7502189c98ec6d8eeea2561e3f/setup-name-boundaries/default-name.png)

## Remaining fidelity work

This is functional setup wiring, not dialog pixel parity. Live portrait,
quotation/detail previews and descriptors now have a separate
[implementation and evidence](faction-selection-preview.md). OTHER/file
selection, original positioning, fonts and complete button artwork remain
open. Gender is retained and shown;
editing remains deferred because the original static binding is unverified.
The form currently offers an explicit Customize button instead of claiming the
source Change Gender secondary action is reproduced.

Name acceptance now commits to the live selected definition before optional
customization. Cancel or Escape from Customize returns directly to faction
selection, keeping the accepted name and discarding unaccepted form edits.
Restore Defaults reloads the catalog's TXT identity immediately, so cancelling
afterwards keeps the restored defaults. This covers the supported catalog flow;
OTHER definitions and source graphics reload remain unfinished. Name-only
acceptance checks its23-byte edit limit without rejecting untouched TXT fields
that can legitimately exceed their own form edit limits. Research UI still
uses the separate controller name in some headings.
The actual input controls now use `max_bytes: 23`, with39 for Formal.
Defaults, later assignments (including Restore Defaults), keyboard insertion
and clipboard assignment all pass through the same bounded setter. Lowering
the bound trims the existing text; bounded strings stop at their first NUL.
Unrelated controls retain the default zero/unbounded setting. The
[generic modal trace](behavior/setup-modal-result-boundaries.md) establishes
default truncation before user edits, rather than rejecting long parsed TXT
defaults only when the form is submitted. Its private nine-file receipt was
hash-verified with manifest SHA256
`e4c7ac9c5d3f330e499b04bfdccfc0fccb67e3a4c06c03ea1ad8a729170e4d70`.
Native real-widget tests cover initialization, repeated overlong assignment,
typing at the limit, Backspace/insertion, first-NUL behavior and bound changes;
the setup script asserts all six per-field limits. Input.cpp and its Container
consumer pass targeted C++ analysis with zero findings. No new visual parity
claim follows from this interaction change. Empty-input acceptance and full
code-page/selection behavior still need matching.
First-base naming and the main-map exploration mask remain separate unfinished
features in the [playthrough audit](smac-playthrough-ui-audit.md).
