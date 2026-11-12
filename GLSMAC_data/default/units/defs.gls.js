const moralesets = [
	{
		id: 'NATIVE',
		data: [
			{name: 'Hatchling'},
			{name: 'Larval Mass'},
			{name: 'Pre-Boil'},
			{name: 'Boil'},
			{name: 'Mature Boil'},
			{name: 'Great Boil'},
			{name: 'Demon Boil'},
		]
	},
];

const native_lifeform = (id, name, movement_type, movement_per_turn, base_y, base_x, morale_shift) => {
	return {
		id: id,
		data: {
			name: name,
			morale: 'NATIVE',
			type: 'static',
			movement_type: movement_type,
			movement_per_turn: movement_per_turn,
			render: {
				type: 'sprite',
				file: 'units.pcx',
				x: base_x, y: base_y,
				w: 100, h: 75,
				cx: base_x + 51, cy: base_y + 51,
				morale_based_xshift: morale_shift
			},
		},
	};
};

const units = [
	native_lifeform('FungalTower', 'Fungal Tower', 'immovable', 0, 79, 2, 102),
	native_lifeform('MindWorms', 'Mind Worms', 'land', 1, 233, 2, 102),
	// Stock single-cell native artwork in the installed units.pcx atlas.
	// These bodies do not select the adjacent cells by lifecycle morale.
	native_lifeform('IsleOfTheDeep', 'Isle of the Deep', 'water', 4, 2, 2, 0),
	native_lifeform('LocustsOfChiron', 'Locusts of Chiron', 'air', 8, 2, 716, 0),
	native_lifeform('SeaLurk', 'Sea Lurk', 'water', 4, 310, 2, 102),
	native_lifeform('SporeLauncher', 'Spore Launcher', 'land', 1, 387, 2, 102),
];

// GLSMAC camera policy: preserve the legacy 100x75 sprite's pixel scale
// (1.0 by 0.8 world units) on the CVR 256x256 canvas. Use the raster's
// nominal model origin (128,128), not its separate target-blit anchor.
// These are port placement choices, not original visual parity.
const conventional = (id, name, composition) => {
	return {id: id, data: {
		name: name, morale: 'CONVENTIONAL', type: 'static', movement_type: 'land',
		movement_per_turn: 0, // unused legacy field; owned integer stats supply movement
		render: {
			type: composition, facing: 2, zoom: 0, scale_numerator: 1, scale_denominator: 1,
			world_width: 2.56, world_height: 2.730666666666667, center_x: 128.0, center_y: 128.0,
		},
	}};
};

const configure = (game) => {
    if (game.get_ruleset() == 'smac' && game.get_player().is_single_player) {
        game.configure_smac_unit_presentations({scout: 'ConventionalScout', selected_spartan_rover: 'ConventionalSpartanRover', former: 'ConventionalFormer', colony: 'ConventionalColony'});
    }
};
const result = {
    configure: configure,
	define: (game) => {

		for (moraleset of moralesets) {
			game.event('define_moraleset', moraleset);
		}

		for (unit of units) {
			game.event('define_unit', unit);
		}

		if (game.get_ruleset() == 'smac' && game.get_player().is_single_player) {
			game.event('define_moraleset', {id: 'CONVENTIONAL', data: [
				{name: 'Very Green'}, {name: 'Green'}, {name: 'Disciplined'}, {name: 'Hardened'},
				{name: 'Veteran'}, {name: 'Commando'}, {name: 'Elite'},
			]});
			game.event('define_unit', conventional('ConventionalScout', 'Scout Patrol', 'cvr_initialized_scout'));
			game.event('define_unit', conventional('ConventionalSpartanRover', 'Scout Rover', 'cvr_spartan_rover'));
			// Explicit temporary GLSMAC marker until the installed Former recipe
			// is connected. It never borrows Scout/Rover artwork.
			game.event('define_unit', {id: 'ConventionalFormer', data: {
				name: 'Formers', morale: 'CONVENTIONAL', type: 'static', movement_type: 'land',
				movement_per_turn: 0, render: {type: 'port_former_marker'},
			}});
			game.event('define_unit', {id: 'ConventionalColony', data: {
				name: 'Colony Pod', morale: 'CONVENTIONAL', type: 'static', movement_type: 'land',
				movement_per_turn: 0, render: {type: 'port_colony_marker'},
			}});
			configure(game);
		}

	},
};

return result;
