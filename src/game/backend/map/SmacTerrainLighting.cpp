#include "SmacTerrainLighting.h"
#include "SmacTerrainCorners.h"
#include <limits>

namespace game::backend::map::smac_terrain {
namespace {
constexpr int SquareRoot(int value) {
    int root=0;
    for(int bit=256;bit;bit>>=1) {const auto candidate=root|bit;if(candidate*candidate<=value) {root=candidate;}}
    return root;
}
constexpr auto MakeTable() {
    std::array<int8_t,196> table{};
    for(int face=0;face<4;++face) {for(int a=-3;a<=3;++a) {for(int b=-3;b<=3;++b) {
        const int x=100*(face==0?a:face==1?-b:face==2?-a:b);
        const int y=100*(face==0?b:face==1?a:face==2?-b:-a);
        const int length=SquareRoot(x*x+y*y);
        table[static_cast<size_t>(49*face+7*(a+3)+b+3)]=static_cast<int8_t>(length?-3*(x+y)/length:0);
    }}}
    return table;
}
constexpr auto LIGHTS=MakeTable();
int FloorAverage(int sum,int count) {return sum>=0?sum/count:-((-sum+count-1)/count);}
}

std::optional<int> FaceLighting(int a,int b,unsigned face) {
    if(a< -3 || a>3 || b< -3 || b>3 || face>=4) {return {};}
    return LIGHTS[49*face+static_cast<unsigned>(7*(a+3)+b+3)];
}

std::optional<Lighting> OrdinaryLighting(int x,int y,const BandAt& at) {
    if(!at || x<std::numeric_limits<int>::min()+4 || x>std::numeric_limits<int>::max()-4 ||
        y<std::numeric_limits<int>::min()+4 || y>std::numeric_limits<int>::max()-4) {return {};}
    const auto own=at(x,y);
    if(!own || *own<3 || *own>7) {return {};}
    const auto slope=[&](int tx,int ty,int corner)->std::optional<int> {
        const auto center=at(tx,ty);
        if(!center || *center<3) {return 0;}
        if(*center>7) {return {};}
        int sum=0;
        for(const auto& offset:CORNERS[corner]) {
            const auto band=at(tx+offset[0],ty+offset[1]);
            if(!band || *band<3) {return 0;}
            if(*band>7) {return {};}
            sum+=*band;
        }
        return static_cast<int>(static_cast<unsigned>(sum-4*int(*center)+4)&15U)-4;
    };
    const auto face=[&](int tx,int ty,unsigned index)->std::optional<int> {
        const auto band=at(tx,ty);
        if(!band || *band<3) {return 0;}
        const auto a=slope(tx,ty,static_cast<int>(index));
        const auto b=slope(tx,ty,static_cast<int>((index+1)&3));
        if(!a || !b) {return {};}
        return FaceLighting(*a,*b,index);
    };
    Lighting result{};
    int sum=0;
    for(unsigned f=0;f<4;++f) {const auto light=face(x,y,f);if(!light) {return {};}sum+=*light;}
    result[0]=FloorAverage(sum,4);
    for(size_t corner=0;corner<4;++corner) {
        sum=0;
        for(unsigned i=0;i<4;++i) {
            const auto& offset=CORNERS[corner][i];
            const auto first=face(x+offset[0],y+offset[1],(i+1)&3);
            const auto second=face(x+offset[0],y+offset[1],(i+2)&3);
            if(!first || !second) {return {};}
            sum+=*first+*second;
        }
        result[corner+1]=FloorAverage(sum,8);
    }
    return result;
}
}
