"""Existing independently checked component-cost arithmetic."""
def raw_cost(c, abilities=None):
    w0 = int(WE[c['weapon']][4])
    w = w0
    a0 = int(AR[c['armor']][4])
    a = a0
    ch = CH[c['chassis']]
    domain = int(ch[9])
    speed = int(ch[13])
    r = c['reactor']
    penalty = 0
    selected = c['abilities'] if abilities is None else abilities
    for j in sorted(selected):
        if penalty:
            penalty += 1
        cost = c['custom_ability_cost'] if j == 0 and c['custom_ability_cost'] is not None else int(AB[j][1])
        if cost >= 0:
            penalty += cost
        elif cost == -1:
            penalty += min(2, max(0, w // a))
        elif cost == -2:
            penalty += w - 1
        elif cost == -3:
            penalty += a - 1
        elif cost == -4:
            penalty += speed - 1
        elif cost == -5:
            penalty += w + a - 2
        elif cost == -6:
            penalty += w + speed - 2
        elif cost == -7:
            penalty += a + speed - 2
    if domain == 1:
        a //= 2
        speed += r
    elif domain == 2:
        speed += 2 * r
        if a > 1:
            a *= 2 * r
    else:
        penalty += sum((bool(int(AB[j][4], 2) & 1024) for j in selected))
    if int(ch[11]) and int(WE[c['weapon']][2]) >= 99:
        return w0
    w = max(w, a // 2 + 1)
    divisor = 2 << r
    cost = ((speed + a) * w + divisor // 2) // divisor
    if speed == 1:
        cost = cost // 2 + 1
    if w0 > 1 and a0 > 1:
        cost += 1
    if domain == 0 and w0 > 1 and (a0 > 1) and (speed > 1):
        cost += 1
    if domain == 1 and int(WE[c['weapon']][3]) != 11:
        cost = (cost + 1) // 2
    if domain == 2:
        cost //= 2 if int(WE[c['weapon']][3]) > 2 else 4
    cost = max(cost, 2 * r - r // 2)
    if w == a == speed == r == 1:
        cost = 1
    return ((penalty + 4) * cost + 2) // 4
