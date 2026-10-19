const create = #include('_restored_minimap');
const read = create({
    get_year: () => { return 2101; },
    get_turn_status: () => { return 'active'; },
    get_smac_credits: () => { return 0; },
    get_smac_minimap: () => { return {}; },
    get_smac_detail_minimap: () => { return #undefined; },
    on: (name, callback) => {},
});
return read();
