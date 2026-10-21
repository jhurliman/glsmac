"""Stored-design Upgrade prompt text; resolved stock rule inputs, no UI."""
from workshop_main_presentation_reference import stat_line

ABILITY_TOKENS = (
    'Super', '', 'Cloaked', 'Amphibious', 'Drop', 'SAM', 'Sub', 'Carrier',
    'AAA', 'ECM', 'Grav', 'Empath', 'Secure', 'Fungicidal', 'Trained',
    'Artillery', 'Clean', 'Blink', 'Trance', 'Heavy', 'X', 'Repair',
    'Police', 'Slow',
)


def stored_stats(*, psi='Psi', sea='Sea', air='Air', **rules):
    """5935c0 differs from main5932d0 in PSI/domain text."""
    result = stat_line(**rules).replace('?', psi)
    suffix = f"*{rules['reactor']}" if rules['reactor'] > 1 else ''
    if suffix:
        result = result[:-len(suffix)]
    if rules['triad'] == 1:
        result += ' ' + sea
    elif rules['triad'] == 2:
        result += ' ' + air
    return result + suffix


def upgrade_design_text(name, stats, mask, ability_tokens=ABILITY_TOKENS):
    """504fe0; names/stats come from the stored source/target records."""
    result = name + ' (' + stats
    if mask & 0xffffffff:
        result += ' ' + ''.join(ability_tokens[j] for j in range(24)
                                if mask & (1 << j))
    return result + ')'
