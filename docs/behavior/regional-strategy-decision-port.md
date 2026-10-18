# Regional strategy decisions

`RefreshSmacRegionalStrategy` implements the strategy-code phase of the
[owner summary](regional-research-summary-producer.md) and the
[foreign-peer overrides](regional-strategy-overrides.md). It returns a staged
row of codes; it neither refreshes peer inputs nor publishes simulation state.
It preserves entry0 and visits entries1–127 in ascending order.

The expansion calculation uses retained suitable/covered/neutral counts, rating,
regional Base presence, the signed owner Base total, personality priorities,
leading-human dominance and retained terrain concentration. The foreign phase
uses retained military counts, directed relations and the prior strategy byte.
Its late-turn stagger can avoid routing entirely. Fresh routing tries strict
first, then broad only after strict failure; zero peer Base presence has its own
shortcut. Unreachable hostile peers still contribute to take0. A hostile take0
clears the Pact mask before actual take2 can override it. Empty special regions
and the final sea-region rule retain their separate order.

All route calls go through a required callback at their actual decision boundary.
The live adapter must supply Base-record/component routing and the retained path
workspace; this callback is not permission to synthesize reachability in a
gameplay caller. The helper returns strategy codes only. In particular, it makes
no claim to implement the unrelated bilateral-policy flag4 or the complete final
regional flag word. Signed intermediate calculations use wide arithmetic;
original overflow behavior outside measured ordinary inputs remains unverified.

## Validation and remaining integration

- All33 original controlled policy cases match, including the exact sequence of
  strict/broad route calls and the absence of calls on staggered/shortcut paths.
  Those tests supply literal native routing results to isolate the policy.
- All14 recorded opening region1 projections match. The reset-canary control
  also checks its nine literal region samples, including untouched entry0 and
  polar/sea boundaries. This does not claim to port all1,792 original comparison
  outputs: the public receipt does not enumerate every input/output row.
- Three of the original weak/strong/stale-presence contexts additionally compose
  real `RegionalBasesReachable` and `RegionalLandWorkspace` calls over their
  displayed land map and Base records. Existing386 path queries and66 Base-caller
  projections continue to test the separately implemented routing boundary.
- Targeted correctness analysis passes for the new production translation unit.
  This pure decision change adds no pointer ownership, wire format or threading.

This PR establishes the decision kernel, not a live AI or completed Blind
Research feature. Connecting the owner-summary caller requires saved regional
strategy rows and the independent retained signed Base-total authority. The
newly frozen `faction-base-total-lifetime` receipt establishes that latter input's
immediate action deltas, summary recount and last-Base disposition reset. Live
dominance also needs the existing retained ranking, project/catalog and victory
inputs supplied at the correct point in the owner pass. Those adapters must
publish the complete staged result after the existing Base/unit scan, rather
than refreshing at research-picker or inspector time. SMAX parity is unverified.
