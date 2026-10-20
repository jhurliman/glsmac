const choose_faction = #include('../default/ui/parts/mainmenu/steps/select_faction');
let title = '';
let clicks = {};
let choices = null;
let portrait = '';
let lines = [];
let preview_removals = 0;
let selected = '';
const root = {
    panel: (style) => {
        return {
            clear: () => {portrait = ''; lines = [];},
            remove: () => {preview_removals++;},
            height: style.height,
            surface: (style) => {return {smacfactionportrait: (filename) => {portrait = filename;}};},
            text: (style) => {
                const item = {text: style.text, remove: () => {}};
                item.measure = () => {return {width: #sizeof(#split(item.text, ' ')) * 40};};
                if (style.text != '') {lines :+style.text;}
                return item;
            },
        };
    },
    window: (data) => {
        title = data.title; clicks = {};
        return {
            remove: () => {}, on: (event, callback) => {}, text: (style) => { return {text: style.text}; },
            panel: (style) => {return {
                choicelist: (style) => {
                    choices = {items: style.items, value: 'RANDOM', handlers: {}, on: (event, callback) => {this.handlers[event] = callback;}};
                    return choices;
                },
                button: (style) => {return {on: (event, callback) => {clicks[style.text] = callback;}};},
            };},
        };
    },
};
const identity = {faction_name: 'Morgan Industries', setup: {}};
const fm = {
    list: () => {return [{id: 'MORGANITES', name: 'Stable name', display_identity: identity}, {id: 'GAIANS', name: 'Gaians'}];},
    import_selection_preview: (filename, section) => {
        return {descriptor: 'Role ' + section, quotation: ['Quote ' + filename, '', 'Attribution'], details: ['Details ' + filename]};
    },
};
const glsmac = {game: {get_ruleset: () => {return 'smac';}, get_fm: () => {return fm;}}, reset: () => {throw WrongBack('Lost ancestor');}};
const popup = #include('../default/ui/parts/mainmenu/popup')({root: root, glsmac: glsmac});
const i = {glsmac: glsmac, popup: popup, steps: {select_identity: (context, faction) => {
    selected = faction.id; popup.show({title: 'Name', width: 440, height: 100});
}}};
popup.show({title: 'Rules', width: 440, height: 200});
choose_faction(i);
test.assert(title == 'CHOOSE A FACTION' && portrait == 'morgan.pcx');
test.assert((choices.items[0])[1] == 'Morgan Industries');
test.assert((choices.items[0])[2].righttext == '(Role MORGAN)');
choices.value = 'MORGANITES';
test.assert(choices.handlers.select({value: 'MORGANITES'}) == true);
test.assert(portrait == 'morgan.pcx' && lines[0] == 'Quote morgan.txt' && lines[1] == 'Attribution');
test.assert(clicks['Faction Details']() == true && lines[0] == 'Details morgan.txt');
test.assert(clicks.OK() == true && selected == 'MORGANITES' && title == 'Name' && preview_removals == 1);
popup.back();
test.assert(title == 'CHOOSE A FACTION' && choices.value == 'MORGANITES' && portrait == 'morgan.pcx' && lines[0] == 'Details morgan.txt');
test.assert(clicks.Cancel() == true && title == 'Rules' && preview_removals == 2);
popup.hide();
