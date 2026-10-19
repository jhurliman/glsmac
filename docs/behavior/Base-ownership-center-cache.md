# Base ownership changes update the center's occupying affiliation

**Both successful surviving-Base capture and riot transfer explicitly set the
Base center's cached occupying affiliation to the recipient.** This write is
independent of any Unit conversion or placement on the center. It preserves the
high site-rating nibble in the same byte.

Ordinary capture performs its center write after the preceding ownership,
facility and home/production work, before its later presentation. In the reused
complete undefended capture, center affiliation changes from defender 2 to
attacker 1. The attacker already occupies the center when the capture routine
begins and already belongs to faction 1; no Unit conversion establishes this
cache change. The source's center setter has no center-Unit admission guard.
The empty-center execution discriminator below is for riot. The complete capture/inspector-close
and outer movement return use the previously accepted virtual-show boundary.

Riot transfer performs its own final center write after facility grants and
before final support/intake/nutrient calculations. It is additional to the
per-tile writes caused by eligible Unit conversions. A complete Base-upkeep
control first deletes all center Units through the original deletion helper;
the center remains empty throughout transfer, affiliation changes 1 to recipient
2, and Base upkeep returns its ordinary ownership-change result 1. The diagnostic
high site-rating nibble A survives the center setter. Base presence remains set.

Implement these writes at the respective ownership transaction points. Keep the
existing [placement/removal/cache history rules](climate-initialization-and-occupant-cache.md)
and separate load repairs; no global recount of Base owners is implied. The
21-radius attribution pass is another field and does not replace this center
cache update. This follow-up does not extend the claim to Base destruction,
raze, subversion, editor ownership changes or every combat branch.

[Literal controls](Base-ownership-center-cache-cases.json). Broad RE remains paused.
