return (i, faction) => {
    let draft = #clone(faction.display_identity);
    // DOM properties are references in GSE. Materialize text before replacing
    // a popup, so the draft never points into an input that has been removed.
    const start = (status) => {
        let accepted = false;
        try { i.glsmac.start_game(faction.id, draft); accepted = true; }
        catch { : (error) => { status.text = error.reason; } }
        if (accepted) { i.popup.dismiss(); }
    };
    const customize = () => {
        let fields = [];
        let status = null;
        const cancel = () => { i.popup.dismiss(); i.popup.back(); return true; };
        const accept = () => {
            draft.leader_name = '' + fields[0].value; draft.faction_name = '' + fields[1].value;
            draft.setup.noun = '' + fields[2].value; draft.setup.adjective = '' + fields[3].value;
            draft.leader_title = '' + fields[4].value; draft.setup.description = '' + fields[5].value;
            start(status); return true;
        };
        i.popup.show({
            title: 'Customize Your Faction', width: 600, height: 320,
            on_cancel: cancel,
            generator: (body) => {
                const rows = [
                    ['Name:', draft.leader_name], ['Formal:', draft.faction_name],
                    ['Noun:', draft.setup.noun], ['Adjective:', draft.setup.adjective],
                    ['Title:', draft.leader_title], ['Description:', draft.setup.description],
                ];
                const limits = [23, 39, 23, 23, 23, 23];
                let top = 12;
                let index = 0;
                for (row of rows) {
                    body.text({class: 'popup-text', left: 12, top: top, text: row[0]});
                    const field = body.input({class: 'popup-input', left: 140, right: 12, top: top, max_bytes: limits[index], value: '' + row[1]});
                    field.on('input', accept);
                    fields :+field; top += 30; index++;
                }
                body.text({class: 'popup-text', left: 12, top: top, text: 'Gender: ' + (draft.setup.female ? 'Female' : 'Male')});
                status = body.text({class: 'popup-text', left: 12, right: 12, top: top + 30, text: ''});
            },
            buttons: [
                {style: {text: 'Restore Defaults', align: 'left', width: 190}, onclick: () => {
                    try {
                        let source = null;
                        for (entry of #include('../../../../factions').get_roster(i.glsmac.game.get_ruleset())) {
                            if (#uppercase(entry[0]) == faction.id) { source = '' + entry[1]; }
                        }
                        if (source == null) { throw UnknownFaction('Faction definition source is unavailable'); }
                        const restored = i.glsmac.restore_singleplayer_identity(faction.id, source + '.txt', #uppercase(source));
                        draft = #clone(restored.display_identity);
                        const values = [draft.leader_name, draft.faction_name, draft.setup.noun, draft.setup.adjective, draft.leader_title, draft.setup.description];
                        for (let index = 0; index < #sizeof(fields); index++) { fields[index].value = '' + values[index]; }
                        status.text = '';
                    } catch { : (error) => { status.text = error.reason; } }
                    return true;
                }},
                {style: {text: 'OK', align: 'center', width: 190, is_ok: true}, onclick: accept},
                {style: {text: 'Cancel', align: 'right', width: 190, is_cancel: true}, onclick: cancel},
            ],
        });
    };
    let input = null;
    let status = null;
    const commit_name = () => {
        let accepted = false;
        try {
            draft.leader_name = '' + input.value;
            i.glsmac.set_singleplayer_leader(faction.id, draft.leader_name);
            accepted = true;
        } catch { : (error) => { status.text = error.reason; } }
        return accepted;
    };
    const accept = () => {
        if (commit_name()) {
            let accepted = false;
            try { i.glsmac.start_game(faction.id); accepted = true; }
            catch { : (error) => { status.text = error.reason; } }
            if (accepted) { i.popup.dismiss(); }
        }
        return true;
    };
    const original_name = i.glsmac.game.get_ruleset() == 'smac';
    let name_dialog = {
        title: 'ENTER YOUR NAME', width: 600, height: 150,
        generator: (body) => {
            body.text({class: 'popup-text', left: original_name ? 6 : 12, top: original_name ? 0 : 12, text: 'Name (' + (draft.setup.female ? 'Female' : 'Male') + '):'});
            let input_style = {class: 'popup-input', left: 140, right: 12, top: 12, max_bytes: 23, value: draft.leader_name};
            if (original_name) { input_style = {class: 'smac-original-input', left: draft.setup.female ? 119 : 101,
                top: 1, width: draft.setup.female ? 544 : 562, height: 20, max_bytes: 23, value: draft.leader_name}; }
            input = body.input(input_style);
            input.on('input', accept);
            status = body.text({class: 'popup-text', left: 12, right: 12, top: original_name ? 27 : 48, text: ''});
        },
        buttons: [
            {style: {text: 'Customize', align: 'left', width: 190}, onclick: () => { if (commit_name()) { customize(); } return true; }},
            {style: {text: 'OK', align: 'center', width: 190, is_ok: true}, onclick: accept},
            {style: {text: 'Cancel', align: 'right', width: 190, is_cancel: true}, onclick: () => { i.popup.back(); return true; }},
        ],
    };
    if (original_name) { name_dialog.smac_kind = 'name'; }
    i.popup.show(name_dialog);
};
