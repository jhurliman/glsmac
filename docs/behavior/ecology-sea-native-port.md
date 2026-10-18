# Native Isle and Locust records and queries

Production native records now initialize designs8/9/10 independently of controller ownership. The live Unit query resolves each admitted design from the frozen loaded component packet: Foil/Psi/reactor1/carry4 for Isle9, Gravship/Psi/reactor1/ability32 for Locust10. Locust's intrinsic native level increment is applied before clamping; stored morale and ecology attribution remain independent.

The existing allowance helper is reused after native component/level resolution. Negative Psi suppresses elite movement. Injured Isle allowance rounds whole points before Q with a two-point floor; air Locust bypasses damage scaling. Queries retain clamped zero-health results for supplied damage bytes, while live Unit creation still rejects dead records. No damage/combat producer is implied.

Source fixtures: ecology-sea-native-records-cases.json contains six complete returned and six final records plus60 query sets. native-damaged-allowance-cases.json contains16 original damage controls. Tests compare literal records, caller tags, native links, lifecycle boundaries, immutable queries, live Unit decode/encode/presentation and RNG preservation. The fixture's source wrapper supplies the original carry/cost rows without altering the shared synthetic catalog.

Live ecology birth batching, sight/stack publication, nonnegative assignment references, native AI, project effects, and native frontend artwork remain unfinished. This change does not claim completed spawning or visual parity.
