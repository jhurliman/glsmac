# Regional strategy storage at whole-new-world entry

RE-SMAC-NEW-WORLD-STRATEGY-ROWS-001, 2026-10-18. Stock SMAC.
[Poisoned-row control](new-world-regional-strategy-storage-cases.json).

**The checked whole-new-world entry does not clear regional strategy rows
before faction initialization.** All eight slots'128-byte rows preserve
their supplied prior contents. The reached turn0 faction-initializer prefix
also preserves them. Describe the earlier zero observation as **cold
PE-loaded executable storage**, not a measured cross-world reset.

The new control starts at the actual whole-new-world entry with current turn17
and distinct canaries in all1,024 strategy bytes. Native execution resets
the turn to0 and reaches faction1 initialization. All1,024 bytes remain exact,
with no watched strategy write. Resuming that same child entry with its actual
caller stack/registers through the checked initialization region again leaves
every row unchanged. The cold mapped rows were separately observed as zero
before poisoning; they were not treated as proof of a reset routine.

This supplements the existing [fresh and nonzero initialization evidence](regional-lifecycle-and-territory.md).
Fresh initialization clears other regional military/count/flag arrays; that
does not imply a strategy-row clear. The whole-world prefix's other bulk
clears—technology ownership, project markers and global histories—likewise
do not establish a clear of these strategy rows.

The [owner summary](regional-research-summary-producer.md) remains a later
producer. Existing complete calls rebuild strategy entries1–127 for the
requested owner; entry0 is preserved, and other owners' rows are not globally
refreshed by that call. Which rows have already been refreshed is part of the
caller ordering, not an instruction to reconstruct every row as zero on world
entry. Ordinary restore and nonzero initialization retain their established
separate lifetimes.

For same-process reuse, prior bytes can therefore survive **this world-reset
and fresh-child boundary until later producers run**. This controlled poison
stands for prior memory; it is not a replay of two completed games. No complete
frontend new-game-to-first-command value, last later writer, editor path or
indirect-alias completeness is claimed. In particular, do not relabel every
completed new world's final rows as retained or zero without its later summary
observations. The bounded result answers the pre-faction-init reset question.

Validation: one continuous whole-world-to-child-entry execution, then a resumed
fresh-child prefix;1,024 bytes compared and watched at both boundaries. No
frontend automation, broad matrix, production edits or standing-goal change.
