# Installed predefined designs and localized opening names

`LoadSmacPredefinedDesigns` implements stock `59dfc0` over the complete512-record
pool. It consumes fourteen parsed source rows and numeric component tables.
Each row copies its name/prerequisite, builds Fission components, then applies
plan, nonzero cost/capacity and icon overrides in original order. It preserves
name tails, unused records, byte47 and unrelated fields, including poisoned input.
The second automatic role byte remains independent of an overridden plan.

The shared `BuildSmacFissionDesign` now serves both predefined loading and fresh
opening construction. Cost is calculated from weapon/armor/chassis cost and
ability penalties, followed by domain adjustments and original rounding. Capacity
and automatic role transitions are calculated before row/allocation overrides.
The earlier explicit early-component profile remains available for its existing
controls; the installed caller supplies actual decoded component rules.

`NameSmacOpeningDesign` implements the fresh no-collision Hand Weapons/Fission
Synthmetal defender and Speeder Recon label paths. It reads defense/chassis text
and the Scout label from assets. The infantry defense variant uses owner parity
XOR armor-strength parity, retaining the original Garrison/Sentinels distinction.
`GenerateSmacInstalledOpeningDesigns` reads the Mind Worm gate from the initialized
predefined record, then composes capability refresh, real localized naming and
fresh construction. The existing separate caller marks prototypes/selects Scout.
General advanced naming, collision suffixes, non-Fission component construction
and later workshop/replacement paths remain separate required work.

## Independent source and native controls

Frozen bundle `independent-startup-installed-designs-evidence-20261020` contains273
verified files; manifest SHA256 is
`1193c2fcb18655d2c010233e63e05319c2bb5069b48eeb8d5abd4361b2fc200b`.
Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Numeric rule data comes from independently parsed owned alpha.txt; label text
comes from alpha.txt and labels.txt. Native helper outputs are expectations only.

- Two full512-design loader controls start from zero and poisoned storage and
  compare468 ordered numeric stores.
- Eighteen original localized constructor controls compare complete pools and357
  numeric stores. Native-derived source-label identities also check public
  symbolic parity selection.
- Fifty-six full installed early-controller controls cover all seven owners,
  four difficulties, stock named grants and combined early grants, comparing
  complete faction/pool buffers,3,844 numeric and64 caller-marking stores.

Asset-free tests substitute same-length predefined labels, normalize only the
written text payload for numeric pool comparisons, and preserve checks on all
unwritten name tails and other bytes. Fresh custom-name fields are normalized
separately. They do not claim original full text hashes with substituted labels.
The owned-text verifier checks complete unnormalized pools, faction state,
original names and journals under independently verified source file hashes:

```sh
python3 tools/reference/verify_startup_installed_designs.py \
  --asset-directory /path/to/purchased/game --sanitize
```

Temporary owned-text headers and binaries are removed together. All76 controls
pass under standalone ASan/UBSan with actual source text. Purchased tables and
full capture buffers remain outside Git; public profiles contain hashes, numeric
inputs and comparison hashes.

## Generated composition and validation

The generated zero-storage Gaia test initializes all fourteen predefined records
and uses the installed early wrapper. Base production costs come from initialized
records0..2 rather than literal costs. The complete native pool is identical at
startup entry, after Gaia's early-design caller and at4faab7; its hash is
`5aedb2706612e697fe0e0bf07b1deff87ec80b2d2fefb43df5ba76b1f1c42bf6`.
The asset-free composition checks those boundaries outside declared text payloads;
the owned verifier also checks the exact full catalog hash. Complete first Base,
Gaia faction, map/regions and RNG checks continue to pass. This supersedes the
supplied-label/basic-three startup admission for the installed early profile.

Cached incremental build and five selected native groups pass, including previous
fresh-constructor and early-controller regressions. Both affected production TUs
pass correctness analysis. A focused five-TU ASan/UBSan engine build passes the
same groups; other objects are normal and leak detection is disabled. The separate
owned-data sanitizer probe checks full text pools. No duplicate platform matrix or
new runtime screenshot is claimed. Relocated Base contexts and all-seven startup
composition remain the next integration gates.
