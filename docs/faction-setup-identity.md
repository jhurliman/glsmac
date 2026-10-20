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

## Remaining fidelity work

This is functional setup wiring, not dialog pixel parity. Portrait/quotation
and detail previews, descriptors, OTHER/file selection, original positioning,
fonts and complete button artwork remain open. Gender is retained and shown;
editing remains deferred because the original static binding is unverified.
The form currently offers an explicit Customize button instead of claiming the
source Change Gender secondary action is reproduced.

The port stages drafts until startup. The original copies an accepted leader
name before extended customization; its cancellation/default-reload mutation
boundary still needs matching. Research UI also still uses the controller's
name in some headings. These are explicit follow-ups, not completed fidelity.
First-base naming and the main-map exploration mask remain separate unfinished
features in the [playthrough audit](smac-playthrough-ui-audit.md).