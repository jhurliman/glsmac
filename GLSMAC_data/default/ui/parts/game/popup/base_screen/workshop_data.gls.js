// 441087..441b88. The two DATA colors are equal, so brace spans can be
// flattened after the original absolute-byte SP truncation has occurred.
const plain = (raw) => {
    let result = '';
    for (group of #split(raw, '{')) { for (part of #split(group, '}')) { result += part; } }
    return result;
};
const special = (label, slot, names) => {
    const prefix = '{' + label + '} ';
    if (slot < 0) { return prefix + '-'; }
    const name = names[slot]; let result = (prefix + name)[0::6]; let index = 0;
    while (true) {
        const byte = ('' + name)[index::(index + 1)];
        if (byte == '') { break; }
        if (byte == ' ') {
            const suffix = ('' + name)[(index + 1)::(index + 2)];
            result = (result + suffix)[0::7]; break;
        }
        index++;
    }
    return result;
};
return (labels, psi, data, armor, slots, reactor) => {
    let rows = ['', '', '', '', '', ''];
    const equipment = data.weapon_mode >= 3;
    const strength = data.offense < 0 ? psi : (data.offense >= 99 ? '*' : #to_string(data.offense));
    rows[0] = labels[equipment ? 14 : 8] + ' ' + (equipment ? #to_string(data.weapon_cost) : strength);
    if (armor.armor_class < 3) { rows[1] = labels[9] + ' ' + (armor.rating < 0 ? psi : #to_string(armor.rating)); }
    rows[2] = labels[10] + ' ' + #to_string(data.movement);
    rows[3] = labels[11] + ' ' + #to_string(reactor);
    rows[4] = plain(special(labels[12], slots[0], data.ability_names));
    rows[5] = plain(special(labels[13], slots[1], data.ability_names));
    return rows;
};
