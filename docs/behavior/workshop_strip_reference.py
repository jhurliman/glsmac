"""Saved-strip card geometry and cyclic offset contracts, source-derived."""
def truncdiv(a,b):return a//b if a>=0 else -((-a)//b)


def cards(browse,offset,strip,left_arrow,right_arrow,active,admitted=lambda i:True):
    if not browse:return []
    start=left_arrow[2]+3
    width=truncdiv(right_arrow[0]-3-start+2,10)-2
    top,bottom=strip[1]+3,strip[3]-3
    out=[]
    for k in range(offset,offset+len(browse)):
        design=browse[k%len(browse)]
        if not admitted(design):continue
        if len(out)==10:break
        left=start+len(out)*(width+2);outer=[left,top,left+width,bottom]
        inner=[left+2,top+2,left+width-2,bottom-2]
        out.append(dict(design=design,outer=outer,inner=inner,hits=[outer,inner] if active(design) else [outer],model=[left+15,strip[1]+15] if active(design) else None))
    return out


def arrow(offset,count,right):
    if right:
        offset+=1
        return 0 if offset==count else offset
    offset-=1
    return count if offset<0 else offset


def hit(rows,x,y):
    for index in range(len(rows)-1,-1,-1):
        l,t,r,b,kind,value=rows[index]
        if l<=x<r and t<=y<b:return index,value,kind
    return -1,None,None


def trim_caption(text,width,measure):
    while text and measure(text)>width:text=text[:-1]
    first=text.find(' ')
    second=text.find(' ',first+1) if first>=0 else -1
    return text[:second] if second>=0 else text
