# Current faction fight policy

`SmacFactionStrategy::fight` retains the original signed32 current AI aggression
field95d584, separate from the four Explore/Discover/Build/Conquer priorities.
Research checkbox answers and social ratings do not overwrite it.
The source identities and initializer assignments are documented in the
[Workshop input-authority map](design-workshop-input-authority.md) and
[startup faction reference](startup_faction_prefix_reference.py).

Fresh ordinary nonrandom worlds copy the installed aggression header once.
The original primary-rules bit40 override follows randomization and writes1.
Until the actual random-personality RNG producer is connected to live startup,
secondary-rules bit800000 leaves fight absent rather than pretending the header
is the sampled value. Bit40 still supplies1 after that unknown draw. This
representation does not implement random-personality startup or the AI policy
editor. Current policy remains a mutable runtime field for their later callers.

## GLSMAC save format

Simulation V35 appends each retained strategy owner's optional fight after the
V34 faction-turn tail, in the existing sorted strategy-owner order. All earlier
fields retain their framing. V11..34 restore fight absent; loading never reruns
an initializer or guesses from frozen definitions. A missing strategy map has
no fight tail. Signed32 extremes, per-owner absence and known zero remain
distinct. Existing identity/count validation owns the strategy map; truncation,
extra fields and out-of-range integers fail restoration.

The detached tests preserve full signed state through save/reload and research
priority changes, exercise the aggressive override after the random boundary,
and retain V34 absence. A live fixture verifies new-world header initialization,
saves a changed diagnostic value-17, and restores it through the actual world
decoder instead of re-reading the default faction. Its next worker calculation
and RNG state still match the uninterrupted continuation.

The focused validation is58 native groups, normally and with six production/two
test translation units instrumented by ASan/UBSan (leak detection disabled).
Correctness analysis covers eight production translation units/consumers. No
new platform matrix or renderer run is implied. The field is one required live
Workshop input; air-summary refresh and recommendation/preset commands remain
separate integration work.


## Nonzero-turn lifetime

[The restart lifetime audit](restart-strategy-priority-lifetime.md) pins the
original5c89ef jump past definition copying, randomization and the aggressive
rule override. Current Fight and all four priorities survive successful restart,
failed restart/elimination and repeated empty-owner disposition. Unknown legacy
Fight remains unknown. The port's existing preservation is retained.
Four bounded native prefix controls preserve all five fields and peer canaries;
they are not new full search/upkeep executions. Live port tests additionally
exercise changed Fight and priority values through the actual restart/elimination,
second empty-owner initialization, saved Start and continuation paths.


The broader CI run exposed a dependent V30 dominance-migration fixture that
stripped only the pre-V35 tail. Its explicit tail now includes every optional
Fight value, and the migrated frame is checked for absent Fight rather than
header replay. Production framing and reader admission are unchanged by that
fixture repair.
