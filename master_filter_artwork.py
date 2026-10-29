"""Flatten the authored room to stock path leaves and generated Q10 geometry."""
import math
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
NS = '{http://www.w3.org/2000/svg}'
DYNAMIC = ('door-panel', 'door-panel-inset', 'handle-outer', 'handle-stem',
           'blind-panel', 'window-blind-bottom-bar', 'door-edge')


def points(node):
    tag = node.tag.removeprefix(NS)
    if tag == 'rect':
        x, y, w, h = (float(node.get(k, '0')) for k in ('x', 'y', 'width', 'height'))
        return [(x,y),(x+w,y),(x+w,y+h),(x,y+h)], True
    if tag == 'circle':
        x,y,r = (float(node.get(k)) for k in ('cx','cy','r'))
        return [(x+r*math.cos(i*math.tau/32),y+r*math.sin(i*math.tau/32)) for i in range(32)], True
    if tag == 'polygon':
        values = [float(x) for x in re.findall(r'-?\d+(?:\.\d+)?',node.get('points'))]
        return list(zip(values[::2],values[1::2])), True
    tokens = re.findall(r'[MLHVZ]|-?\d+(?:\.\d+)?',node.get('d',''))
    out=[]; i=0; command=''; closed=False
    while i<len(tokens):
        if tokens[i] in ('M','L','H','V','Z'):
            command=tokens[i]; i+=1
            if command=='Z': closed=True; continue
        if command in ('M','L'):
            out.append((float(tokens[i]),float(tokens[i+1])));i+=2
        elif command=='H':out.append((float(tokens[i]),out[-1][1]));i+=1
        elif command=='V':out.append((out[-1][0],float(tokens[i])));i+=1
        else:raise ValueError('Unsupported source path')
    return out,closed


def room_layout(byid):
    original,_=points(byid['room-floor-reference'])
    target,_=points(byid['floor-grid-reference'])
    if len(original)!=4 or len(target)!=4:
        raise ValueError('Floor references need four corners')
    count=int(byid['floor-grid-reference'].get('data-tiles-per-edge'))
    if count!=5 or target!=[(240.,110.),(315.,160.),(240.,210.),(165.,160.)]:
        raise ValueError('Room must span five stock-grid steps in both directions')
    sx=(target[1][0]-target[3][0])/(original[1][0]-original[3][0])
    sy=(target[2][1]-target[0][1])/(original[2][1]-original[0][1])
    def world(p):
        return (240+(p[0]-240)*sx,160+(p[1]-160)*sy)
    if any(abs(a-b)>1e-9 for p,q in zip(original,target) for a,b in zip(world(p),q)):
        raise ValueError('Floor references do not share the room affine transform')
    return sx,sy,world


