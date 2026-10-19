#include "SmacTerritoryBoundary.h"
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace game::frontend::smac_territory_boundary {
int Owner(int raw,const Context& context) {
    if(raw<=0) {return -1;}
    if(raw>7 || context.viewer<0 || context.viewer>7) {throw std::invalid_argument("Invalid territory owner/viewer");}
    return raw==context.viewer || context.scenario_bypass || (context.relations[raw]&0x08000008u)==0x08000008u?raw:-1;
}
std::optional<Pass> Select(int x,int y,const std::array<raster::Point,5>& points,
    const CellAt& cells,const Context& context,uint32_t flags) {
    if(!(flags&0x10000) || context.override_mode || (!context.visible && !context.visibility_bypass && !context.scenario_bypass) ||
        x<0 || x>=80 || y<0 || y>=80 || ((x^y)&1)) {return {};}
    const auto own=cells(x,y);if(!own) {return {};}
    const int owner=Owner(own->owner,context);if(owner<0) {return {};}
    constexpr std::array<raster::Point,8> offsets{{{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}}};
    Mapping mapping{};bool needed=false;
    for(unsigned i=0;i<8;++i) {
        const auto neighbor=cells(x+offsets[i][0],y+offsets[i][1]);
        if(!neighbor || (neighbor->band<3)!=(own->band<3) || neighbor->region!=own->region || Owner(neighbor->owner,context)==owner) {continue;}
        needed=true;const auto value=static_cast<uint8_t>(owner+1);
        if(i&1) {mapping[i+1]=value;}
        else if(!i) {mapping[8]=mapping[2]=mapping[1]=value;}
        else {mapping[i]=mapping[i+2]=mapping[i+1]=value;}
    }
    if(!needed) {return {};}
    Pass pass{{},own->band<3?std::array<int32_t,2>{{-256,256}}:std::array<int32_t,2>{{-4,4}},mapping};
    constexpr std::array<raster::Point,4> uv{{{{0,0}},{{59,0}},{{59,59}},{{0,59}}}};
    pass.polygon.count=4;
    for(unsigned i=0;i<4;++i) {pass.polygon.vertices[i]={points[i+1],uv[i],0};}
    return pass;
}
raster::Bitmap Mask(const raster::Bitmap& rainfall) {
    if(rainfall.width<62 || rainfall.height<314 || rainfall.width>=32767 || rainfall.height>=32767 ||
        uint64_t(rainfall.width)*static_cast<uint32_t>(rainfall.height)!=rainfall.pixels.size()) {throw std::invalid_argument("Invalid territory mask source");}
    std::array<uint8_t,256> classes{};
    constexpr std::array<uint8_t,9> source{{211,208,41,143,116,151,206,95,192}};
    for(size_t i=0;i<source.size();++i) {classes[static_cast<uint8_t>(source[i]+10)]=static_cast<uint8_t>(i);}
    raster::Bitmap result{60,60,std::vector<uint8_t>(3600)};
    for(size_t y=0;y<60;++y) {for(size_t x=0;x<60;++x) {
        const auto loaded=rainfall.pixels[(254+y)*static_cast<size_t>(rainfall.width)+2+x];
        result.pixels[y*60+x]=loaded==9?9:classes[loaded];
    }}
    return result;
}
std::array<uint8_t,256> Row(const smac_land_material::Palette& colors,uint8_t target,int opacity) {
    if(opacity<0 || opacity>100) {throw std::invalid_argument("Boundary opacity requires0..100");}
    std::array<uint8_t,256> row{};std::iota(row.begin(),row.end(),0);
    for(int destination=10;destination<=245;++destination) {
        std::array<int,3> mixed{};
        for(size_t c=0;c<3;++c) {mixed[c]=((100-opacity)*colors[destination][c]+opacity*colors[target][c])/100;}
        int best=std::numeric_limits<int>::max();
        for(int candidate=10;candidate<=245;++candidate) {
            int distance=0;
            for(size_t c=0;c<3;++c) {const int delta=mixed[c]-colors[candidate][c];distance+=delta*delta;}
            if(distance<best) {best=distance;row[destination]=static_cast<uint8_t>(candidate);}
        }
    }
    return row;
}
raster::OwnerClut Rows(const smac_land_material::Palette& colors,const std::array<uint8_t,8>& targets,const std::array<int,8>& opacities) {
    raster::OwnerClut rows{};for(size_t i=0;i<rows.size();++i) {rows[i]=Row(colors,targets[i],opacities[i]);}return rows;
}
std::array<int,8> ReadOpacities(std::string_view system) {
    std::istringstream input{std::string(system)};std::string line;bool found=false;size_t count=0;std::array<int,8> result{};
    while(std::getline(input,line)) {
        const auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos) {continue;}line.erase(0,first);
        const auto last=line.find_last_not_of(" \t\r");line.erase(last+1);
        if(!found) {if(line=="#BOUNDARYPERCENTS") {found=true;}continue;}
        if(line[0]=='#') {break;}if(line[0]==';') {continue;}
        std::istringstream value(line);int opacity=0;char separator=0;
        if(!(value>>opacity) || opacity<0 || opacity>100 || (value>>separator && separator!=',' && separator!=';')) {throw std::invalid_argument("Invalid BOUNDARYPERCENTS entry");}
        result[count++]=opacity;if(count==result.size()) {return result;}
    }
    throw std::invalid_argument("Missing eight BOUNDARYPERCENTS entries");
}
}
