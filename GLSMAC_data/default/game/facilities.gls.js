const def = (game, id, name, description, cost, pcx, source_id) => {
	game.event('define_base_facility', {
		id: id,
		def: (game.get_ruleset() == 'smac' ? { smac_source_id: source_id } : {}) + {
			name: name,
			description: description,
			cost: cost,
			render: {
				type: 'sprite',
				file: 'facs/' + pcx,
				x: 0,
				y: 0,
				w: 215,
				h: 215,
			},
		},
	});
};

const result = {
	define: (game) => {

		def(game, 'HEADQUARTERS',
			'Headquarters',
			'Administrative center of your colony. +1 ENERGY. No inefficiency. Enemy probe teams may not attempt mind control here.',
			40,
			'fac001.pcx', 1
		);

		def(game, 'RECYCLING_TANKS',
			'Recycling Tanks',
			'Increases NUTRIENT, MINERALS and ENERGY output of base square.',
			32,
			'fac003.pcx', 3
		);

		if (game.get_ruleset() == 'smac') {
			def(game, 'RECREATION_COMMONS', 'Recreation Commons', 'Fewer Drones', 40, 'fac006.pcx', 6);
			def(game, 'ENERGY_BANK', 'Energy Bank', 'Economy Bonus', 80, 'fac007.pcx', 7);
			def(game, 'PERIMETER_DEFENSE', 'Perimeter Defense', 'Defense +100%', 50, 'fac004.pcx', 4);
			def(game, 'NETWORK_NODE', 'Network Node', 'Labs Bonus', 80, 'fac008.pcx', 8);
			def(game, 'HAB_COMPLEX', 'Hab Complex', 'Increases the population limit.', 80, 'fac024.pcx', 24);
			def(game, 'HABITATION_DOME', 'Habitation Dome', 'Removes the habitation population limit.', 160, 'fac025.pcx', 25);
		}

	},
};

return result;
