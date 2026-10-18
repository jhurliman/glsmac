# Ecology words before the first fresh faction initializer

2026-10-18, stock SMAC. **The whole-new-world parent preserves all eight
clean-mineral history words, all eight major-atrocity words and all eight
retained research/ecology aggregate words through first faction-initializer
entry. It does not bulk reset these24 words in that prefix.**

One actual parent prefix starts with previous turn17, human slot1, and distinct
poison values for every field/slot. It reaches the first faction initializer
for owner1 at turn0, stopping before that child's first instruction. Every
word below survives unchanged, with zero writes to the24 watched words.
[Literal cases](new-world-ecology-word-boundary-cases.json).

| Field | Slots0..7, before parent and before first child |
| --- | --- |
| Clean-mineral history | 101,108,115,122,129,136,143,150 |
| Major atrocities | 202,209,216,223,230,237,244,251 |
| Retained research/ecology aggregate | 303,310,317,324,331,338,345,352 |

These poisoned values model prior memory, not an organically played previous
world. The result distinguishes parent retention from the separate per-owner
writes in [the existing initializer/pass receipt](ecology-input-lifetimes.md):
fresh owner initialization clears clean history and major atrocities in the
tested child prefixes; nonzero-turn initialization preserves them. Those
child prefixes preserve the research/ecology aggregate, which the owner Base
pass clears before accumulating contributions. The parent boundary must not
be implemented as a substitute for those later writers.

This new control does **not** resume the child, return from the whole parent,
or replay two complete frontend sessions. It therefore establishes neither
that these words survive a complete new game nor that all slots have already
been initialized at first-child entry. Cold executable zeros are not a
cross-world reset authority. Standing research remains paused.
