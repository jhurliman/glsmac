# First Base introduction and naming

Fresh local canonical games now pause at turn zero to name their existing first
Base, before the first advance, upkeep, research interaction and human commands.
This follows the [original caller order](behavior/firstbase-startup-schedule.md),
not the later ordinary Base Rename command. Naming does not create another Base,
reapply starting resources or consume gameplay RNG.

The initialization admission retains a first-turn continuation while awaiting
an owned result. Each request has a monotonic identity, world ticket and session;
reset/retired-world results cannot rename a later world's Base. Results always
enter the claimed ingress phase, including calls from a synchronous prompt
listener. Ordinary gameplay and saving remain unavailable until introduction
completes. Campaign restores skip this fresh-start stage and preserve the normal
saved research/upkeep continuation. Saving inside the original active FIRSTBASE
modal has not been measured; this port does not offer it.

The prompt uses the first live owned Base in record order, its suggested name,
a23-byte narrow edit limit and original x10/y40/width320 template position.
No-Base starts skip naming. Every modal return transfers current text, including
Cancel/Escape, empty text and spaces, following the original copy boundary.
Duplicate normalization counts case-sensitive exact matches among other Bases,
then uses at most21 input bytes plus `-` and decimal(count+1), once. It does not
search for an unused suffix. The renderer currently admits printable ASCII;
full original code-page and selection behavior remain unfinished.

The input is created only when needed, explicitly focused and removed after
completion. DOM text is copied into an owned string before the asynchronous
handoff. The existing Base is published and centered while startup is paused.
The prompt is functional, but its border/font/layout, console attachment rails
and landing animations are not a screen-parity claim. Full main-map unknown
terrain masking remains a separate missing feature.

## Validation

- 20 focused native startup, conventional-unit, research, reset and Rename groups
 pass; the new scenarios verify pre-upkeep ordering, deferred/duplicate results,
 stale-session retirement, unchanged RNG/entity counts and exact normalization.
- Actual input-widget focus/keyboard test and FIRSTBASE GSE interaction script pass.
- Targeted analysis: Game.cpp, GameIntroduction.cpp, GameCanonicalTurn.cpp,
 BaseRenameCommands.cpp, affected GameCampaign.cpp, Input.cpp and Container.cpp;
 zero failed analyses/findings. The presentation change to GameIntroduction.cpp
 was rechecked.
- Focused ASan/UBSan covers seven instrumented translation units (the four changed
 backend implementation files, Backend.cpp, Input.cpp and UIClass.cpp). All eight
 selected groups pass; other objects are normal, leak detection disabled and
 stack-use-after-return enabled. This is not a full sanitized engine matrix.
- A real1024x768 hidden-SDL/OpenGL session typed `b` without clicking the input,
 submitted `Morgan Industriesb`, selected research and reached the playable map.
 Saving while research was pending and loading in a separate normal-menu session
 retained that edited Base name and resumed research without replaying FIRSTBASE.
 Both sessions quit normally with exit0 and no tracked OpenGL resource leaks;
 allocation tracking was not enabled. The campaign SHA256 was
 `98bf8408d29ec6bea6eedbe14a58bb7dac34a947c07b8bee6878cf312d762606`.

The ten-file static original scheduling receipt was verified against manifest
SHA256 `d56971856cf9d5e45cd65d8531d11159d7ffd6e6dc194176cc441a0b663e8428`.
It establishes source boundaries, not an original modal framebuffer execution.

![Actual first-base edit before initial upkeep](https://raw.githubusercontent.com/jhurliman/glsmac/8598a8cfa418be9e73c709ef3d8d2824b57359e6/first-base-introduction/edited.png)

![Actual edited Base after campaign restore](https://raw.githubusercontent.com/jhurliman/glsmac/8598a8cfa418be9e73c709ef3d8d2824b57359e6/first-base-introduction/restored-gameplay.png)
