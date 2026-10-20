const select_identity = #include('../default/ui/parts/mainmenu/steps/select_identity');
let shown = null;
let inputs = [];
let texts = [];
let submitted = null;
let dismissed = 0;
let backs = 0;
let reject = false;
const original = {faction_name: 'Morgan Industries', leader_title: 'CEO', leader_name: 'Morgan', setup: {noun: 'Morganites', adjective: 'Morganic', description: 'shrewd', female: false}};
let live = #clone(original);
const i = {
    popup: {
        show: (data) => {
            shown = data; inputs = []; texts = [];
            data.generator({
                text: (style) => { const result = {text: style.text}; texts :+result; return result; },
                input: (style) => { const result = {value: style.value, max_bytes: style.max_bytes, handlers: {}, on: (event, callback) => { this.handlers[event] = callback; }}; inputs :+result; return result; },
            });
        },
        dismiss: () => { dismissed++; },
        back: () => { backs++; },
    },
    glsmac: {
        game: {get_ruleset: () => { return 'smac'; }},
        set_singleplayer_leader: (id, name) => {
            if (reject) { throw InvalidIdentity('Rejected identity'); }
            test.assert(id == 'MORGANITES'); live.leader_name = '' + name;
        },
        restore_singleplayer_identity: (id, filename, section) => {
            test.assert(id == 'MORGANITES' && filename == 'morgan.txt' && section == 'MORGAN');
            live = #clone(original); return {display_identity: #clone(live)};
        },
        start_game: (id, identity) => {
            if (reject) { throw InvalidIdentity('Rejected identity'); }
            submitted = {id: id, identity: #is_defined(identity) ? #clone(identity) : #clone(live)};
        },
    },
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
test.assert(inputs[0].max_bytes == 23);
inputs[0].value = 'Ada'; test.assert(inputs[0].handlers.input({}) == true);
test.assert(submitted.id == 'MORGANITES' && submitted.identity.leader_name == 'Ada' && dismissed == 1);
test.assert(original.leader_name == 'Morgan');
select_identity(i, {id: 'MORGANITES', display_identity: original});
inputs[0].value = 'Edited leader'; click('Customize');
test.assert(shown.title == 'Customize Your Faction' && #sizeof(inputs) == 6 && inputs[0].value == 'Edited leader');
for (let field = 0; field < #sizeof(inputs); field++) {test.assert(inputs[field].max_bytes == (field == 1 ? 39 : 23));}
test.assert(live.leader_name == 'Edited leader', 'Name was not committed before optional customization');
inputs[1].value = 'The Builders'; inputs[2].value = 'Builders'; inputs[3].value = 'Builder'; inputs[4].value = 'Director'; inputs[5].value = 'careful';
test.assert(inputs[3].handlers.input({}) == true);
test.assert(submitted.identity.faction_name == 'The Builders' && submitted.identity.setup.noun == 'Builders' && submitted.identity.setup.adjective == 'Builder' && submitted.identity.leader_title == 'Director' && submitted.identity.setup.description == 'careful');
test.assert(!submitted.identity.setup.female && original.setup.description == 'shrewd');
select_identity(i, {id: 'MORGANITES', display_identity: original});
inputs[0].value = 'Previously accepted'; click('Customize'); click('Restore Defaults');
test.assert(inputs[0].value == 'Morgan' && inputs[1].value == 'Morgan Industries' && live.leader_name == 'Morgan');
reject = true; click('OK');
test.assert(texts[#sizeof(texts) - 1].text == 'Rejected identity' && dismissed == 2);
click('Cancel'); test.assert(backs == 1 && dismissed == 3 && live.leader_name == 'Morgan');
reject = false;
select_identity(i, {id: 'MORGANITES', display_identity: live});
inputs[0].value = 'Retained'; click('Customize');
inputs[0].value = 'Discard this draft';
test.assert(shown.on_cancel() == true && backs == 2 && dismissed == 4 && live.leader_name == 'Retained');
select_identity(i, {id: 'MORGANITES', display_identity: live});
test.assert(inputs[0].value == 'Retained', 'Cancelled customization lost the accepted leader name');
reject = true; inputs[0].value = 'Rejected'; click('Customize');
test.assert(shown.title == 'ENTER YOUR NAME' && live.leader_name == 'Retained' && texts[#sizeof(texts) - 1].text == 'Rejected identity');
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

// The real popup stack must return directly from Customize to Faction for
// both the Cancel button and Escape, without rendering Name again.
let title = '';
let clicks = {};
let keydown = null;
let controls = [];
const actual_popup = #include('../default/ui/parts/mainmenu/popup')({
    glsmac: {reset: () => {throw WrongBack('Cancelled past faction selection');}},
    root: {window: (data) => {
        title = data.title; clicks = {}; controls = [];
        return {
            remove: () => {}, on: (event, callback) => {keydown = callback;}, text: (style) => {return {text: style.text};},
            panel: (style) => {return {
                text: (style) => {return {text: style.text};},
                input: (style) => {const field = {value: style.value, on: (event, callback) => {}}; controls :+field; return field;},
                button: (style) => {return {on: (event, callback) => {clicks[style.text] = callback;}};},
            };},
        };
    }},
});
reject = false;
for (use_escape of [false, true]) {
    actual_popup.show({title: 'Faction', width: 480, height: 250});
    select_identity({popup: actual_popup, glsmac: i.glsmac}, {id: 'MORGANITES', display_identity: live});
    controls[0].value = 'Keep this'; clicks.Customize();
    test.assert(title == 'Customize Your Faction' && live.leader_name == 'Keep this');
    controls[0].value = 'Unaccepted draft';
    if (use_escape) {test.assert(keydown({code: 'ESCAPE', modifiers: []}) == true);}
    else {test.assert(clicks.Cancel() == true);}
    test.assert(title == 'Faction' && live.leader_name == 'Keep this', 'Custom cancellation lost its source mutation boundary');
    actual_popup.hide();
}
