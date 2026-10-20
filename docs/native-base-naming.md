# Original automatic Base naming

`GenerateSmacBaseName` implements original `4f9950` using parsed ordinary
single-byte text, an owned faction record and the map's resource pattern.
Land and sea cursors remain separate. A successful sea name advances only the
sea cursor; sea exhaustion falls through to land. Preview computes the same
text without cursor stores. No registered-name uniqueness policy or gameplay
RNG is introduced.

The first land name stays fixed. Later faction-list names use the original
8-bit LFSR initialized from owner plus resource pattern, skipping states outside
the list. The cursor stores the sequential ordinal, independently of the
permuted source index. Generic names follow without permutation; exhausted
lists retain noun-plus-decimal fallback and do not advance either cursor.
Successful labels copy exactly24 bytes with zero padding and forced byte23
termination. The earlier fallback and all untouched output bytes survive as
in the original. Negative/overflowing cursors, missing source sections and
modded substitution syntax remain outside this ordinary installed-text API.

## Source and complete native verification

Frozen bundle `independent-startup-base-naming-evidence-20261020` has254 verified
files and manifest SHA256
`c657f65ef23c9161fd8e291e3e9df9f9eab1aa4f35fee311e4b92f06b4ff3a83`.
Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The813 controls cover all seven faction lists, three pattern controls across
every land ordinal, previews, commits, sea/land/generic boundaries and exhaustion.
They retain native hashes of all128 output bytes, all67,040 faction bytes and
694 exact cursor stores with PCs. All selected native labels uniquely identify
an entry in the independently parsed source tables.

Public asset-free tests use symbolic list labels with the original counts and
native-derived expected entry identities; they compare selection, complete
faction buffers, cursor journals and symbolic buffer behavior. They do not claim
that substituted text matches original output hashes. Separate literal controls
cover truncation and unpadded fallback. Purchased lists are not committed.

The owned-text verifier checks all eight original source hashes, parses the
actual faction/generic sections, builds a temporary standalone C++ probe and
compares every native output/faction/journal hash:

```sh
python3 tools/reference/verify_startup_base_naming.py \
  --asset-directory /path/to/purchased/game --sanitize
```

The temporary text header and binary are removed together. This runs without
the game executable, emulator, graphics or full engine build. All813 complete
original controls and694 stores pass under ASan/UBSan with the owned text.
The public source profile contains hashes and file identities only.

## Generated composition and remaining work

The zero-storage Gaia composition now calculates the first name and cursor
before Base construction. The first installed label is a declared text input;
unreached entries need no asset copies in the public fixture. Full owned lists
are covered by the verifier above. All304 Base bytes, the complete Gaia faction
record (including cursor), map/regions and RNG match at4faab7. The earlier
cursor-normalized faction comparison is superseded; no producer cursor patch
is used. Actual startup controller/catalog loading and full seven-faction
publication still need connecting to gameplay.

Cached incremental build and three selected native groups pass. The new
production TU passes correctness analysis. A focused three-TU ASan/UBSan engine
build passes naming and generated composition, with other objects normal and
leak detection disabled. The separate owned-text standalone sanitizer check
covers complete native text buffers. No platform matrix or new rendered image
is claimed for this raw-state calculation.
