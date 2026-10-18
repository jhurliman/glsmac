# RE-SMAC-RESTART-HOSTILE-HISTORY-001: nonzero restart delta

One turn6 case executes the original nonzero faction initializer and accepted
restart package continuation through normal return. The supplied history values
are deliberate poison controls. Placement search is omitted, as in the accepted
[package-selector boundary](restart-unit-package-selector.md).
[Literal case](restart-hostile-history-cases.json).

| Field from the state-ownership receipt | Nonzero restart behavior |
| --- | --- |
| Primary relations | Normalize every outgoing direction **and the reverse direction involving this faction** through the original revoke helper |
| Secondary relations | Preserve |
| Directed transition dates | Preserve; do not replace with current turn or−1 |
| Paired counters A/B | Preserve |
| All512 loss-history entries | Preserve |
| Integrity blemishes / bookkeeping | Preserve |

The primary normalization clears logical mask `0x9000ff48`, equivalently keeps
`0x6fff00b7`. For each counterpart slot0..7, apply it both ways. Unrelated
foreign-to-foreign entries preserve. For example, an affected primary word
`0xffffffff` becomes `0x6fff00b7`; the earlier hostile-caller result
`0x80008118` would become `0x10` under this same mask. Existing Vendetta and
Pact/Treaty/Truce bits themselves survive. Do not replace the relationship with
zero, automatically end Vendetta, or inherit the whole primary word unchanged.
The helper's secondary-clearing branch is not selected by this mask.

The actual nonzero branch skips the fresh initialization block for integrity,
bookkeeping and most history, but it still runs the relation-revoke loop.
Its per-design loop skips the fresh-only loss-history clearing. All checked
history fields preserve through the subsequent original package factories and
normal return—not just at an intermediate prefix.

The case returns the existing Pod/Former/Pod/Scout package at the supplied
accepted site. This verifies continuation only; it adds no package/placement,
palette, AI/network, reused-newworld or save-lifecycle claim. No gameplay C++
or original files were edited. The standing research goal remains paused.
