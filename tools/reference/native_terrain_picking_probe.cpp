#include "game/frontend/NativeViewGeometry.h"
#include <fstream>
#include <iostream>
#include <vector>
namespace native=game::frontend::native_terrain;
void Write(std::ofstream& out,int value) {
    const uint32_t bits=uint32_t(value);
    const char bytes[4]={char(bits),char(bits>>8),char(bits>>16),char(bits>>24)};out.write(bytes,4);
}
int main(int argc,char** argv) {
    if(argc!=3) {return 1;}
    std::ifstream input(argv[1]);std::ofstream out(argv[2],std::ios::binary);
    int w,h,vw,vh,cx,cy,zoom,relief,wrap,pattern;size_t count;
    if(!(input>>w>>h>>vw>>vh>>cx>>cy>>zoom>>relief>>wrap>>pattern>>count) || count>100000 || !out) {return 1;}
    game::native_terrain::World world(w,h);world.horizontal_wrap=wrap;
    for(int y=0;y<h;++y) {for(int x=y&1;x<w;x+=2) {
        auto& cell=world.tiles[size_t(y)*size_t(w/2)+size_t(x/2)];
        cell.band=pattern==1?(x==cx && y==cy?5:3):pattern==2?(x<cx?3:x<cx+4?4:5):pattern==3?(x<cx?2:4):3;
    }}
    native::ViewGeometry geometry(world);const auto view=native::Layout(cx,cy,zoom,wrap,{{w,h}},{{vw,vh}});
    for(const int n:{view.first[0],view.first[1],view.unit,view.span[0],view.span[1],view.span[2],view.span[3],view.offset[0],view.offset[1]}) {Write(out,n);}
    for(size_t i=0;i<count;++i) {
        int px,py;if(!(input>>px>>py)) {return 1;}
        std::vector<std::array<int,4>> queries,edges;
        const auto result=geometry.Pick(view,zoom,relief,{{px,py}},[&](const auto& q) {
            queries.push_back({{q.tile[0],q.tile[1],int(q.vertex),q.result[0]}});
        },[&](const auto& step) {edges.push_back({{step.lhs,step.rhs,step.next_x,step.row}});});
        int ties=0;for(const auto& edge:edges) {ties+=edge[0]==edge[1];}
        const native::Point tile=result.written.value_or(native::Point{{-1234567,-7654321}});
        for(const int n:{px,py,result.code,tile[0],tile[1],int(queries.size()),int(edges.size()),ties}) {Write(out,n);}
        for(const auto& query:queries) {for(const int n:query) {Write(out,n);}}
        for(const auto& edge:edges) {for(const int n:edge) {Write(out,n);}}
    }
    if(!out) {return 1;}
}
