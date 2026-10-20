#include "types/texture/SmacIndexedSprite.h"
#include <iostream>
#include <fstream>
#include <filesystem>
int main(int argc,char** argv) {
    if(argc!=2) {return 1;}namespace native=types::texture::smac_indexed_sprite;
    const std::filesystem::path out(argv[1]);std::filesystem::create_directories(out);
    int count;std::cin>>count;
    std::array<uint8_t,256> shadow{};for(auto& value:shadow) {int n;std::cin>>n;value=uint8_t(n);}
    for(int index=0;index<count;++index) {
        int fw,fh,pitch,w,h,sp,n,d,ax,ay,ox,oy,l,t,r,b,mode;size_t bytes;
        std::cin>>fw>>fh>>pitch>>w>>h>>sp>>n>>d>>ax>>ay>>ox>>oy>>l>>t>>r>>b>>mode>>bytes;
        if(!std::cin || bytes>1000000 || fw<=0 || fh<=0 || pitch<fw) {throw std::runtime_error("Invalid probe dimensions");}
        std::vector<uint8_t> source(bytes),frame(size_t(pitch)*size_t(fh));
        for(auto& value:source) {int v;std::cin>>v;value=uint8_t(v);}
        for(int y=0;y<fh;++y) {for(int x=0;x<pitch;++x) {frame[size_t(y)*pitch+x]=uint8_t(x<fw?(23+x+3*y)%256:253);}}
        native::Source sprite{source,w,h,sp,{{ox,oy}},9};
        const auto plan=native::Prepare(sprite,{{ax,ay}},{{n,d}},{{std::max(0,l),std::max(0,t),std::min(fw,r),std::min(fh,b)}});
        const auto stem=out/std::to_string(index);std::ofstream stores(stem.string()+".stores",std::ios::binary);
        auto stats=native::Paint(sprite,plan,[&](int x,int y){return frame.at(size_t(y)*pitch+x);},[&](int x,int y,uint8_t value) {
            const uint32_t offset=uint32_t(y*pitch+x);frame.at(offset)=value;
            for(unsigned byte=0;byte<4;++byte) {stores.put(char(offset>>(8*byte)));}stores.put(char(value));
        },mode==1?&shadow:nullptr,mode==2?std::optional<uint8_t>{248}:std::nullopt);
        std::ofstream pixels(stem.string()+".frame",std::ios::binary);pixels.write(reinterpret_cast<const char*>(frame.data()),frame.size());
        std::cout<<index<<' '<<stats.writes;
        if(plan.rectangle) {for(auto p:*plan.rectangle) {std::cout<<' '<<p;}}else {std::cout<<" none";}
        std::cout<<'\n';
    }
    if(!std::cin) {throw std::runtime_error("Truncated probe input");}
}
