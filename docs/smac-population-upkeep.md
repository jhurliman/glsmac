# SMAC population upkeep

`SmacPopulationStep` prepares the population branch after production. Its owned
plan distinguishes collection, starvation/removal, ordinary growth, boom,
stagnation and housing limits. It does not publish Base or worker state and is
not exposed as a repeatable script command.

Ordinary growth resets food, requests actual resource recalculation at the new
population, then collects that new surplus. Boom recalculates even at population
127 and caps food to the old threshold **after** nutrient normalization, without
collection. Starvation survivors retain the pre-loss surplus; the outer upkeep
must still perform its unconditional second resource pass. A removal result is
a request for the real Base teardown, not a claim that teardown was implemented.

The nutrient factor preserves separate rule/world truncation and final growth
rounding, including zero costs. Housing checks independently require Complex
and Dome. Human cost ignores AI adaptive inputs; the supplied AI cost helper
requires resolved leader/dominance inputs and does not implement their lifecycle.
Objective protection is an explicit input. Signed32 overflow and the unobserved
ordinary-growth edge above population127 reject instead of inventing behavior.

The test fixture copies 36 self-authored `RE-SMAC-GROW-001` observations and 20
`RE-SMAC-NUTRIENT-COST-001` observations from the research workspace. Expected
internal recalculation calls are taken from its original-x86 result receipt.
Those population cases supply the post-growth resource solver; they are not
full-turn port tests. The live caller must obtain actual candidate-population
worker/resource results before finishing and publishing the plan.
