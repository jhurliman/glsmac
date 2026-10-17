# Startup difficulty and governor defaults

The normal local player now stores the difficulty chosen in setup. All six
semantic names are resolved through the configured rules map before a roster is
created. The current local roster has human controllers only; it does not treat
other Player slots as original AI factions. The original AI stored-level-3 rule
therefore has no new live producer in this change.

Canonical founding uses the captured global-difficulty defaults: Citizen
produces governor options 0xc0dfbe41; the other five produce 0x00dfbe41. Global
difficulty is read independently of the owner's stored difficulty. The existing
saved Base context already owns all 32 bits, including activation and the
0x40000000 startup automation marker; restore does not derive them again.
There is currently no governor-preference editor in GLSMAC, so subsequent Bases
copy the unchanged initial default from saved startup difficulty. A future
editor must own the faction's remembered configuration before changing that
producer; a manual production override changes only its Base.

Turn-zero production still prefers design 1 when its ordinary prerequisite is
owned. Otherwise it selects Colony Pod 0 at stored owner levels 0/1 and Scout 2
at levels 2..5. The existing cost, half-credit, baseline and queue producers are
unchanged. Founding and recalculation pass actual saved governor activation,
citizen management, directives and frozen faction priorities into the existing
allocator. Its ordinary and managed-upkeep paths remain distinct.

An authenticated changed-item prelude clears governor activation and the startup
marker only when that marker was present. Same-item selection preserves them;
activation without the marker is preserved. Read-only picker queries do not
change state. The first OK for a quoted loss queues the prelude before displaying
the confirmation. Declining at stored difficulty 2/3 preserves the old item,
minerals, baseline and queue but leaves marked activation cleared, as measured by
`governor-marker-loss-cancel`. These marked higher-level cases are controlled
retained-marker states, not original startup defaults. Normal stored levels 0/1
already skip the loss prompt and restore the full baseline on selection.

A governor-only prelude does not queue production/display refresh, which would
close the pending loss dialog. The next worker query consumes the updated context;
accepted production changes retain the existing staged refresh transaction. The
saved context codec preserves the result on restore. This is the bounded manual
interaction, not a complete governor production selector or preference editor.

Evidence: startup-governor-difficulty.md and its 8 prefix, 6 founding and 8
manual-prelude controls; founding-production-initialization.md for the stored
owner target branch; existing worker-allocation-priorities and whole allocator
receipts for the live governor input composition. These are bounded original
checks, not six complete original campaigns.

Coastal Unity Foil grants remain a separate known gap. The accepted receipt uses
raw starting-region area and recorded faction presence, not tile counts guessed
from the current map. The port still needs those producers and an actual naval
Unit/presentation package before implementing the global-0/1 exclusion and grant.
This change neither fabricates that authority nor silently changes start sites
to avoid it. Beginner preference-reset dialogs and AI controllers remain separate.

Validation: syntax checks passed for all seven changed production translation
units and four affected test translation units. Added actual six-difficulty
claimed Start cases, a global-versus-stored context discriminator, eight real
production-event configurations, and managed-upkeep allocation coverage. Linked
native execution is reserved for the parent's next shared cached build; no full
suite or platform run is claimed here. The later pre-confirmation follow-up adds
one six-case actual picker/event/restore group; its shared linked run is pending.
