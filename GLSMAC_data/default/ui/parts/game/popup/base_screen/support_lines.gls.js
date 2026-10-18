// Presentation only; the native provider owns ordering, homes and support rules.
return (presentation, detailed) => {
	if (!#is_defined(presentation)) { return ['Support unavailable']; }
	const current = presentation.current;
	let lines = ['Current preview: ' + #to_string(current.minerals) + ' minerals'];
	if (detailed) {
		lines :+ 'Eligible: ' + #to_string(current.eligible) + '  Free allowance: ' + #to_string(current.free_allowance) + '  Paid: ' + #to_string(current.paid);
	}
	if (#sizeof(current.units) == 0) { lines :+ 'No units homed here'; }
	for (unit of current.units) {
		let line = unit.name + ' #' + #to_string(unit.id) + ': ' + unit.status;
		if (unit.minerals > 0) { line += ' (' + #to_string(unit.minerals) + ')'; }
		if (detailed) { line += ' at ' + #to_string(unit.x) + ',' + #to_string(unit.y); }
		lines :+ line;
	}
	if (#is_defined(presentation.retained)) {
		lines :+ 'Last resource pass: ' + #to_string(presentation.retained.minerals) + ' minerals';
	} else { lines :+ 'Last resource pass unavailable'; }
	return lines;
};
