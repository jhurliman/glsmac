# Supreme Leader requirement: common PROPOSALS reader

RE-SMAC-PROPOSALS-REQUIREMENT-READER-001, 2026-10-18. Stock SMAC.
[Literal installed rows and reader controls](proposals-requirement-reader-cases.json).

The requirement comes from **`#PROPOSALS`, row2, column2** (one-based):
“Unite Behind Me As Supreme Leader”, key **MindMac**. Internally this is proposal
index1. The common reader stores three fields per proposal: name, description,
resolved requirement. It reads the source columns in name/key/description order.
The stock requirement resolves to technology ID41, Mind/Machine Interface.
IDs39/52 in the earlier pressure receipt are that technology's immediate
prerequisites, not adjacent proposal-row requirements.

Bind the live consumer to this **owned common-catalog requirement**. ID41 is
the measured installed result, not a replacement for loading the key. There
is no numeric technology-ID parse in this reader.

The actual comma-token reader trims the token, then the native reference
resolver performs case-insensitive whole-string comparisons:

| Input token | Resolved result |
| --- | --- |
| `None`, including tested lowercase/uppercase | −1 |
| `Disable`, including tested lowercase | −2 |
| `MindMac`, `mindmac`, `MINDMAC`, surrounding spaces |41 |
| Unknown, empty, incomplete or overlong reference | −2, after unknown-reference notice path |

`MindMacX` is not accepted as MindMac. The **seven-byte limit belongs to
technology-definition key storage**: the original definition loader copies
eight bytes, then terminates byte7. A controlled `LongKeyExtra` definition
therefore stores `LongKey`. Reference lookup itself does not truncate an
overlong token to seven bytes. It scans the currently loaded stored keys in
catalog order and returns the matching positional ID.

Native validation executes the installed technology-name loader and the
complete PROPOSALS section of the common reader, producing all11 records.
Fifteen reference-token controls execute the actual token trim and resolver;
only unknown-reference notice presentation is supplied. One additional native
definition-copy fragment checks the seven-byte limit. No pressure/majority
arithmetic, council voting or complete unknown-reference modal behavior is
rerun or newly claimed. SMAX and PRACX are outside this receipt.
