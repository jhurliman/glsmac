#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace types::texture::smac_indexed_sprite {
using Point=std::array<int32_t,2>;
using Rect=std::array<int32_t,4>;
struct Source {const std::vector<uint8_t>& pixels;int width,height,pitch;Point offset;uint8_t key=9;};
struct Plan {
    std::optional<Rect> rectangle;
    Rect visible{};
    std::vector<int> xs,ys;
    bool identity=false,enlarged=false;
};
// Round the rational to the original cached IEEE binary32 ratio, independently
// of host FP mode or MSVC's double-sized long double. Admitted ratios use16-bit
// positive integers; all ordinary zoom and the measured overrides fit here.
inline uint32_t RatioBits(int n,int d) {
    if(n<=0 || d<=0 || n>65535 || d>65535) {throw std::invalid_argument("Invalid native sprite scale ratio");}
    uint64_t num=n,den=d;int exponent=0;
    while(num<den) {num*=2;--exponent;}
    while(num>=2*den) {den*=2;++exponent;}
    const uint64_t fixed=num<<23;uint64_t mantissa=fixed/den;const auto remainder=fixed%den;
    if(2*remainder>den || (2*remainder==den && (mantissa&1))) {++mantissa;}
    if(mantissa==(1u<<24)) {mantissa>>=1;++exponent;}
    return (uint32_t(exponent+127)<<23)|(uint32_t(mantissa)&0x7fffffu);
}
inline int Scaled(int value,int anchor,uint32_t bits) {
    if((bits>>23)<111u || (bits>>23)>129u) {throw std::invalid_argument("Unsupported native sprite scale exponent");}
    const auto shift=150u-(bits>>23);const int64_t denominator=int64_t(1)<<shift;
    const int64_t mantissa=(bits&0x7fffffu)|0x800000u;
    return int((int64_t(anchor)*denominator+int64_t(value)*mantissa)/denominator);
}
inline std::array<int,800> Steps(int n,int d) {
    if(n<=0 || d<=0 || n>65535 || d>65535) {throw std::invalid_argument("Invalid native sprite step ratio");}
    const int64_t hi=std::max(n,d),lo=std::min(n,d),q=hi/lo,rem=hi%lo;
    std::array<int,800> result{};int64_t error=rem-2*lo+((q&1)?hi:0);
    for(auto& step:result) {
        step=int(q);if(lo==1) {continue;}
        error+=2*rem;if(error>0) {++step;error-=2*lo;}
    }return result;
}
inline std::vector<int> Samples(int length,int extent,int n,int d,const std::array<int,800>& steps) {
    std::vector<int> result;result.reserve(extent);
    if(n<=d) {
        int source=0;
        for(int i=0;i<extent;++i) {result.push_back(source);source+=n==d?1:steps.at(i);}
    } else {
        for(int source=0;source<length && int(result.size())<extent;++source) {
            for(int j=0;j<steps.at(source) && int(result.size())<extent;++j) {result.push_back(source);}
        }
    }
    if(int(result.size())!=extent || std::any_of(result.begin(),result.end(),[&](int v){return v<0 || v>=length;})) {
        throw std::invalid_argument("Native sprite sampling exceeds retained source");
    }return result;
}
inline Plan Prepare(const Source& source,Point anchor,Point ratio,Rect clip) {
    const auto bits=RatioBits(ratio[0],ratio[1]);Plan plan;
    if(source.width<=0 || source.height<=0 || source.width>800 || source.height>800 || source.pitch<source.width ||
        uint64_t(source.height-1)*uint64_t(source.pitch)+uint64_t(source.width)>source.pixels.size()) {
        throw std::invalid_argument("Invalid retained native sprite source");
    }
    for(const auto p:{source.offset,anchor}) {for(const auto value:p) {
        if(value< -32768 || value>32767) {throw std::invalid_argument("Unsupported native sprite coordinate");}
    }}
    if(bits>0x40800000u) {return plan;} // original software path rejects scale>4
    plan.identity=ratio[0]==ratio[1];plan.enlarged=ratio[0]>ratio[1];
    const int w=plan.identity?source.width:Scaled(source.width,0,bits),h=plan.identity?source.height:Scaled(source.height,0,bits);
    const int left=plan.identity?anchor[0]+source.offset[0]:Scaled(source.offset[0],anchor[0],bits);
    const int top=plan.identity?anchor[1]+source.offset[1]:Scaled(source.offset[1],anchor[1],bits);
    plan.rectangle=Rect{{left,top,left+w,top+h}};
    plan.visible={{std::max(left,clip[0]),std::max(top,clip[1]),std::min(left+w,clip[2]),std::min(top+h,clip[3])}};
    if(plan.visible[0]>=plan.visible[2] || plan.visible[1]>=plan.visible[3]) {return plan;}
    const auto steps=Steps(ratio[0],ratio[1]);
    plan.xs=Samples(source.width,w,ratio[0],ratio[1],steps);plan.ys=Samples(source.height,h,ratio[0],ratio[1],steps);
    return plan;
}
struct Statistics {uint64_t attempts=0,writes=0;};
// Clip must already be intersected with the destination. Read and Store are
// synchronous: no source, destination, shadow table or callback is retained.
template<class Read,class Store>
Statistics Paint(const Source& source,const Plan& plan,Read read,Store store,
    const std::array<uint8_t,256>* shadow=nullptr,std::optional<uint8_t> solid=std::nullopt) {
    Statistics stats;
    if(!plan.rectangle || plan.xs.empty() || plan.ys.empty()) {return stats;}
    const int left=plan.visible[0],top=plan.visible[1],right=plan.visible[2],bottom=plan.visible[3];
    const int origin_x=plan.rectangle->at(0),origin_y=plan.rectangle->at(1);
    const auto source_at=[&](int x,int y) {
        int64_t off=int64_t(plan.ys.at(y-origin_y))*source.pitch+plan.xs.at(x-origin_x);
        // Measured solid-mask identity quirk: the initial clipped row uses
        // logical retained width, while subsequent rows advance with pitch.
        if(solid && plan.identity) {off+=int64_t(top-origin_y)*(source.width-source.pitch);}
        if(off<0 || uint64_t(off)>=source.pixels.size()) {throw std::invalid_argument("Invalid native sprite row offset");}
        return source.pixels[size_t(off)];
    };
    const auto paint=[&](int x,int y,uint8_t src,std::optional<uint8_t> block_shadow) {
        ++stats.attempts;if(src==source.key) {return;}
        const uint8_t value=solid?*solid:shadow && src==0?(block_shadow?*block_shadow:(*shadow)[read(x,y)]):src;
        store(x,y,value);++stats.writes;
    };
    if(!plan.enlarged) {
        for(int y=top;y<bottom;++y) {for(int x=left;x<right;++x) {paint(x,y,source_at(x,y),{});}}
    } else {
        for(int y=top;y<bottom;) {
            int yn=y+1;while(yn<bottom && plan.ys[yn-origin_y]==plan.ys[y-origin_y]) {++yn;}
            for(int x=left;x<right;) {
                int xn=x+1;while(xn<right && plan.xs[xn-origin_x]==plan.xs[x-origin_x]) {++xn;}
                const auto src=source_at(x,y);
                const auto block_shadow=shadow && src==0 && src!=source.key?std::optional<uint8_t>{(*shadow)[read(x,y)]}:std::nullopt;
                for(int yy=y;yy<yn;++yy) {for(int xx=x;xx<xn;++xx) {paint(xx,yy,src,block_shadow);}}
                x=xn;
            }y=yn;
        }
    }return stats;
}
}
