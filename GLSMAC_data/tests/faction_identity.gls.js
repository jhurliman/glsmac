const select_identity = #include('../default/ui/parts/mainmenu/steps/select_identity');
let shown = null;
let inputs = [];
let texts = [];
let submitted = null;
let dismissed = 0;
let backs = 0;
let reject = false;
const original = {faction_name: 'Morgan Industries', leader_title: 'CEO', leader_name: 'Morgan', setup: {noun: 'Morganites', adjective: 'Morganic', description: 'shrewd', female: false}};
const i = {
    popup: {
        show: (data) => {
            shown = data; inputs = []; texts = [];
            data.generator({
                text: (style) => { const result = {text: style.text}; texts :+result; return result; },
                input: (style) => { const result = {value: style.value, handlers: {}, on: (event, callback) => { this.handlers[event] = callback; }}; inputs :+result; return result; },
            });
        },
        dismiss: () => { dismissed++; },
        back: () => { backs++; },
    },
    glsmac: {start_game: (id, identity) => {
        if (reject) { throw InvalidIdentity('Rejected identity'); }
        submitted = {id: id, identity: #clone(identity)};
    }},
};
const click = (label) => {
    let found = false;
    for (button of shown.buttons) {
        if (button.style.text == label) { test.assert(button.onclick() == true); found = true; break; }
    }
    test.assert(found);
};
select_identity(i, {id: 'MORGANITES', display_identity: original});
test.assert(shown.title == 'ENTER YOUR NAME' && inputs[0].value == 'Morgan');
inputs[0].value = 'Ada'; test.assert(inputs[0].handlers.input({}) == true);
test.assert(submitted.id == 'MORGANITES' && submitted.identity.leader_name == 'Ada' && dismissed == 1);
test.assert(original.leader_name == 'Morgan');
select_identity(i, {id: 'MORGANITES', display_identity: original});
inputs[0].value = 'Edited leader'; click('Customize');
test.assert(shown.title == 'Customize Your Faction' && #sizeof(inputs) == 6 && inputs[0].value == 'Edited leader');
inputs[1].value = 'The Builders'; inputs[2].value = 'Builders'; inputs[3].value = 'Builder'; inputs[4].value = 'Director'; inputs[5].value = 'careful';
test.assert(inputs[3].handlers.input({}) == true);
test.assert(submitted.identity.faction_name == 'The Builders' && submitted.identity.setup.noun == 'Builders' && submitted.identity.setup.adjective == 'Builder' && submitted.identity.leader_title == 'Director' && submitted.identity.setup.description == 'careful');
test.assert(!submitted.identity.setup.female && original.setup.description == 'shrewd');
select_identity(i, {id: 'MORGANITES', display_identity: original});
inputs[0].value = 'Previously accepted'; click('Customize'); click('Restore Defaults');
test.assert(inputs[0].value == 'Morgan' && inputs[1].value == 'Morgan Industries');
reject = true; click('OK');
test.assert(texts[#sizeof(texts) - 1].text == 'Rejected identity' && dismissed == 2);
click('Cancel'); test.assert(backs == 1);
// Exercise the actual faction step's handoff into identity setup.
let faction_popup = null;
let choice = null;
let handoff = null;
const faction = {id: 'MORGANITES', name: 'Stable catalog label', display_identity: original};
#include('../default/ui/parts/mainmenu/steps/select_faction')({
    glsmac: {game: {get_fm: () => {return {list: () => {return [faction];}};}}, start_game: () => {throw WrongPath('Identity was skipped');}},
    steps: {select_identity: (context, selected) => {handoff = selected;}},
    popup: {show: (data) => {
        faction_popup = data;
        data.generator({choicelist: (style) => {choice = {items: style.items, value: 'RANDOM'};return choice;}});
    }},
});
test.assert((choice.items[1])[1] == 'Morgan Industries');choice.value = 'MORGANITES';
test.assert((faction_popup.buttons[0]).onclick() == true && handoff.id == faction.id);
faction_popup.generator({choicelist: (style) => {choice = {items: style.items, value: 'RANDOM'};return choice;}});
test.assert(choice.value == 'MORGANITES', 'Back lost the accepted faction selection');
