const create = #include('../default/ui/parts/game/preferences/draft');
const pages = [{rows: [{id: 7}, {id: 8}]}, {rows: [{id: 28}]}, {rows: [{id: 43}]},
    {rows: [{id: 42}]}, {rows: [{id: 22}]}, {rows: [{id: 78}]}, {rows: [{id: 57}]}];
let live = {}; let factory = {}; let writes = 0; let fail = false;
for (page of pages) { for (row of page.rows) { const key = #to_string(row.id); live[key] = false; factory[key] = true; } }
const model = create(pages, () => { return live; }, () => { return factory; }, (values) => {
    if (fail) { throw Error('Disk unavailable'); } writes++; live = values;
});
test.assert(!model.set(8,true) && !model.restore() && !model.finish(true));
test.assert(model.open(0) && !model.open(1));
test.assert(model.set(8,true) && !live['8'] && !model.set(53,true) && !model.set(8,1));
test.assert(model.select(3) && model.select(1) && model.select(0) && model.values['8'] && writes == 0);
test.assert(model.key({code: 'ESCAPE', modifiers: {}}) && !model.active && !live['8'] && writes == 0);
test.assert(model.open(0) && !model.values['8']);
test.assert(model.set(8,true) && model.key({code: 'ENTER', modifiers: {}}) && live['8'] && writes == 1);
test.assert(model.open(0) && model.values['8']); model.set(8,false); model.finish(false);
test.assert(live['8'] && writes == 1);
model.open(0); model.set(8,false); model.finish(true); test.assert(!live['8'] && writes == 2);
model.open(0); test.assert(model.restore() && model.values['8'] && !live['8']); model.finish(false);
model.open(0); test.assert(!model.values['8']);
test.assert(model.key({code: 'UP', modifiers: {}}) && model.page == 5);
test.assert(model.key({code: 'DOWN', modifiers: {}}) && model.page == 0);
test.assert(!model.key({code: 'RIGHT', modifiers: {}}) && !model.key({code: 'ENTER', modifiers: {ctrl: true}}));
test.assert(!model.more() && model.select(2) && model.more() && model.page == 6 && model.more() && model.page == 2);
test.assert(!model.select(7) && !model.select(0 - 1));
model.set(8,true); fail = true; let rejected = false;
try { model.finish(true); } catch { : (error) => { rejected = true; } }
test.assert(rejected && model.active && model.values['8'] && !live['8'] && writes == 2);
fail = false; model.finish(true); test.assert(live['8'] && writes == 3);
// Neither the source snapshot nor defaults may alias the modal's draft.
model.open(0); model.restore(); model.set(8,false); test.assert(factory['8'] && live['8']); model.finish(false);
live['8'] = false; model.open(0); test.assert(!model.values['8']); model.finish(false);
let broken = create(pages, () => { return {}; }, () => { return factory; }, (values) => {}); rejected = false;
try { broken.open(0); } catch { : (error) => { rejected = true; } }
test.assert(rejected && !broken.active);
