# Restart unit package selection

`SmacRestartUnitPackage::Select` consumes the retained predefined and custom
catalogs, the owner's saved technologies and an admitted turn in 1..100. It
returns selected design IDs and factory creation order without changing those
inputs. It does not generate designs from technologies or use faction names.

Predefined eligibility reuses the frozen production prerequisite definition
check; custom records use active/excluded state and their saved owner block.
The selector has no Base context or coastal restriction. Ground designs with
nonzero raw weapon power compete in ascending positional order. Strict score
replacement preserves the earlier record on ties. The restart initializer's
cleared strength caches make negative components evaluate to zero here only.
Pod and Former grants bypass the competing-design filter. Reinforcements use
the strict turn20/40/60/80 boundaries and preserve factory order.

The supported component inputs fit the original signed weapon/armor bytes and
unsigned speed byte; unknown prerequisite definitions, malformed identities,
out-of-range components and turns outside 1..100 reject. The statically proved
Scout fallback remains, although an empty eligible defender set was not an
original dynamic fixture. Reactor, abilities, prototype and plan do not score.

Evidence is `RE-SMAC-RESTART-PACKAGE-001` in the human-owned
`docs/behavior/restart-unit-package-selector.md` and cases. The committed compact
fixture projects 23 literal selected-ID/factory-order records, pinned to public
fixture SHA256 `df18f5493b6bb0dc52a70f160fedbf5e34242408a5ca3622ea17a016dca43f65`.
Custom component identities are taken from the retained probe's supplied
records; numeric attack/armor/speed inputs are reproduced in owned synthetic
catalogs. No installed asset bytes or original checker implementation are
included. Four native groups cover all seven startup catalogs, strict turn
boundaries, retained-technology changes, distinct roles, ties, negative mobile
scores, inactive/excluded/foreign/sea/noncombat records and mixed Psi components.
Additional port checks cover malformed domains and predefined eligibility.

This is a selection dependency, not a completed restart. A caller must still
establish local-human admission, Do or Die/history rules, accepted placement,
allocation success and disposition. No Units are created, no RNG is consumed,
and no live caller, persistence schema or default UI is changed. The original
history-veto case and placement search are deliberately outside this API.

The cached Linux Debug build passes all four native groups with memory tracking.
The focused clang-tidy 15 pass checks the production translation unit in 1.43
seconds with zero failures. No additional platform build was needed for this
bounded selection rule.
