"""Original SMAC Workshop scalar text contracts; caller supplies localized PSI.

Arguments are signed int8 record values unless explicitly named unsigned.
The normal mode dispatch must happen before combat_strength is used.
"""
def armor_strength(strength, psi='Psi'):
    assert -128 <= strength <= 127
    return psi if strength < 0 else str(strength)


def combat_strength(strength, psi='Psi',high_marker='*'):
    assert -128 <= strength <= 127
    return psi if strength < 0 else high_marker if strength >= 99 else str(strength)


def weapon_picker(strength, mode, secondary_unsigned, psi='Psi'):
    assert -128 <= strength <= 127
    assert 0 <= secondary_unsigned <= 255
    return psi if strength == -1 else str(secondary_unsigned if mode >= 3 else strength & 255)

