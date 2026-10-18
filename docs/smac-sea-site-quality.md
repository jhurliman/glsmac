# Sea-centered site quality

`map::smac_site::Quality` extends the land query with the original ordinary-turn
water branch. Full-map territory refill visits water and polar tiles too, so
those scores cannot be replaced with skipped entries or default1.

The water center counts toward water/depth totals and can satisfy the stored
area50 bonus, including through polar region127. Its resource contributes no
point. The last valid inner square still supplies the coastal bonus. Outer Base
markers contribute1 normally,2 at cardinal distance4 on land, or4 at cardinal
distance4 on water. The water score doubles that accumulator's penalty and
rejects the site when the undoubled accumulator reaches4. Deep center fungus
and retained water rockiness do not trigger land rejection; shelf fungus and
monoliths reject. A center Base marker is not a separate rejection.

[Forty frozen complete original queries](../src/gse/tests/fixtures/SmacSeaSiteQuality.h)
compare results and score terms for those discriminators, clipped poles,
horizontal wrap/flat edges, resolved resource/site inputs, Base-radius
exclusions and both clamps. The fixture records the native receipt's SHA256.
All31 original land cases also exercise the shared entry. These are supplied
decoded inputs, not a new generator, rendered-game or founding acceptance claim.

The query remains pure and limited to ordinary turns1..100. It neither refreshes
cache state nor draws randomness. Actual saved cache ownership, invalidation
and territory refill remain the next joined live producer. The late-turn fungus
adjustment, original generation and AI consumers are separate contracts.

Validation: cached Linux Debug build; three affected native groups with
allocation tracking (31 land cases,40 sea cases and domain rejection); scoped
LLVM15 correctness analysis passes for the query and prepared cache consumer.
