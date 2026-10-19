# Live faction console summary

The lower console reads a copied summary for the authenticated local single-player
participant. The zero-argument Game callable does not accept a foreign Player or
simulation ID and never refreshes ranking, history, population caches or RNG.
Diagnostic worlds without an admitted simulation return unavailable.

Integrity uses the retained `SmacHostileOwner.integrity` word. The original
initializer and text reader identify the same field, 95d2cc. Interpret its bits as
signed32, clamp to0..7, then index the source `#REPUTE` labels.

Might uses the saved rank from `SmacRankOrder`, not an ordinary ranking score.
Original ranking writes the faction rank at95d1ac, the same field read by the
text builder. The port's inverse `faction_at_rank` table preserves active ranks
1..7; the `#MIGHT` primary labels are in descending order, so source index is
7−clamp(rank,1,7). An absent table, absent active rank or inactive participant
keeps Might unavailable. The port's inverse table drops inactive positions and
cannot recover their original per-faction stored ranks.

Council votes call the existing source-backed `SmacCouncilVotes` arithmetic with
current Base owners/populations and frozen faction VOTES records. Project bonuses
are absent only under the explicit retained `NoProjectsOrdinaryTerrain`
capability. Missing project authority does not imply false project ownership.
An empty admitted Base set genuinely yields zero votes.

Global settings V21 freezes optional eight-row REPUTE and seven-row MIGHT label
catalogs from the selected rules text. The first MIGHT column supplies the display
word; its second adjective is not substituted. V17–V20 restores retain absence,
and loading never consults changed installed labels. Both missing sections are
supported for diagnostic profiles; partial/duplicate/malformed catalogs reject.
Each retained label is bounded to256 bytes, and late parse failures leave live
settings unchanged. UI refreshes the values on activation, turns and player
updates with the existing native text geometry and highlight color.

The current eight-word `ranking_scores` table is consumed to−1 sentinels by full
ranking; it is not a graph series. No retained timestamped dominance samples or
source-backed sampling cadence exists. The history panel therefore remains
explicitly unavailable. No substitute curves or fresh-score query are introduced.
