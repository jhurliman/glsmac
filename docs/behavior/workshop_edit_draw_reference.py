"""Native custom edit selection recoloring and caret geometry, resolved inputs."""


def selected_pixel(index, normal_foreground=224, normal_background=0,
                   selected_foreground=0, selected_background=36):
    temporary = (selected_background + 1) & 255
    for before, after in [(normal_background, temporary),
                          (normal_foreground, selected_foreground),
                          (temporary, selected_background)]:
        if index == before:
            index = after
    return index


def caret(prefix_width, inset, font_height, bitmap_height):
    """Requested x/y0/y1 inclusive; bitmap clipping happens in the line helper."""
    top = (bitmap_height - font_height) // 2
    return prefix_width + inset, top, top + font_height
