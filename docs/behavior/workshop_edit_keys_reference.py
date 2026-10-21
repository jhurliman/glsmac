"""Native edit-key state for valid initial byte positions in the wide test field.

Plain/Shift navigation, Backspace/Delete and character input only. Ctrl word
navigation and scrolling are outside this bounded executable reference.
"""


def edit_step(text, anchor, caret, key, character=False, maximum=31):
    data = text.encode('cp1252')
    beep = False
    handled = 1
    if character:
        if 0 <= key < 32:
            handled = 0
        elif len(data) == maximum and anchor == caret:
            beep = True
        else:
            lo, hi = sorted((anchor, caret))
            data = data[:lo] + bytes([key & 255]) + data[hi:]
            anchor = caret = lo + 1
    else:
        shift, control = bool(key & 0x10000), bool(key & 0x20000)
        code = key & 65535
        if code in (8, 46):
            if shift or control:
                beep = True
            elif anchor != caret:
                lo, hi = sorted((anchor, caret))
                data = data[:lo] + data[hi:]
                anchor = caret = lo
            elif code == 8:
                if caret == 0:
                    beep = True
                else:
                    data = data[:caret-1] + data[caret:]
                    anchor = caret = caret - 1
            elif caret == len(data):
                beep = True
            else:
                data = data[:caret] + data[caret+1:]
        elif code in (35, 36, 37, 39):
            if control and code in (37, 39):
                raise ValueError('Ctrl word navigation not covered')
            if code == 35:
                caret = len(data)
            elif code == 36:
                caret = 0
            elif code == 37:
                caret = max(0, caret - 1)
            else:
                caret += 1
                if caret == len(data) + 1:
                    if len(data) == maximum:
                        return dict(text=data.decode('cp1252'),anchor=anchor,
                                    caret=caret,beeps=[0],return_value=1)
                    data += b' '
            if not shift:
                anchor = caret
        else:
            handled = 0
    return dict(text=data.decode('cp1252'),anchor=anchor,caret=caret,
                beeps=[0] if beep else [],return_value=handled)
