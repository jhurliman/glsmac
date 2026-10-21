"""Source-derived parent component name layout, using retained card metrics.

No string-width or font raster implementation: caller supplies measured width
and wrapped line count. RECT bottoms are alignment bounds, not draw clips.
"""
def name_layout(kind, measured_width, lines=1, font_baseline=7, line_step=10):
    left,top,right,bottom={'chassis':(30,7,129,92),'weapon':(30,101,129,186),'armor':(30,195,129,280),'reactor':(462,7,561,92)}[kind]
    F=font_baseline;L=line_step
    if kind=='reactor':
        height=lines*L;y=bottom-5-height-F
        rect=[left,y,right,y+F];baselines=[y+F+j*L for j in range(lines)]
        value_rect=[left,y+height,right,y+height+F]
        return dict(mode='wrap',name_rect=rect,name_baselines=baselines,value_rect=value_rect,value_baseline=value_rect[3])
    y=bottom-5-2*F;wrap=measured_width>right-left
    rect=[left,y-(L if wrap else 0),right,y+F-(L if wrap else 0)]
    baselines=[rect[1]+F+j*L for j in range(lines if wrap else 1)]
    value_rect=[left,y+F+3,right,y+2*F+3]
    return dict(mode='wrap' if wrap else 'single_center',name_rect=rect,name_baselines=baselines,value_rect=value_rect,value_baseline=value_rect[3])
