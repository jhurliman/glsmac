"""Source-derived presentation/filter helpers, not compatibility authority."""

def choices(slot, slots, admitted_ids):
    admitted = set(admitted_ids)
    other = slots[1 - slot]
    return [24] + [i for i in range(24) if i in admitted and i != other]


def cost_annotation(code, flags, triad, weapon_cost, armor_cost, chassis_speed):
    """English labels stand in for the original localized LABELS tokens."""
    if code == -1:
        quantity = max(0, min(2, weapon_cost // armor_cost))
        return f'{quantity} (Attack/Defense)'
    values = {
        -2: (weapon_cost - 1, 'Attack-1'),
        -3: (armor_cost - 1, 'Defense-1'),
        -4: (chassis_speed - 1, 'Speed-1'),
        -5: (weapon_cost + armor_cost - 2, 'Attack+Defense-2'),
        -6: (weapon_cost + chassis_speed - 2, 'Attack+Speed-2'),
        -7: (armor_cost + chassis_speed - 2, 'Defense+Speed-2'),
    }
    if code in values:
        quantity, formula = values[code]
        return f'{quantity} ({formula})'
    if code < -7:
        return ''
    quantity = code + int(bool(flags & 0x400) and triad == 0)
    return str(quantity) + (f' ({["Land", "Sea", "Air"][triad]})'
                            if flags & 0x400 else '')
