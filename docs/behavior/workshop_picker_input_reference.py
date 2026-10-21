"""Source-derived43ca60/43c880 state transitions; painter/close are boundaries.

State: count, selected, first, position, availability. availability_by_ordinal
contains original painter validity, not a keyboard navigation filter.
"""


def trunc_div(n, d):
    return (abs(n)//d) * (-1 if n < 0 else 1)


def paint(state, ordinal, selected, availability_by_ordinal, events):
    events.append(['paint', ordinal, selected])
    if not state['first'] <= ordinal < state['first']+20:
        return 1
    if ordinal >= state['count']:
        return 1
    return int(availability_by_ordinal[ordinal])


def keyboard(state, key, availability_by_ordinal):
    q = dict(state)
    events = []
    old_s, old_p = q['selected'], q['first']
    n, s, p = q['count'], old_s, old_p

    def position(value):
        q['position'] = max(0, min(value, n//5-4+2))
        events.append(['scroll_position', q['position']])

    if key == 27:
        q['selected'] = -1
        events.append(['close'])
    elif key == 13:
        events.append(['close'])
    elif key in (38, 104):
        if s < 5:
            return q, events
        s -= 5
        while s >= p+20:
            s -= 5
        q['selected'] = s
        if s < p:
            q['first'] = p-5
            position(q['position']-1)
    elif key in (40, 98):
        if s > n-5-1:
            if n <= 20:
                return q, events
            position(q['position']+1)
            q['first'] = 5*q['position']
        else:
            s += 5
            while s < p:
                s += 5
            q['selected'] = s
            if s >= p+20:
                q['first'] = p+5
                position(q['position']+1)
    elif key in (37, 100, 39, 102):
        lo = 5*trunc_div(s, 5)
        hi = min(lo+4, n-1)
        candidate = s + (-1 if key in (37, 100) else 1)
        q['selected'] = lo if candidate < lo or hi < lo else min(candidate, hi)
    elif key == 36:
        q['first'] = q['selected'] = 0
    elif key == 35:
        q['selected'] = n-1
        if n > 20:
            q['first'] = 5*(trunc_div(n-1,5)+1)-20
    elif key in (33, 34):
        if n <= 20:
            return q, events
        position(q['position'] + (-3 if key == 33 else 3))
        q['first'] = 5*q['position']

    if old_p != q['first']:
        events.append(['full_repaint'])  #43ca10 paints but does not assign availability
    elif old_s != q['selected']:
        paint(q, old_s, False, availability_by_ordinal, events)
        q['availability'] = paint(q, q['selected'], True, availability_by_ordinal, events)
    events.append(['invalidate'])
    return q, events


def hover(state, x, y, availability_by_ordinal):
    q = dict(state)
    events = []
    s = q['selected']
    if 4 <= x < 544 and 4 <= y < 380:
        candidate = q['first'] + ((y-4)//94)*5 + (x-4)//108
        if candidate == s:
            return q, events
        if candidate < q['count']:
            q['availability'] = paint(q,candidate,True,availability_by_ordinal,events)
            if s >= 0:
                paint(q,s,False,availability_by_ordinal,events)
            q['selected'] = candidate
            events.append(['invalidate'])
            return q, events
    if s >= 0:
        paint(q,s,False,availability_by_ordinal,events)
    q['availability'] = 0
    q['selected'] = -1
    return q, events


def release(state, x, y, availability_by_ordinal):
    q, events = hover(state,x,y,availability_by_ordinal)
    if not q['availability']:
        q['selected'] = -1
    events.append(['release_owned_capture'])
    events.append(['close'])
    return q, events


def modal_result(state, initial_ordinal):
    return state['selected'] if state['selected'] >= 0 and state['availability'] else initial_ordinal
