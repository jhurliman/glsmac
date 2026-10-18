# Saved shared landmark detail

The original tile's shared high byte is independent of landmark membership and
registration. Its full initializer writes zero; allocation alone does not.
Save/read preserves all values, including 127, 128 and 255. Region fill,
terrain postpass, Base founding/removal and the measured off-view capture
preserve it, including overlapping low-word writes.

`GlsmacTerritoryTopology` now owns this optional byte array. Complete fresh
DisplayedTerrainV1 admission initializes zero. Region construction alone and
old saves leave it unavailable. Region rebuild, terrain postpass and site-cache
operations preserve the owned array. No registry or membership inference is
used to reconstruct an absent value.

Topology V4 appends these opaque bytes after optional site ratings. Presence
flags distinguish unavailable history from a known all-zero array. V1–V3
frames remain readable without acquiring invented detail. Bounded exact
framing rejects truncated, trailing and reserved-flag frames; high detail
values are data, not invalid site-cache flags.

Actual uncached site quality consumes the byte for Planet/Crater detail and
landmark suppression. Legacy explicit active Jungle/Uranium flags retain
their existing interpretation; queried Planet/Crater tiles without owned
detail still fail preparation. The original Q scan can now receive owned raw
bytes. Its live restart caller and original landmark/world generation remain
unfinished; this change does not replace the farthest-site approximation.

The frozen native save/read fixture retains both complete 64-byte payloads,
including a registry-free case. Live regressions check the fresh initializer,
suppression in the actual site-quality caller, and all 4,096 raw bytes through
founding, removal, save and actual Start. These test port history ownership;
they do not claim compatibility with the original binary save format.
