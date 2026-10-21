"""Captured1024/mode0/MainFont16 land illustration prompt geometry."""


def prompt_geometry(body_lines, choices):
    width = 755 if choices else 758
    button = ([33,217,688,237] if choices else [33,167,691,187])
    return dict(client=[0,0,width,300],
                screen=[150,255,905,555] if choices else [149,255,907,555],
                frames=[[23,0,width-55,305],[27,4,width-59,28],
                        [30,7,width-62,25],
                        [236,35,width-62,38+20*body_lines],
                        [233,32,width-59,41+20*body_lines]],
                button=button,
                list_rect=[33,164,688,208] if choices else None,
                rows=[[0,1,655,21],[0,23,655,43]] if choices else [],
                text_x=239,body_baselines=[50+20*i for i in range(body_lines)])
