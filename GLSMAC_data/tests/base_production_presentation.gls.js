// Actual stored-panel methods: no synthetic resource/production computation.
const production = #include('../default/ui/parts/game/popup/base_screen/bottom_bar/production');
const queue = #include('../default/ui/parts/game/popup/base_screen/bottom_bar/queue');
let lines = [];
const items = {
	clear: () => { lines = []; },
	text: (properties) => { lines :+ properties.text; },
};
production.production_title = {};
production.production_cells = items;
production.production_label = {};
queue.items = items;
let editable = true;
queue.p = {utils: {can_edit_queue: () => { return editable; }}};
queue.edit_button = {};
queue.hurry_button = {};
const presentation = {name: 'Recon Rover', queue: ['Former', 'Colony Pod', 'HQ', 'Tanks', 'Bank', 'Node', 'Scout', 'Scout', 'Scout']};
production.set_stored({minerals: 17}, presentation);
test.assert(production.production_title.text == 'Recon Rover');
test.assert(lines == ['Stored: 17']);
queue.set_stored(presentation);
test.assert(lines == presentation.queue);
test.assert(queue.edit_button.text == 'EDIT QUEUE');
editable = false; queue.set_stored(presentation);
test.assert(lines == presentation.queue && queue.edit_button.text == 'Queue unavailable');
queue.set_stored({name: 'Recon Rover', queue: []});
test.assert(lines == ['Empty queue']);
queue.set_stored({name: 'Recon Rover'});
test.assert(lines == ['Unavailable']);
queue.set_stored(undefined);
test.assert(lines == ['Unavailable']);

// Owned cost and current mineral preview are factual, not a guessed turn count.
production.set_stored({minerals: 3}, {name: 'Scout', cost: 10}, 2);
test.assert(lines == ['Stored: 3 / 10', 'Remaining: 7', 'Net/turn: +2'] && production.production_label.text == 'Forecast unavailable');
production.set_stored({minerals: 12}, {name: 'Scout', cost: 10}, 0 - 1);
test.assert(lines == ['Stored: 12 / 10', 'Remaining: 0', 'Net/turn: -1']);
production.set_stored({minerals: 3}, {name: 'Foreign Scout'}, undefined);
test.assert(lines == ['Stored: 3']);

production.set_stored({minerals: 7}, {name: 'Tanks', cost: 40, forecast: 'TURNS:  9'}, 4);
test.assert(lines == ['Stored: 7 / 40', 'Remaining: 33', 'Net/turn: +4'] && production.production_label.text == 'TURNS:  9');
production.set_stored({minerals: 40}, {name: 'Tanks', cost: 40, forecast: 'No Minerals'}, 0);
test.assert(production.production_label.text == 'No Minerals');

production.set_stored({minerals: (0 - 9223372036854775807) - 1}, {name: 'Tanks', cost: 40}, undefined);
test.assert(lines == ['Stored: -9223372036854775808 / 40', 'Remaining: unavailable'] && production.production_label.text == 'Forecast unavailable');

production.set_stored({minerals: 17}, {name: 'Zero cost', cost: 0}, 4);
test.assert(lines == ['Stored: 17'] && production.production_label.text == 'Forecast unavailable');
