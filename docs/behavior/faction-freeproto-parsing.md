# FREEPROTO token to prototype-cost capability

2026-10-20. This closes the previously unproven token producer in
[the first-prototype handoff](design-workshop-first-prototype.md).
Original executable SHA-256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

**FREEPROTO is a presence capability. Its value is ignored. It consumes no
generic bonus record.** Parsing it ORs `0x200` into the definition flagword,
preserving every other bit. It does not clear the capability when given zero
or a negative value. Repeated entries cannot disable it.

Exact source path within the faction bonus-pair loop:

```text
59d944..59d958: compare keyword with canonical BONUSNAMES entry 28
59d95e:        read definition +508
59d964:        OR AH with 2
59d967:        write definition +508
59d96d:        jump to next pair at 59daf7
```

There is no integer conversion, value test, `+50c` record-count guard,
record append or count increment on this branch. The shared eight-record
budget applies to record-producing entries such as FACILITY; FREEPROTO
continues working after that budget is exhausted.

Definition base is `95a4c8 + owner*59c`; its `+508` flagword is therefore
`95a9d0 + owner*59c`, exactly the `0x200` consumer used by prototype-price
helpers `5d9790` and `5d9810`. Represent it separately from `Records()`.
Retaining the parsed source value as metadata is fine; interpreting that
value as an enable/disable number changes native behavior.

Fourteen controls execute the complete original `59d3be..59db0e` bonus-pair
loop. Values `0`, `-1`, `1`, `99`, `nonnumeric` and blank are tested both
with an empty record list and following eight FACILITY records. Every case
then supplies FACILITY 17: it is retained when space remains and dropped when
the list is full. Repeated `FREEPROTO,1`, `FREEPROTO,0`, `FREEPROTO,-4` preserves
the enabled flag; an absent-keyword control preserves it clear. All 1,436
definition bytes match an independent expected buffer in every case, including
an unrelated preservation flagword and untouched record storage.

Private additive freeze:
`tmp/reference/independent-freeproto-parser-evidence-20261020/`.
Manifest SHA-256:
`14100c2a79bfae109a5b4931c33ed36ade19a42e9034ed3e8567dbe1672f37a0`.
Inputs and native observations are separate files. The token reader and
installed canonical BONUSNAMES table are supplied; original comparisons,
integer conversions for the FACILITY controls and the entire pair loop execute.
This is a bonus-parser check, not a new file-lexer or full-startup claim.
No stock faction identity inference is used and no production source changed.
