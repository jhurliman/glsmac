// Real menu/popup modules with file APIs and DOM replaced. Native file atomicity
// and claimed restore are tested separately; this checks actual UI callbacks.
const save = #include('../default/ui/parts/game/popup/save_campaign');
const load = #include('../default/ui/parts/mainmenu/steps/load_campaign');
const main = #include('../default/ui/parts/mainmenu/steps/main');
const menu = #include('../default/ui/parts/game/menu/left_menu_game');

let save_calls = [];
let save_complete = null;
let closed = 0;
let save_input = null;
let save_status = null;
let save_buttons = {};
const save_body = {
	text: (properties) => { if (properties.text == '') { save_status = properties; } return properties; },
	input: (properties) => { save_input = properties; return properties; },
	button: (properties) => { const label = properties.text; return {on: (name, callback) => {save_buttons[label] = callback;}}; },
};
save.init({
	game: {
        on: (name, callback) => { test.assert(name == 'campaign_saved'); save_complete = callback; },
        save_campaign: (path) => { save_calls :+path; return #sizeof(save_calls); },
    },
	create: (title, width, height, generator) => {
		test.assert(title == 'SAVE GLSMAC CAMPAIGN');
		generator(save_body, (result) => { closed++; });
		return {};
	},
});
let opened = 0;
const menu_entries = menu.init({create: (entries) => {return entries;}, save_campaign: () => {opened++;save.on_show();}});
let save_entry = null;
for (entry of menu_entries) { if (entry.label == 'Save Campaign') { save_entry = entry; } }
test.assert(save_entry != null); save_entry.open();
test.assert(opened == 1 && save_input.value == 'campaign.glsmac');
save_input.value = 'chosen.glsmac';
save_buttons.Save({});
test.assert(#sizeof(save_calls) == 1 && save_calls[0] == 'chosen.glsmac' && closed == 0);
test.assert(save_status.text == 'Saving campaign...' && save_input.value == 'chosen.glsmac');
save_buttons.Save({});
test.assert(#sizeof(save_calls) == 1); // no duplicate while queued
save_complete({request_id: 99, path: '/stale.glsmac', error: ''});
test.assert(save_status.text == 'Saving campaign...');
save_complete({request_id: 1, path: '', error: 'Save rejected without replacing the campaign file'});
test.assert(save_status.text == 'Save rejected without replacing the campaign file' && save_input.value == 'chosen.glsmac');
save_buttons.Save({});
test.assert(#sizeof(save_calls) == 2 && closed == 0 && save_input.value == 'chosen.glsmac');
save_buttons.Close({}); // closing the panel does not falsely report success or cancel the write
save.on_show();
test.assert(closed == 1 && save_status.text == 'Saving campaign...');
save_complete({request_id: 2, path: '/campaigns/chosen.glsmac', error: ''});
test.assert(save_input.value == '/campaigns/chosen.glsmac');
test.assert(save_status.text == 'Campaign saved. The full path is shown above.');
save_complete({request_id: 1, path: '/stale.glsmac', error: ''});
test.assert(save_input.value == '/campaigns/chosen.glsmac');
save_buttons.Close({});
test.assert(closed == 2 && #sizeof(save_calls) == 2);
save.on_show();
test.assert(save_status.text == '' && save_input.value == '/campaigns/chosen.glsmac');

let load_calls = [];
let fail_load = true;
let dismissed = 0;
let returned = 0;
let deinitialized = 0;
let load_input = null;
let load_status = null;
let load_dialog = null;
let main_entries = [];
const load_body = {
	text: (properties) => { if (properties.text == '') {load_status = properties;} return properties; },
	input: (properties) => {load_input = properties;return properties;},
};
const context = {
	glsmac: {
		deinit: () => {deinitialized++;},
		load_campaign: (path) => {
			load_calls :+path;
			if (fail_load) {throw GameError('Invalid GLSMAC campaign file');}
		},
	},
	sliding: {show: (data) => {main_entries = data.entries;}},
	steps: {load_campaign: load},
	popup: {
		show: (data) => {load_dialog = data;data.generator(load_body);},
		dismiss: () => {dismissed++;},
        back: () => {returned++;},
	},
};
main(context);
for (entry of main_entries) {if (entry[0] == 'Load Game') {entry[1]();}}
test.assert(deinitialized == 1 && load_dialog.title == 'Load GLSMAC Campaign' && load_input.value == 'campaign.glsmac');
load_input.value = 'folder/chosen.glsmac';
load_dialog.buttons[0].onclick();
test.assert(#sizeof(load_calls) == 1 && load_calls[0] == 'folder/chosen.glsmac' && dismissed == 0 && deinitialized == 1);
test.assert(load_status.text == 'Invalid GLSMAC campaign file' && load_input.value == 'folder/chosen.glsmac');
load_dialog.buttons[1].onclick();
test.assert(returned == 1 && dismissed == 0 && #sizeof(load_calls) == 1);
load(context);
fail_load = false;
load_dialog.buttons[0].onclick();
test.assert(returned == 1 && dismissed == 1 && #sizeof(load_calls) == 2 && deinitialized == 1);
