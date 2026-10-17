const mocks = #include('./_base_population_mocks');

// Population mutations are host-authoritative.
for (handler of mocks.events) {
	test.assert(#is_defined(handler.validate({caller: 1})));
	test.assert(!#is_defined(handler.validate({caller: 0})));
}

// Undoing worker growth releases both sides of the work assignment exactly once.
{
	const f = mocks.create_fixture();
	f.seed_pop('WORKER', f.tiles[0]);
	f.base.set_accumulated_nutrients(31);
	const event = f.game.event('add_base_pop', {
		base: f.base, type: 'WORKER', worked_tile: f.tiles[1],
	});
	test.assert(f.base.get_size() == 2);
	test.assert(f.base.get_accumulated_nutrients() == 11);
	test.assert(f.tiles[1].get_working_pop() == event.applied.pop);
	test.assert(event.applied.pop.get_worked_tile() == f.tiles[1]);
	f.rollback(event);
	test.assert(f.base.get_size() == 1);
	test.assert(f.base.get_accumulated_nutrients() == 31);
	test.assert(#sizeof(f.base.get_worked_tiles()) == 1);
	test.assert(!#is_defined(f.tiles[1].get_working_pop()));
	// A subsequent growth can claim the same tile again.
	f.game.event('add_base_pop', {base: f.base, type: 'WORKER', worked_tile: f.tiles[1]});
	test.assert(f.base.get_size() == 2);
	test.assert(#sizeof(f.base.get_worked_tiles()) == 2);
}

// Specialist rollback also restores the initial zero nutrient counter.
{
	const f = mocks.create_fixture();
	const event = f.game.event('add_base_pop', {base: f.base, type: 'DOCTOR'});
	test.assert(f.base.get_size() == 1);
	f.rollback(event);
	test.assert(f.base.get_size() == 0);
	test.assert(f.base.get_accumulated_nutrients() == 0);
	test.assert(#sizeof(f.base.get_worked_tiles()) == 0);
}

// Removing a worker clears the tile before destruction; undo restores its work.
{
	const f = mocks.create_fixture();
	const pop = f.seed_pop('WORKER', f.tiles[0]);
	f.base.set_accumulated_nutrients(7);
	const event = f.game.event('remove_base_pop', {base: f.base, pop: pop});
	test.assert(f.base.get_size() == 0);
	test.assert(f.base.get_accumulated_nutrients() == 0);
	test.assert(#sizeof(f.base.get_worked_tiles()) == 0);
	test.assert(!#is_defined(f.tiles[0].get_working_pop()));
	f.rollback(event);
	const restored = (f.base.get_pops())[0];
	test.assert(restored.get_type() == 'WORKER');
	test.assert(restored.get_worked_tile() == f.tiles[0]);
	test.assert(f.tiles[0].get_working_pop() == restored);
	test.assert(#sizeof(f.base.get_worked_tiles()) == 1);
	test.assert(f.base.get_accumulated_nutrients() == 7);
}

// Specialist removal/undo must not disturb an existing worker assignment.
{
	const f = mocks.create_fixture();
	const worker = f.seed_pop('WORKER', f.tiles[0]);
	const doctor = f.seed_pop('DOCTOR');
	f.base.set_accumulated_nutrients(5);
	const event = f.game.event('remove_base_pop', {base: f.base, pop: doctor});
	test.assert(f.base.get_size() == 1);
	test.assert(f.tiles[0].get_working_pop() == worker);
	f.rollback(event);
	test.assert(f.base.get_size() == 2);
	const restored = (f.base.get_pops())[1];
	test.assert(restored.get_type() == 'DOCTOR');
	test.assert(!#is_defined(restored.get_worked_tile()));
	test.assert(f.base.get_accumulated_nutrients() == 5);
}

// Exercise the actual turn/growth callback, yield aggregation and tile selection.
{
	const f = mocks.create_fixture();
	f.seed_pop('WORKER', f.tiles[0]);
	f.base.set_accumulated_nutrients(18);
	f.turn(); // +2 food reaches the 20-food threshold at size 1.
	test.assert(f.base.get_size() == 2);
	test.assert(f.base.get_accumulated_nutrients() == 0);
	test.assert(#is_defined(f.tiles[1].get_working_pop())); // best available tile
	const event = (f.get_events())[0];
	test.assert(event.name == 'add_base_pop');
	f.rollback(event);
	test.assert(f.base.get_size() == 1);
	test.assert(!#is_defined(f.tiles[1].get_working_pop()));
	test.assert(f.base.get_accumulated_nutrients() == 20); // event-entry balance
}

// Starvation prefers a specialist, preserving productive workers.
{
	const f = mocks.create_fixture();
	const worker = f.seed_pop('WORKER', f.tiles[2]);
	f.seed_pop('DOCTOR');
	f.base.set_accumulated_nutrients(0);
	f.turn(); // 2 intake - 4 consumption
	test.assert(f.base.get_size() == 1);
	test.assert((f.base.get_pops())[0] == worker);
	test.assert(f.tiles[2].get_working_pop() == worker);
	test.assert((f.get_events())[0].name == 'remove_base_pop');
}

// With workers only, starvation releases the least productive tile for reuse.
{
	const f = mocks.create_fixture();
	f.center.set('nutrients', 1);
	const worker = f.seed_pop('WORKER', f.tiles[0]);
	f.seed_pop('WORKER', f.tiles[2]);
	f.base.set_accumulated_nutrients(0);
	f.turn(); // 3 intake - 4 consumption
	test.assert(f.base.get_size() == 1);
	test.assert((f.base.get_pops())[0] == worker);
	test.assert(!#is_defined(f.tiles[2].get_working_pop()));
	test.assert(#sizeof(f.base.get_worked_tiles()) == 1);
	f.rollback((f.get_events())[0]);
	test.assert(f.base.get_size() == 2);
	test.assert(#is_defined(f.tiles[2].get_working_pop()));
	test.assert(#sizeof(f.base.get_worked_tiles()) == 2);
	test.assert(f.base.get_accumulated_nutrients() == 0);
}
