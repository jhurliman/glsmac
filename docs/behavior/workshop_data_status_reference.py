"""Source-derived DATA formatting and console-status selection, terran.exe."""


def movement(speed, triad, weapon_mode, reactor, mask):
    value = speed + (2 * reactor if triad == 2 else 0)
    value -= int(weapon_mode == 7) + int(bool(mask & 0x800000))
    if mask & 0x400:
        value += 2 * reactor if triad == 2 else 1
    if triad == 2 and mask & 0x20:
        value = int(value * 4 / 5)
    if triad == 2 and weapon_mode == 7:
        value = int(value / 2)
    return max(1, min(99, value))


def special(label, slot, ability_names):
    prefix = '{' + label + '} '
    if slot < 0:
        return prefix + '-'
    name = ability_names[slot]
    # These are absolute byte cuts in the original buffer, not word initials.
    text = (prefix + name)[:6]
    _, found, suffix = name.partition(' ')
    return (text + suffix)[:7] if found and suffix else text


def data_rows(*, offense, defense, armor_class, weapon_mode, weapon_cost,
              speed, triad, reactor, mask, slots, ability_names):
    """List of (fixed row index, raw markup); omitted A retains its row gap."""
    first = ('{W:} ' + ('Psi' if offense < 0 else '*' if offense >= 99
                       else str(offense)) if weapon_mode < 3 else
             '{E:} ' + str(weapon_cost))
    rows = [(0, first)]
    if armor_class < 3:
        rows.append((1, '{A:} ' + ('Psi' if defense < 0 else str(defense))))
    rows.extend([(2, '{C:} ' + str(movement(speed, triad, weapon_mode, reactor, mask))),
                 (3, '{R:} ' + str(reactor)),
                 (4, special('SP1:', slots[0], ability_names)),
                 (5, special('SP2:', slots[1], ability_names))])
    return rows


def status(selected, owner, draft, records, unit_counts, production_counts):
    """Returns selected ID and LABELS offset; counts are this owner's caches.

    records[id]: tuple, active, excluded (owner bitmask), role.
    draft tuple order must match records[id]['tuple']; names are irrelevant.
    """
    current = records[selected]
    same = draft == current['tuple']
    if selected < 64:
        return selected, 0xe44 if same else 0xe54
    if not current['active'] or current['excluded'] & (1 << owner):
        return selected, 0xc34
    if same:
        return selected, 0xe50
    accepted = False
    for candidate in range(owner * 64, owner * 64 + 64):
        record = records[candidate]
        if not record['active']:
            return candidate, 0xe54
        if not record['excluded'] & (1 << owner) or unit_counts[candidate]:
            continue
        if production_counts[candidate] and (
                accepted or record['role'] != records[selected]['role']):
            continue
        selected = candidate
        accepted = True
    return selected, 0xe54 if accepted else 0xc38
