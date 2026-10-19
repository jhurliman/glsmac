#include "SmacDevelopedOverlay.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace game::frontend::smac_developed_overlay {
std::vector<Selection> Select(uint32_t items,uint8_t band,int64_t nutrients,uint32_t landmarks,uint32_t flags,int zoom) {
    if(band>7) {throw std::invalid_argument("Unsupported developed overlay altitude band");}
    std::vector<Selection> out;
    const bool land=band>=3;
    const auto add=[&](int x,int y,bool shadow=false){out.push_back({{{x,y,100,62}},shadow});};
    if(!land && (flags&1) && (items&0x8000)) {add(607,190);}
    if(!(flags&0x20) || zoom<=-8) {return out;}
    if(land && (items&0x800)) {add(506,316);}
    if(land && (items&0x40000)) {add(607,316);}
    if(items&0x80000000u) {add(708,316,true);}
    if(land && (items&0x8000)) {
        const auto tier=nutrients<=1?0:nutrients>=4?3:static_cast<int>(nutrients-1);
        add(items&0x80000?822:923,453+63*tier);
    }
    if((items&0x10) && (land || band==2)) {add(land?607:506,64);}
    if((items&0x40) && (land || band==2)) {add(land?607:506,127);}
    if(items&0x400000) {add(506,253);}
    if(items&0x800000) {add(607,253);}
    if(items&0x1000000) {add(708,(landmarks&0x80001000u)==0x1000?190:253);}
    if(land && (items&0x88000)==0x88000 && nutrients>=5) {add(822,453);}
    return out;
}
Sprite Decode(const smac_terrain_raster::Bitmap& atlas,const Crop& crop) {
    const int x=crop[0],y=crop[1],w=crop[2],h=crop[3];
    if(atlas.width<=0 || atlas.height<=0 || uint64_t(atlas.width)*uint32_t(atlas.height)!=atlas.pixels.size()
        || w!=100 || h!=62 || x<0 || y<0 || x>atlas.width-w || y>atlas.height-h) {
        throw std::invalid_argument("Invalid developed overlay atlas crop");
    }
    const auto at=[&](int sx,int sy){const auto v=atlas.pixels[static_cast<size_t>(y+sy)*size_t(atlas.width)+size_t(x+sx)];return uint8_t(v==6 || v==7?9:v);};
    int left=w,top=h,right=0,bottom=0;
    for(int sy=0;sy<h;++sy) {for(int sx=0;sx<w;++sx) {if(at(sx,sy)!=9) {
        left=std::min(left,sx);top=std::min(top,sy);right=std::max(right,sx+1);bottom=std::max(bottom,sy+1);
    }}}
    if(right<=left || bottom<=top) {return {{0,0,{}},{{0,0}}};}
    Sprite result{{right-left,bottom-top,{}},{{left,top}}};
    result.bitmap.pixels.reserve(size_t(right-left)*size_t(bottom-top));
    for(int sy=top;sy<bottom;++sy) {for(int sx=left;sx<right;++sx) {result.bitmap.pixels.push_back(at(sx,sy));}}
    return result;
}
bool ReplacesLegacySprite(std::string_view name) {
    for(const auto candidate:{"Condenser","FarmLand","FarmSea","SolarLand","SolarSea","MineLand","MineSea",
        "ThermalBorehole","SoilEnricher","EchelonMirror","Airbase","Sensor","Bunker"}) {
        if(name==candidate) {return true;}
    }
    return false;
}
std::array<uint8_t,256> ShadowLookup(const Palette& palette) {
    using Triple=std::array<double,3>;
    std::array<Triple,256> points{};
    for(size_t i=0;i<256;++i) {
        const double r=palette[i][0]/256.0,g=palette[i][1]/256.0,b=palette[i][2]/256.0;
        const double value=std::max({r,g,b}),low=b<value?b:std::min(r,g);
        const double saturation=value?(value-low)/value:0;
        double hue=-1;
        if(saturation) {
            const auto delta=value-low;
            hue=(r==value?(g-b)/delta:g==value?2+(b-r)/delta:4+(r-g)/delta)*60;
            if(hue<0) {hue+=360;}
        }
        const auto radians=hue*(3.14159265358979323846/180.0);
        points[i]={saturation*std::cos(radians),saturation*std::sin(radians),value};
    }
    std::array<uint8_t,256> result{};
    for(size_t i=0;i<256;++i) {
        auto target=points[i];target[2]*=.57;
        double best=std::numeric_limits<double>::infinity();
        for(size_t candidate=10;candidate<=245;++candidate) {
            double distance=0;for(size_t c=0;c<3;++c) {const auto delta=target[c]-points[candidate][c];distance+=delta*delta;}
            if(distance<best) {best=distance;result[i]=static_cast<uint8_t>(candidate);}
        }
    }
    return result;
}
}
