"""WHICHUPGRADE2 equal-height row grid, source627740, resolved text width."""


def upgrade_grid(count, max_text_width, minimum_width=660, row_limit=12,
                 text_height=19, gap=3, padding=3):
    assert count > 0
    columns = 1 if count <= row_limit else count // row_limit + 1
    rows = (count + columns - 1) // columns
    outer = max(minimum_width, columns * (max_text_width + gap + padding))
    cell_width = outer // columns - gap
    return dict(columns=columns, rows=rows, cell_width=cell_width,
                column_stride=cell_width + gap, row_stride=text_height + gap,
                width=outer - gap, height=rows * (text_height + gap))


def cell_rect(ordinal, grid):
    column, row = divmod(ordinal, grid['rows'])
    x = column * grid['column_stride']
    y = row * grid['row_stride'] + 1
    return [x, y, x + grid['cell_width'], y + 20]
