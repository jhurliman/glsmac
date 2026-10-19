const randomize = #include('../default/ui/parts/mainmenu/map_randomization');
for (ruleset of ['smac', 'smax']) {
    // A retained main-menu wrapper predates the player's ruleset selection.
    const settings = {ruleset: 'unselected', map: {}};
    for (let i = 0; i < 20; i++) {
        randomize(settings, ruleset);
        const map = settings.map;
        test.assert(map.erosive_forces >= 0.5 && map.erosive_forces <= 1.0);
        for (value of [map.native_lifeforms, map.cloud_cover]) {
            test.assert(value >= 0.25 && value <= 0.75);
            if (ruleset == 'smac') { test.assert(value == 0.25 || value == 0.5 || value == 0.75); }
        }
    }
}
