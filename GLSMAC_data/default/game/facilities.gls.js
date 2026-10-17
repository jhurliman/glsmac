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
			def(game, 'PERIMETER_DEFENSE', 'Perimeter Defense', 'Defense +100%', 50, 'fac004.pcx', 4);
			def(game, 'NETWORK_NODE', 'Network Node', 'Labs Bonus', 80, 'fac008.pcx', 8);
		}

	},
};

return result;
