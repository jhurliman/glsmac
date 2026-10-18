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
const presentation = {name: 'Recon Rover', queue: ['Former', 'Colony Pod', 'HQ', 'Tanks', 'Bank', 'Node', 'Scout', 'Scout', 'Scout']};
production.set_stored({minerals: 17}, presentation);
test.assert(production.production_title.text == 'Recon Rover');
test.assert(lines == ['Stored: 17']);
queue.set_stored(presentation);
test.assert(lines == presentation.queue);
queue.set_stored({name: 'Recon Rover', queue: []});
test.assert(lines == ['Empty queue']);
queue.set_stored({name: 'Recon Rover'});
test.assert(lines == ['Unavailable']);
queue.set_stored(undefined);
test.assert(lines == ['Unavailable']);
