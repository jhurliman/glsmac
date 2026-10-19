#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "game/backend/map/SmacTerrainCorners.h"

namespace game::smac_detail {
using Point=std::array<int,2>;
using Rect=std::array<int,4>;
inline bool AdmitsBase(bool active,bool present,bool geography_known,bool owned,bool visited) {return active && present && geography_known && (owned || visited);}
struct Tile {uint8_t band=0;bool known=false,base_known=false,supported=true;std::array<int,5> heights{};};
struct Base {Point tile;std::string file;int population=1;bool owned=true;int crop_y=1;};
struct World {bool wrap=true;std::array<Tile,3200> tiles{};std::vector<Base> bases;};
struct Geometry {Point first;std::array<int,4> span{{5,5,5,5}};};
inline Geometry View(Point center,bool wrap=true) {
    if(center[0]<0 || center[0]>=80 || center[1]<0 || center[1]>=80) {throw std::invalid_argument("Detail requires retained80x80 camera");}
    int x=center[0]-5,y=std::clamp(center[1]-5,-2,74);
    if(wrap) {if(x<0) {x+=80;}else if(x>=80) {x-=80;}}
    else {x=std::clamp(x,-4,74);}
    x=(x&~1)|(y&1);return {{{x,y}}};
}
// Default-relief zoom-7 uses the quantized step14 before shared-corner means.
// This deliberately does not scale already-quantized zoom0 heights.
template<class BandAt>
inline std::array<int,5> Heights(int x,int y,const BandAt& band_at) {
    std::array<int,5> out{};const auto own=band_at(x,y);if(!own || *own<3) {return out;}
    out[0]=14*(*own-3);
    for(size_t v=0;v<4;++v) {
        bool land=true;int sum=0;
        for(const auto& delta:backend::map::smac_terrain::CORNERS[v]) {
            const auto band=band_at(x+delta[0],y+delta[1]);if(!band || *band<3) {land=false;break;}
            sum+=14*(*band-3);
        }
        out[v+1]=land?sum/4:0;
    }return out;
}
struct Entry {Point tile,anchor,center;};
inline std::vector<Entry> Entries(const World& world,Point camera) {
    const auto view=View(camera,world.wrap);std::vector<Entry> out;out.reserve(55);
    for(int row=0;row<=10;++row) {const int y=view.first[1]+row;
        for(int col=0;col<5;++col) {int x=view.first[0]+(row&1)+2*col;
            if(world.wrap) {x=(x%80+80)%80;}
            if(x<0 || x>=80 || y<0 || y>=80) {continue;}
            int dx=x-view.first[0];
            if(row<10) {if(world.wrap) {if(dx<0) {dx+=80;}else if(dx>=80) {dx-=80;}}}
            else {if(dx>40) {dx-=80;}if(dx< -40) {dx+=80;}}
            const auto& h=world.tiles[size_t(y)*40+x/2].heights;
            const int average=(h[1]+h[2]+h[3]+h[4])>>2;
            out.push_back({{x,y},{28*(dx-1),14*(row-1)-average},{28*dx,14*row}});
        }
    }return out;
}
template<class Paint>
inline void Fill(const std::array<Point,4>& points,Rect clip,const Paint& paint) {
    if(clip[0]>=clip[2] || clip[1]>=clip[3]) {return;}
    int top=0,bottom=0;
    for(int i=1;i<4;++i) {if(points[i][1]<points[top][1]) {top=i;}if(points[i][1]>points[bottom][1]) {bottom=i;}}
    if(points[top][1]>=points[bottom][1]) {return;}
    struct Edge {int end,dir,x,dy,left,q,step,error,remainder;};
    const auto edge=[&](int start,int direction)->std::optional<Edge> {
        while(start!=bottom) {const int end=(start+direction+4)%4;
            const int dx=points[end][0]-points[start][0],dy=points[end][1]-points[start][1];
            if(dy<0) {return {}; }if(!dy) {start=end;continue;}
            return Edge{end,direction,points[start][0],dy,dy,dx/dy,dx<0?-1:1,dx<0?1-dy:0,std::abs(dx)%dy};
        }return {};
    };
    const auto advance=[&](Edge value)->std::optional<Edge> {
        if(!--value.left) {return edge(value.end,value.dir);}
        value.x+=value.q;value.error+=value.remainder;
        if(value.error>0) {value.x+=value.step;value.error-=value.dy;}return value;
    };
    auto a=edge(top,-1),b=edge(top,1);int y=points[top][1];
    while(a && b && y<clip[3]-1) {
        if(y>=clip[1]) {const int left=std::max(std::min(a->x,b->x),clip[0]),right=std::min(std::max(a->x,b->x),clip[2]-1);
            if(right>left) {paint(y,left,right);}}
        a=advance(*a);if(!a) {break;}b=advance(*b);++y;
    }
}
struct Frame {static constexpr int WIDTH=239,HEIGHT=119,PITCH=240;std::array<uint8_t,PITCH*HEIGHT> pixels{};};
template<class Observe>
inline Frame Terrain(const World& world,Point camera,uint8_t background,const Observe& observe) {
    Frame frame;frame.pixels.fill(background);
    for(const auto& entry:Entries(world,camera)) {
        const auto& tile=world.tiles[size_t(entry.tile[1])*40+entry.tile[0]/2];
        if(!tile.known || !tile.supported) {continue;}
        const auto& c=entry.center;const auto& h=tile.heights;
        const std::array<Point,4> points{{{c[0]-28,c[1]-h[1]},{c[0],c[1]-14-h[2]},{c[0]+28,c[1]-h[3]},{c[0],c[1]+14-h[4]}}};
        const uint8_t color=tile.band<3?190:2*(14-tile.band);observe(entry,points,color);
        Fill(points,{0,0,239,119},[&](int y,int left,int right) {std::fill(frame.pixels.begin()+y*240+left,frame.pixels.begin()+y*240+right,color);});
    }return frame;
}
inline Frame Terrain(const World& world,Point camera,uint8_t background=0) {
    return Terrain(world,camera,background,[](const auto&,const auto&,uint8_t){});
}
struct Sprite {int width=0,height=0;Point offset{};std::vector<uint8_t> pixels;};
inline std::array<int,800> Steps() {
    std::array<int,800> steps{};int error=7-18+16;
    for(auto& step:steps) {error+=14;step=1;if(error>0) {++step;error-=18;}}return steps;
}
template<class Observe>
inline void Composite(Frame& frame,Point anchor,const Sprite& sprite,const std::array<uint8_t,256>& shadow,const Observe& observe) {
    if(sprite.width<=0 || sprite.height<=0 || sprite.width>100 || sprite.height>75 || sprite.pixels.size()!=size_t(sprite.width*sprite.height)
        || sprite.offset[0]<0 || sprite.offset[1]<0 || sprite.offset[0]+sprite.width>100 || sprite.offset[1]+sprite.height>75) {throw std::invalid_argument("Invalid retained Detail Base source");}
    const int left=(anchor[0]*16+sprite.offset[0]*9)/16,top=(anchor[1]*16+sprite.offset[1]*9)/16;
    const int right=left+sprite.width*9/16,bottom=top+sprite.height*9/16;
    const int x0=std::max(0,left),x1=std::min(239,right),y0=std::max(0,top),y1=std::min(119,bottom);
    if(x0>=x1 || y0>=y1) {return;}
    const auto steps=Steps();int sy=0;
    for(int i=0;i<y0-top;++i) {sy+=steps.at(i);}
    for(int y=y0;y<y1;++y) {int sx=0;for(int i=0;i<x0-left;++i) {sx+=steps.at(i);}
        for(int x=x0;x<x1;++x) {
            const uint8_t raw=sprite.pixels.at(size_t(sy)*sprite.width+sx);observe(x,y,sx,sy,raw);
            if(raw!=9) {auto& pixel=frame.pixels[y*240+x];pixel=raw==0?shadow[pixel]:raw;}sx+=steps.at(x-left);
        }sy+=steps.at(y-top);
    }
}
inline void PopulationBox(Frame& frame,Point anchor,bool owned) {
    const int left=anchor[0],top=anchor[1],right=left+10,bottom=top+11;
    for(int y=std::max(top,0);y<std::min(bottom,119);++y) {for(int x=std::max(left,0);x<std::min(right,239);++x) {
        if(x==left || x==right-1 || y==top || y==bottom-1) {frame.pixels[y*240+x]=0;}
        else if(!owned) {frame.pixels[y*240+x]=252;}
    }}
}
}
