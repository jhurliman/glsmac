"""Source-derived Workshop display rules; inputs are resolved rules/state."""

def stat_line(*, offense, defense, speed, triad, special_chassis,
              weapon_mode, reactor, mask):
    if offense < 0:
        attack = '?'
    elif offense >= 99:
        attack = '*'
    else:
        attack = str(offense * 3 // 2 if special_chassis else offense)
        for bit, symbol in [(0x10, '^'), (8, '~'), (0x100000, 'x'),
                            (0x800, 'e'), (0x20000, '!')]:
            if mask & bit:
                attack += symbol
        if offense > 0 and defense >= 0 and (triad == 1 or
                (triad == 0 and mask & 0x8000)):
            attack = '(' + attack + ')'
        if mask & 0x20:
            attack = '<' + attack + '>'
    if defense < 0:
        armor = '?'
    else:
        armor = str(max(1, min(9999, defense)))
        if mask & 0x200:
            armor += '+'
        if mask & 0x40000:
            armor += 't'
        if mask & 0x100:
            armor = '<' + armor + '>'
    movement = speed + (2 * reactor if triad == 2 else 0)
    movement -= int(weapon_mode == 7) + int(bool(mask & 0x800000))
    if mask & 0x400:
        movement += 2 * reactor if triad == 2 else 1
    if triad == 2 and mask & 0x20:
        movement = int(movement * 4 / 5)
    if triad == 2 and weapon_mode == 7:
        movement = int(movement / 2)
    movement = max(1, min(99, movement))
    return f'{attack}-{armor}-{movement}' + (f'*{reactor}' if reactor > 1 else '')


def human_mineral_factor(world, industry, rule_multiplier=10):
    factor = rule_multiplier
    if world == 0:
        factor = factor * 8 // 10
    elif world == 1:
        factor = factor * 9 // 10
    numerator, denominator = ((13, 10) if -7 <= industry <= -3 else
        (6, 5) if industry == -2 else (11, 10) if industry == -1 else
        (1, 1) if industry == 0 else (9, 10) if industry == 1 else
        (4, 5) if industry == 2 else (7, 10) if industry == 3 else
        (3, 5) if industry == 4 else (1, 2))
    return (factor * numerator + denominator - 1) // denominator


def cost_display(raw_rows, reactor, cached_factor):
    """Returns quantity and minimum branch; localized caption is caller-owned."""
    return max(raw_rows, reactor) * cached_factor, raw_rows < reactor


def stat_rectangle(preview, font_first, font_ascent, font_internal_leading,
                   font_height_with_external_leading):
    height = (font_first + font_ascent - font_internal_leading
              if font_first >= 0 else font_height_with_external_leading)
    left, _, right, bottom = preview
    return left, bottom - 4 * height, right, bottom - 3 * height
