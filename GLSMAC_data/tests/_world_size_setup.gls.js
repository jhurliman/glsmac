// Actual menu/handler code, real native settings; only UI factories are supplied.
return (game, settings, smac) => {
	const check = (ok, message) => { if (!ok) { throw TestError(message); } };
	let displayed = null;
	let popup = null;
	let continued = 0;
	let errors = 0;
	const i = {
		glsmac: {game: game}, settings: {global: settings},
		sliding: {show: (data) => { displayed = data; }},
		popup: {show: (data) => { popup = data; }, dismiss: () => {}, error: (message) => { check(message != '', 'missing error message'); errors++; }},
		steps: {select_difficulty_level: (unused) => { continued++; }, customize_ocean_coverage: (unused) => { continued++; }},
	};
	const menu = #include('../default/ui/parts/mainmenu/steps/select_mapsize');
	menu(i);
	check(#sizeof(displayed.entries) == (smac ? 6 : 5), 'wrong ruleset menu');
	if (!smac) {
		((displayed.entries[0])[1])();
		check(settings.map.size_x == 68 && settings.map.size_y == 34 && (settings.get_world_size())[0] == -1, 'SMAX acquired SMAC semantics');
		return;
	}
	check((settings.get_world_size())[0] == -1, 'menu display accepted a category');
	for (let index = 0; index < 5; index++) {
		((displayed.entries[index])[1])();
		check((settings.get_world_size())[0] == index, 'preset callback lost its row identity');
	}
	check(continued == 5, 'preset did not continue exactly once');
	const before_cancel = settings.get_world_size();
	((displayed.entries[5])[1])();
	(popup.buttons[1]).onclick();
	check(settings.get_world_size() == before_cancel, 'cancel committed custom selection');
	let inputs = [];
	popup.generator({text: (data) => {}, input: (data) => { const input = {value: data.value}; inputs :+input; return input; }});
	(inputs[0]).value = '48'; (inputs[1]).value = '49';
	(popup.buttons[0]).onclick();
	check(errors == 1 && settings.get_world_size() == before_cancel && continued == 5, 'unsupported odd height changed selection');
	(inputs[1]).value = '48';
	(popup.buttons[0]).onclick();
	check(settings.get_world_size() == [99, 48, 48] && continued == 6, 'custom matching preset became a preset');
	const handler = #include('../default/game/event/game_settings');
	for (changes of [
		[['planet_size', '0'], ['planet_size', '1']],
		[['size_x', 48], ['planet_size', '1']],
		[['planet_size', '1'], ['size_y', 48]],
		[['size_x', 48], ['size_y', 48]],
	]) {
		check(#is_defined(handler.validate({caller: 0, game: game, data: {changes: changes}})), 'ambiguous world-size transaction accepted');
		check(settings.get_world_size() == [99, 48, 48], 'rejected size transaction changed acceptance');
	}
	const event = {caller: 0, game: game, data: {changes: [['planet_size', '1']]}};
	check(!#is_defined(handler.validate(event)), 'lobby rejected master');
	const applied = handler.apply(event);
	check(settings.get_world_size() == [1, 48, 48], 'actual lobby handler inferred category from dimensions');
	handler.rollback(event + {applied: applied});
	check(settings.get_world_size() == [99, 48, 48], 'lobby rollback lost custom identity');
	// Actual lobby producer: informational entries cannot visually replace the
	// accepted native choice without issuing an authoritative change.
	let size_control = null;
	#include('../default/ui/parts/mainmenu/steps/multiplayer_lobby/game_settings')({
		glsmac: {game: game}, make_section: (title, style, generate) => {
			generate({text: (data) => {}, listen: (object, event, callback) => {}, select: (data) => {
				let control = null;
				control = {value: data.value, callback: null, on: (event, callback) => { control.callback = callback; }};
				if ((data.items[0])[0] == '-1') { size_control = control; }
				return control;
			}}, null);
		},
	});
	check(size_control.value == '99', 'lobby did not display accepted custom identity');
	size_control.value = '-1'; size_control.callback({value: '-1'});
	check(size_control.value == '99' && settings.get_world_size() == [99, 48, 48], 'placeholder visually replaced accepted size');
	settings.map.size_x = 50;
	check((settings.get_world_size())[0] == -1, 'raw dimension edit kept a forged preset/custom identity');
};
