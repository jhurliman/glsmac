"""Geometry of the captured mode0, 1024x768, MainFont16 action dialogs.

This is a bounded profile reference, not a replacement generic text wrapper.
Body line count is an input measured by the native/localized font wrapper.
"""


def action_modal(body_lines, chooser=False):
    if body_lines not in (1, 2):
        raise ValueError('Captured profile covers one/two body lines only')
    top = 44 + 20 * body_lines
    bottom = top + 44
    buttons = ([[34, bottom + 9, 359, bottom + 29],
                [365, bottom + 9, 690, bottom + 29]] if chooser else
               [[33, bottom + 9, 690, bottom + 29]])
    return dict(window=[149, 255, 906, 555], client=[0, 0, 757, 300],
                frames=[[23, 0, 702, 305], [27, 4, 698, 28],
                        [30, 7, 695, 25], [30, 35, 695, 38 + 20 * body_lines],
                        [27, 32, 698, bottom + 3]],
                list_rect=[33, top, 690, bottom], buttons=buttons,
                row_rects=[[0, 1, 657, 21], [0, 23, 657, 43]],
                body_baselines=[50 + 20 * i for i in range(body_lines)],
                row_baselines=[17, 39])
