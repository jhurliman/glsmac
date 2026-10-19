# Seven-faction Standard Planet startup and first scheduling boundary

[Numeric receipt](seven-faction-planet-start-cases.json). Pinned original SMAC, **Librarian difficulty3**, Standard Rules`0x1a0e`, fresh secondary word0, local non-network game, installed Map of Planet, stock faction order. Gaia/native slot1 is the only human; slots2..7 are computer factions. Explicit CRT/shared seeds1 are supplied after native Planet preparation and before the actual setup controller. This pins a reproducible start; it does not reconstruct the frontend entropy/clock lifecycle.

The original map loader, map-search buffer initializer, terrain/region preparation, difficulty/rules/faction setup writers, all faction initialization and normal setup balancing execute. No starting site, Base, Unit, technology, treasury or resource result is supplied. UI answers select the stated setup; presentation/intro services retain explicit headless boundaries. All seven factions successfully start.

## Canonical state after setup

Human membership is **`0x02`**, while active membership is **`0xff`**. The latter also retains wildlife slot0, which has no starting Base/Unit here. These are separate authorities. A local session is not a roster containing only one faction, and a computer opponent does not need a human bit. Native initialization processes human1 first, then nonhuman0,2,3,4,5,6,7. Both compared actors have stored difficulty3; the broader existing producer writes global difficulty for humans and3 for computers.

**Standard Rules disables Look First and Time Warp**, so every positive slot first receives one immediate population1 HQ Base and one selected Scout. There is no initial Base absence on this setup. The six conventional factions use predefined Scout Patrol2; Spartans use generated Recon Speeder320. No initial Foil qualifies in this specific prepared world. All starting Units have damage0, stored morale1, spent0, order0 and home−1.

Normal setup balancing subsequently changes the package. In this pinned start it adds a Colony Pod to slots1,2,3,5,6,7, leaving Morgan without one: **seven Bases, thirteen Units**. Thus both human Gaia and computer Hive finish with **Base + Scout + Colony Pod**. This is not an unconditional computer bonus or the Look First Pod-versus-Base choice. The initial package has seven Scouts and no Pods; the Pods are separate native balancing grants.

The source Pod gate uses the balancing routine's computed deficit`D`: for global difficulty above1, grant when`D>=3` or difficulty is at least4 for a human/5 for a computer. Grant subtracts3 from the remaining deficit and clears the new Pod's home. At Librarian3 this case therefore grants exactly where`D>=3`; recorded deficits for slots1..7 are **10,13,10,0,13,10,13**. The full native balancing routine, including its terrain work, executes. This handoff does not provide a new independent implementation of its upstream deficit/site-quality calculation. Do not hardcode the observed Pod count for different maps, sites, seeds or rules.

| State | Human Gaia1 | Computer Hive2 |
|---|---|---|
| Base coordinate |(17,61)|(34,36)|
| Population / nutrient box |1 /0|1 /0|
| Facilities |HQ|HQ + Perimeter Defense|
| Production / stored minerals |Formers /10|Scout Patrol /5|
| Starting map Units after balancing |Scout2 + Colony Pod0|Scout2 + Colony Pod0|
| Treasury / pending debit |10 /0|10 /0|
| Psych / labs / residual economy |0 /5 /5 tenths|0 /5 /5 tenths|
| Named owned technology |Centauri Ecology6|Doctrine: Loyalty43|
| Research target / threshold cache |−1 /−1|−1 /−1|
| Labs / fraction / score / discount / deferred awards |all0|all0|

Morgan, a computer here, starts with110 credits: starting-energy rule10 plus ENERGY100. Other factions start10. University owns its named starting technology2; its extra numeric TECH award has not run before the first human1 command because University3 has not had its turn1 upkeep yet. Named ownership and technology score remain separate.

## Owning controller and computer handoff

The actual local controller enters at turn0/resume0, runs full global advance to **turn1/year2101**, then native wildlife0 upkeep/empty turn and human1 upkeep. It stops at the original human map-command entry, caller`0x53cb4f`. No human command body or order executes. Gaia's map-specific first upkeep stores nutrient2/minerals12 and treasury11; other factions have not yet received their own faction upkeep.

A **separate admitted continuation** starts at the native next-slot increment using that real controller frame, bypassing the unexecuted human command body. It executes computer Hive2 upkeep and reaches the AI turn entry, caller`0x53cb67`, then stops before its body. Hive's Base stores nutrient1/minerals7; treasury stays10. Global turn remains1, focus remains human1, scheduled faction becomes2, human membership remains`0x02`. This establishes the real computer scheduling connection without a second-human workaround. It is not a claim that a human ended a turn or that an AI algorithm executed.

Both compared post-upkeep states still have no research target, invalid threshold cache and zero research labs/fraction/score. Resource postings are specific to the prepared Planet sites and actual worker calculations, not universal per-turn constants. The computer upkeep changes Hive Scout1's order byte from0 to24 before the AI turn entry; its order-parameter producer is not separately audited here. Positions, homes, damage, morale and spent movement remain unchanged. Thus AI-relevant preparation already occurs inside upkeep; do not assume every computer order decision starts in the later AI turn body. No gameplay chooser is answered.

## Map-search initialization correction

The accepted run executes native`0x5b1ab0`, which allocates a32-bit visited array and two16-bit coordinate queues for the map. For3200 tiles their sizes are12800/6400/6400 bytes. An inherited diagnostic callback had incorrectly labeled this a display reset and suppressed it. Exploratory v1..v5 startup placement/compensation results are excluded: null queues corrupted their region-dependent inputs. Accepted v7 runs the initializer, all canonical reset writers and a guard against null-array writes. Earlier immutable bundles remain unchanged; the raw map-loader geometry/header/tile assertions remain distinct from complete initialized-map claims.

Checker`tmp/reference/check_seven_faction_planet_start.py --verify-only` verifies all seven starting profiles, original package versus balancing grants, initial research/economy, human/computer membership and both scheduling boundaries. Owned maps/assets remain external; no production edits, human orders, full AI audit or live screenshot are claimed. Frozen supplement:`tmp/reference/seven-faction-planet-start-evidence-20261019`.
