# Canonical production targets and pending queue

The actual Base production record now owns an optional positional source ID for
each target and an optional ordered pending queue. An absent queue is legacy or
unconfigured; a present empty queue is an explicitly empty human queue. Current
and baseline targets/minerals remain independent. Duplicate queued targets and
opaque port IDs restore exactly. Neither names nor opaque IDs are parsed to
recover source identity.

`Game::MakeSmacProductionItem` validates the requested kind and source against the
frozen predefined catalog, the owner's saved custom designs, or the frozen
71-row facility/project catalog. Base construction and replacement validate all
canonical targets, including the queue and baseline, before publication. The
cost query uses the frozen cached item cost, mineral multiplier, accepted world
category and current INDUSTRY through the existing mineral-factor calculator.
It admits local human costs; custom prototype surcharges remain unavailable.
Early cost queries require the actual claimed new-world Start. The separate
prerequisite query is only ordinary enabled ownership; it intentionally does
not apply active, exclusion, Colony or coastal eligibility. Founding can thus
test design 1's prerequisite without changing that original caller's meaning.

BaseManager framing advances to V10; World26 framing is unchanged. Canonical
opaque IDs have a 256-byte port budget and the pending queue a nine-entry port
budget, matching the largest retained queue test, not an asserted original
queue-editor maximum. Counts and canonical text lengths are checked before
allocation. Existing unresolved legacy IDs retain their prior domain.

The live worker caller now accepts an explicit Ordinary/Upkeep phase and forwards
it to both resource queries and allocation. This checkpoint does not post
minerals, create units, advance a queue, collect food or advance a turn. Those
operations will consume this record in the upcoming production transaction.

Validation: cached incremental build and 19 affected tracked native groups pass,
including three new actual target/Base-wrapper/restore/codec groups, six existing
production-state groups, six switching groups and four live allocation groups.
The latter retain valid Doctors in ordinary and unmanaged-upkeep calls. Targeted
analysis of four changed production translation units reports zero failures.
The tests use synthetic frozen sources and existing calculators; they do not
claim a new original-executable run or complete production eligibility.

The local human roster policy admits accepted Single/Player participants only
when the actual local controller is Single and no Connection exists. This is
a GLSMAC human-controlled roster policy; it does not implement AI costs or
upkeep. Manual worker commands still require the actual local Base owner.
The follow-up cached run passed twelve affected tracked groups, including an
actual second participant cost/upkeep repair and rejected manual edit.
