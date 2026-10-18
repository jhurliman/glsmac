const install = #include('../default/game/game');
for (restoring of [false, true]) {
    const mode = restoring;
    let app_handlers = {};
    let game_handlers = {};
    let definitions = [];
    let base_roles = undefined;
    let unit_roles = undefined;
    const manager = {on: (name, callback) => {}};
    const game = {
        is_campaign_restore: () => { return mode; },
        get_ruleset: () => { return 'smac'; },
        get_player: () => { return {is_single_player: true}; },
        get_bm: () => { return manager; },
        get_um: () => { return manager; },
        get_tm: () => { return manager; },
        register_event: (name, handler) => {},
        on: (name, callback) => { game_handlers[name] = callback; },
        event: (name, data) => { definitions :+ name; },
        configure_smac_base_presentations: (roles) => { base_roles = roles; },
        configure_smac_unit_presentations: (roles) => { unit_roles = roles; },
    };
    const app = {
        game: game, config: {},
        on: (name, callback) => { app_handlers[name] = callback; },
    };
    install(app);
    app_handlers.configure_game({game: game});
    game_handlers.configure({game: game});
    test.assert(base_roles == ['WORKER', 'TECHNICIAN', 'DOCTOR', 'LIBRARIAN', 'ENGINEER', 'EMPATH', 'THINKER', 'TRANSCEND']);
    test.assert(unit_roles.scout == 'ConventionalScout' && unit_roles.selected_spartan_rover == 'ConventionalSpartanRover');
    test.assert(unit_roles.former == 'ConventionalFormer' && unit_roles.colony == 'ConventionalColony');
    test.assert(#is_defined(game_handlers.start) && #is_defined(game_handlers.create_world));
    if (mode) {
        test.assert(definitions == [], 'restore configure prepopulated saved definition managers');
    } else {
        test.assert(#sizeof(definitions) > 30, 'fresh configuration omitted original definition producers');
    }
}
