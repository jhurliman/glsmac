return (settings) => {
	if (settings.ruleset == 'smac') {
		const coverage = [0.4, 0.6, 0.8];
		const choice = #random_int(0, 2);
		settings.map.ocean_coverage = coverage[choice];
	} else {
		settings.map.ocean_coverage = #random_float(0.4, 0.8);
	}
	settings.map.erosive_forces = #random_float(0.5, 1.0);
	settings.map.native_lifeforms = #random_float(0.25, 0.75);
	settings.map.cloud_cover = #random_float(0.25, 0.75);
};
