# RE-SMAC-MAINTENANCE-INSOLVENCY-001: maintenance continuation

Four focused original-SMAC contexts return through **four complete faction
upkeeps and eight complete Base upkeeps**. All use a human faction at Librarian,
governors disabled, no maintenance-discount project, ordinary positive installed
Bank/Node fees and a Former production target. [Literal cases](facility-maintenance-insolvency-cases.json)
distinguish per-Base ordering, exact zero, automatic facility removal and a
foreign Base. This is original-code emulation with supplied state, not live play.

## Settlement rule

For each owned Base, in the original global Base-record traversal order:

1. Complete resource/population processing and submit this Base's research.
2. Add this Base's computed economy to faction credits.
3. Visit maintained local facilities in ascending facility index. For each
   present facility, resolve its individual applicable fee, subtract it from
   current credits and add it to the faction's charged-maintenance statistic.
4. If credits remain nonnegative, continue. **Exactly zero is payable.**
5. At covered Librarian difficulty, with a normal nonfacility production target,
   a charge that makes credits negative removes **that same facility**. Set
   faction credits to its resolved sale value. This is an assignment: do not
   add the sale value to the negative balance, preserve that deficit, or refund
   the fee/statistic. Request the sale notification and continue with the next
   facility at this Base.
6. Continue the ordinary governor/Base tail, return, and visit the next owned
   Base. Pending faction debit is settled separately at the faction tail.

Thus there is no faction-wide frozen-fee subtraction followed by a single
insolvency decision. There is no random selection, cheapest-facility selection,
or tie lottery: the facility whose charge crosses below zero is removed at
that position in the ascending scan. Bank precedes Node. A later Base's income
cannot rescue an earlier facility before its charge.

The checked sale value is the facility's rules cost multiplied by the original
mineral cost factor for this faction with no Base context. Bank and Node have
installed cost8 and fee1; the reached factor is10, yielding **80 credits** for
either sale. Use the original applicable factor, not a newly invented universal
80-credit price or half-price sale. Allocation failures and nonstandard signed
cost/fee rules are outside these cases.

## Four distinguishing cases

Each context has two actually founded owned Bases. The poor Base has Bank and
Node, no HQ and Cloud Cover, producing economy0. The other Base has HQ and
produces economy1. These facility/event assignments and installed numeric rows
are explicit inputs; their complete prior construction/event histories are not
replayed. Both local fees are1.

| Case | Processing order and initial credits | Outcome |
| --- | --- | --- |
| Funded | Poor then HQ,100 credits | Keep both;100→99→98 at poor Base, then99 after HQ income |
| Poor first | Poor then HQ,0 credits | Bank charge0→−1; remove Bank, set80; Node charge→79; HQ income→80 |
| Poor last | HQ then poor,0 credits | HQ income→1; Bank charge→0 and survives; Node charge→−1; remove Node, set80 |
| Foreign interleaved | Poor, foreign, HQ;0 own credits | Same Bank loss as poor-first; foreign Base and its321 credits unchanged |

All four charge maintenance2, including the fee of the removed facility. Three
sales occur across the cases. All finish with the research increment from the
already computed outputs. No decision dialog is supplied: the reached path
requests a notification only, then proceeds normally.

Each owned Base has its usual two resource/intake/energy/citizen passes before
maintenance. **Removing the facility does not rerun those stages in this upkeep**,
repost income/research, recalculate the current fee total from surviving
facilities, or restart the facility scan. The normal governor entry still runs
once per Base, with automation disabled here. Recalculation by later explicit
callers and next-turn benefits are outside this continuation check.

Neither the facility scan nor its insolvency branch draws RNG in these cases.
The complete faction upkeep makes two separate CRT draws in ecology, one per
owned Base, in both funded and insolvent controls. Shared gameplay RNG state is
unchanged. The tested maintenance path leaves the retained faction status
unchanged; no new insolvency-specific retained counter was observed.

## Limits relevant to implementation

The original has separate branches for the two lowest stored difficulty levels
and for a Base currently targeting the very facility whose fee causes the
deficit: those branches zero credits instead of taking this removal path.
That is static evidence only here; it is not a runtime acceptance claim for
those contexts. Maintenance exemptions/discount projects, scenario/AI/network,
arbitrary facilities and abnormal existing debt are not expanded by this receipt.

The complete current-faction routine returns normally after its second owned
Base; a foreign Base is skipped. The outer global controller's next-faction
dispatch is not executed. Original resource, research, maintenance, facility-bit
removal, sale-factor and traversal instructions execute; inherited neutral
notification/destructor/deallocation services and absent-view diagnostic backing
are supplied. Original binaries/assets and gameplay C++ are unchanged.
