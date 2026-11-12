// Canonical native birth presentations must be available in the real default
// definitions. No proprietary atlas pixels are included in this regression.
const definitions = #include('../default/units/defs');
const units = {};
const game = {
    get_ruleset: () => { return 'smac'; },
    get_player: () => { return {is_single_player: true}; },
    configure_smac_unit_presentations: (roles) => {},
    event: (name, record) => { if (name == 'define_unit') { units[record.id] = record.data; } },
};
definitions.define(game);
test.assert(units.MindWorms.movement_type == 'land', 'Mind Worm presentation');
test.assert(units.IsleOfTheDeep.movement_type == 'water', 'Isle presentation missing from ordinary startup');
test.assert(units.LocustsOfChiron.movement_type == 'air', 'Locust presentation missing from ordinary startup');
for (id of ['MindWorms', 'IsleOfTheDeep', 'LocustsOfChiron']) {
    const render = units[id].render;
    for (let level = 0; level < 7; level++) {
        test.assert(render.x + level * render.morale_based_xshift + render.w <= 1024, 'Native lifecycle art exceeds installed atlas width');
        test.assert(render.y + render.h <= 768, 'Native art exceeds installed atlas height');
    }
}
test.assert(units.IsleOfTheDeep.render.morale_based_xshift == 0, 'Fixed Isle cell must not walk into other sprites');
test.assert(units.LocustsOfChiron.render.morale_based_xshift == 0, 'Fixed Locust cell must not walk outside the atlas');