def generate():
    source=ET.parse(ROOT/'artwork/master_dj_filter.source.svg').getroot()
    byid={n.get('id'):n for n in source.iter() if n.get('id')}
    sx,sy,world=room_layout(byid)
    root=ET.Element('svg',{'xmlns':NS[1:-1],'width':'480','height':'222','viewBox':'0 0 480 222'})
    geometry=[]
    def path(name,coords,closed,style):
        def number(n):return f'{n:.4f}'.rstrip('0').rstrip('.') or '0'
        d=' '.join(('M' if i==0 else 'L')+' '+number(x)+' '+number(y) for i,(x,y) in enumerate(coords))
        if closed:d+=' Z'
        ET.SubElement(root,'path',dict(id=name,d=d,fill=style.get('fill','none'),**{k:v for k,v in style.items() if k!='fill'}))
    def grout(name,poly):
        # Stock-sized lattice: steps (15,10),(-15,10), both intercepts 10 mod20.
        segments=[]
        for slope in (-2/3,2/3):
            offsets=[y-slope*x for x,y in poly]
            first=math.floor((min(offsets)-10)/20)
            last=math.ceil((max(offsets)-10)/20)
            for index in range(first,last+1):
                intercept=10+20*index
                p=(0.,float(intercept));q=(480.,intercept+slope*480)
                lower,upper=0.,1.
                boundary=poly
                if sum(boundary[i][0]*boundary[(i+1)%len(boundary)][1]-boundary[(i+1)%len(boundary)][0]*boundary[i][1] for i in range(len(boundary)))<0:boundary=list(reversed(boundary))
                for a,b in zip(boundary,boundary[1:]+boundary[:1]):
                    s=(b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
                    delta=(b[0]-a[0])*(q[1]-p[1])-(b[1]-a[1])*(q[0]-p[0])
                    if abs(delta)<1e-12:
                        if s<0:upper=-1;break
                    elif delta>0:lower=max(lower,-s/delta)
                    else:upper=min(upper,-s/delta)
                if upper-lower>1e-9:
                    segments.append([(p[0]+t*(q[0]-p[0]),p[1]+t*(q[1]-p[1])) for t in (lower,upper)])
        # Stock merges repeated coordinates inside a path. Separate leaves
        # preserve every line at the shared canvas-boundary intersections.
        for i, segment in enumerate(segments):
            path(name+'-'+str(i),segment,False,{'stroke':'#000000','stroke-width':'0.5','stroke-linecap':'butt'})
    canvas=[(0,0),(480,0),(480,222),(0,222)]
    path('outer-color',canvas,True,{'fill':'#000000'});grout('outer-grout',canvas)
    def walk(node,inherited):
        style=dict(inherited)
        for key in ('fill','stroke','stroke-width','stroke-linecap','stroke-linejoin'):
            if key in node.attrib:style[key]=node.get(key)
        if node.tag==NS+'g':
            for child in node:walk(child,style)
            return
        name=node.get('id'); coords,closed=points(node)
        if name in DYNAMIC:
            geometry.append((name,[(sx*x,sy*y) for x,y in coords],closed))
            if name.startswith('blind-') or name=='window-blind-bottom-bar':
                coords=[(269.02+x,70.9+.656*x+y) for x,y in coords]
            else:coords=[(215.27+x,65.36+(18.81/28.54)*x+y) for x,y in coords]
        coords=[world(p) for p in coords]
        if node.get('fill-rule')=='evenodd':
            # Both authored rings have corresponding four-corner loops.
            outer,inner=coords[:4],coords[4:]
            if len(inner)!=4:
                raise ValueError("Room rings need corresponding four-corner loops")
            pieces=[]
            for i in range(4):
                quad=[outer[i],outer[(i+1)%4],inner[(i+1)%4],inner[i]]
                pieces.append('M '+' L '.join(f'{x:.4f} {y:.4f}' for x,y in quad)+' Z')
            ET.SubElement(root,'path',{'id':name,'d':' '.join(pieces),'fill':style['fill']})
            if style.get('stroke','none')!='none':
                path(name+'-outer',outer,True,dict(style,fill='none'));path(name+'-inner',inner,True,dict(style,fill='none'))
        else:path(name,coords,closed,style)
    walk(byid['room'],{})
    # Never flatten the filled door to zero area in the stock rasterizer.
    # This independent two-point stroke represents the edge-on silhouette.
    geometry.append(('door-edge',[(0,0),(0,59.84*sy)],False))
    path('door-edge',[world((215.27,65.36)),world((215.27,151.8))],False,{'stroke':'#C9C9C9','stroke-width':'1.2','stroke-linecap':'butt'})
    for gray in range(237):
        path('gray-'+str(gray),[(640,320),(641,320),(641,321),(640,321)],True,{'fill':f'#{gray:02X}{gray:02X}{gray:02X}'})
    data=ET.tostring(root,encoding='utf-8',xml_declaration=True)+b'\n'
    ordered=[next(g for g in geometry if g[0]==name) for name in DYNAMIC]
    header=['// Generated by master_filter_artwork.py from the authored room.','#pragma once',
            '#include <opxy/types.hpp>','namespace opxy { namespace extensions { namespace room {',
            'struct Point { i32 x,y; };','struct Shape { const char* id; u32 count; bool closed; Point points[32]; };',
            'static const u32 shape_count = 7;']
    hinge=world((215.27,65.36));blind=world((269.02,70.9))
    header += ['static const Point door_origin = {'+','.join(str(round(v*1024)) for v in hinge)+'};',
               'static const Point blind_origin = {'+','.join(str(round(v*1024)) for v in blind)+'};',
               'static const i32 door_width = '+str(round(28.54*sx*1024))+';',
               'static const i32 door_height = '+str(round(59.84*sy*1024))+';',
               'static const i32 blind_skew = '+str(round(.656*sy/sx*1024))+';',
               'static const i32 door_min_projection = '+str(math.ceil(1024/(28.54*sx)))+';',
               'static const i32 blind_min_coverage = '+str(math.ceil(1024/(34.92*sy)))+';',
               'static const Shape shapes[shape_count] = {']
    for name,coords,closed in ordered:
        header.append('    {"'+name+'", '+str(len(coords))+', '+('true' if closed else 'false')+', {'+', '.join('{'+str(round(x*1024))+','+str(round(y*1024))+'}' for x,y in coords)+'}},')
    header+=['};','// Q16 door projection coefficients for low=0..1.','static const i32 door[257][2] = {']
    for i in range(257):
        theta=i*math.pi/512;s,c=math.sin(theta),math.cos(theta)
        header.append('    {'+str(round((s-c)*65536))+','+str(round((18.81/28.54)*(sy/sx)*(s+c)*65536))+'},')
    header+=['};','}}}','']
    return data,'\n'.join(header)


if __name__=='__main__':
    svg,header=generate()
    (ROOT/'artwork/master_dj_filter.svg').write_bytes(svg)
    (ROOT/'master_filter_room_data.hpp').write_text(header,encoding='utf-8',newline='\n')
