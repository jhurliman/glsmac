# Live population-rule inputs

`Game::GetCanonicalPopulationRules` reads a registered canonical Base and its
authenticated local human owner. It returns owned inputs to `SmacPopulationStep`;
it does not change population, food, allocation, RNG or frontend notifications.
Objective protection remains the population caller's explicit input.

Effective GROWTH comes from the current refreshed social vector, plus two for
the actual local Children's Creche (source2) and two for saved Golden Age.
The full value remains available for boom/stagnation; only the cost helper
clamps it. Retained GROWTH is not used. Nutrient multiplier, demand and housing
thresholds come from the frozen resource profile's RULES6/2/13/14. The accepted
world category is passed verbatim, including custom99; geometry is not a category
substitute. Human cost bypasses AI leader/dominance inputs.

Housing uses population plus the faction's POPULATION scalar and independently
checks actual Hab Complex24 and Habitation Dome25. POPULATION takes the last
assignment across all frozen bonus pairs, including those after eight records.
The retained original parser listing `linear-0059d1d0.json` assigns its converted
value at 0x59d611; the installed BONUSNAMES table identifies that branch. This is
static source evidence, not a new original-executable experiment. The current
saved no-project campaign and registered facility checks establish the absence
of Ascetic Virtues and Cloning Vats. Unresolved facilities or project rows reject.

Three focused native groups use actual Bases and parsed source profiles: current
versus retained GROWTH, Creche/Golden Age, Morgan housing, repeated scalar values,
changed frozen housing limits and rejection without publication. The getter and
test TU pass cached syntax checks. Linked execution is pending the root's combined
population/first-Base/production build; no full-suite or platform run is claimed.
