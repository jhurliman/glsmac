#include "SmacLandmarkGeneration.h"
#include "SmacLandmarkGenerationTables.h"
#include "SmacClimate.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace game::backend::map::smac_generation {
namespace {
uint32_t Read(const Record& r,size_t f) {return uint32_t(r[f])|(uint32_t(r[f+1])<<8)|(uint32_t(r[f+2])<<16)|(uint32_t(r[f+3])<<24);}
struct Cell {size_t offset,index;Coordinate point;};
class Builder {
public:
    Builder(State& state,Geometry geometry,const WorldRules& rules,Regions& regions,Landmarks& landmarks,InitialClimateTrace* trace)
        :s(state),g(geometry),r(rules),regions(regions),out(landmarks),t(trace) {
        if(g.width<16 || g.height<8 || g.width>512 || g.height>256 || (g.width&1)
            || size_t(g.width/2)*size_t(g.height)!=s.records.size()) {throw std::invalid_argument("Unsupported landmark generation plane");}
        shaping.on_write=[&](uint32_t off,uint8_t v){Observe(off,1,v);};
        shaping.on_draw=[&](CrtUse,uint32_t before,uint32_t after,uint16_t value){ObserveDraw(before,after,value);};
    }
    void Run(Landmark kind,Coordinate center,uint32_t clock,bool no_name) {
        if(kind==Landmark::Jungle || kind==Landmark::Dunes) {Rain();}
        if(kind==Landmark::Freshwater) {Freshwater(center);return;}
        const auto selected=Select(kind,center);if(!selected) {return;}center=*selected;
        if(kind==Landmark::Jungle) {
            for(const auto& c:Cells(center,121)) {if(std::abs(c.point[1]-center[1])>8) {continue;}
                if(c.offset<21 && Band(c.index)<3) {Shape(c.point,3);}Mark(c.index,4);Detail(c.index,c.offset);
            }
        } else if(kind==Landmark::Crater) {
            for(const auto band:{0,6,3,2,3}) {Shape(center,band);}const auto temp=s.records[Index(center)][0]&7;
            for(const auto& c:Cells(center,49)) {
                Item(c.index,0x100,false);Mark(c.index,1);Detail(c.index,c.offset);
                if(c.offset<21) {Rock(c.index,1);Write(c.index,0,1,uint8_t((s.records[c.index][0]&~7)|temp));}
            }
        } else if(kind==Landmark::Mount) {
            Shape(center,0);Shape(center,6);
            for(const auto& c:Cells(center,9)) {
                Mark(c.index,2);Detail(c.index,c.offset);auto items=Read(s.records[c.index],8)&0xee377e83u;
                if(c.offset==0) {items&=0xf3ffffffu;}Write(c.index,8,4,items|0x100000u);Rock(c.index,2);
            }out.mount_center=center;
        } else if(kind==Landmark::Mesa) {
            for(const auto& c:Cells(center,49)) {Shape(c.point,c.offset<25?4:3);Mark(c.index,0x100);Detail(c.index,c.offset);}
        } else if(kind==Landmark::Ridge) {
            for(const auto i:landmark_tables::RIDGE) {const auto p=Offset(center,i);if(p) {Shape(*p,4);}}
            for(const auto i:landmark_tables::RIDGE) {const auto p=Offset(center,i);if(p) {Mark(Index(*p),0x800);Detail(Index(*p),i);}}
        } else if(kind==Landmark::Uranium) {
            for(const auto& c:Cells(center,21)) {
                if(c.offset>=9 && Band(c.index)<3 && Draw()%3==0) {continue;}
                Shape(c.point,3);Mark(c.index,8);Rock(c.index,0);Item(c.index,0x20,false);Detail(c.index,c.offset);
            }
        } else if(kind==Landmark::Ruins) {
            for(const auto& c:Cells(center,25)) {
                Shape(c.point,3);
                if(Band(c.index)>=3) {Item(c.index,0x100020);if(c.offset==0) {Item(c.index,0x20,false);}else if(c.offset<9) {Item(c.index,0x2000);}}
                Mark(c.index,0x20);Detail(c.index,c.offset);
            }
        } else if(kind==Landmark::Nexus) {
            for(const auto& c:Cells(center,9)) {Shape(c.point,3);Item(c.index,0x100000);Mark(c.index,0x2000);Detail(c.index,c.offset);}
        } else if(kind==Landmark::Boreholes) {
            const auto residue=int(clock&7),orientation=int((clock>>3)&3);int a=residue/3+5,b=3-residue%3,c=8;
            if(orientation&2) {
                --a;--b;if(orientation&1) {++a;++b;c=8;}else {--a;--b;c=6;}
                c=(c+8)%8+1;a=(a+8)%8+1;b=(b+8)%8+1;
            }
            const std::array<int,3> selected_offsets{{c,a,b}};
            for(const auto& cell:Cells(center,9)) {
                Shape(cell.point,3);Item(cell.index,0x100000);
                if(std::find(selected_offsets.begin(),selected_offsets.end(),int(cell.offset))!=selected_offsets.end()) {
                    Item(cell.index,0x1000000);Mark(cell.index,0x1000);Detail(cell.index,cell.offset);
                }
            }
            for(const auto i:selected_offsets) {const auto p=Offset(center,size_t(i));if(!p) {continue;}
                for(const auto& cell:Cells(*p,9)) {if(cell.offset && Band(cell.index)<3) {Shape(cell.point,3);}}
            }Mark(Index(center),0x1000);
        } else if(kind==Landmark::Sargasso) {
            bool pod=false;
            for(const auto& c:Cells(center,81)) {
                if(Band(c.index)>=3 && c.offset>=21) {continue;}Shape(c.point,2);Mark(c.index,0x10);Detail(c.index,c.offset);
                if(c.offset<=4) {Item(c.index,0x10000000);}
                if(!pod && c.offset>=25 && Draw()%24==0) {pod=true;continue;}
                if(c.offset>=49 && Draw()%3==0) {pod=true;continue;}Item(c.index,0x20);
            }
        } else if(kind==Landmark::Dunes) {
            for(const auto& c:Cells(center,25)) {
                if(Band(c.index)<3 && c.offset>=9 && Draw()%3==0) {continue;}
                Shape(c.point,c.offset==2 || c.offset==7?4:3);Item(c.index,0x20,false);Mark(c.index,0x40);Rock(c.index,0);Detail(c.index,c.offset);
            }
        } else if(kind==Landmark::Geothermal) {
            bool pod=false;
            for(const auto& c:Cells(center,25)) {
                if(Band(c.index)>=3 && c.offset>=9) {continue;}
                if(!pod && c.offset && Draw()%25==0) {pod=true;continue;}
                if(c.offset>=9 && Draw()%3==0) {pod=true;continue;}
                Shape(c.point,2);Mark(c.index,0x400);Detail(c.index,c.offset);
            }
        } else {throw std::invalid_argument("Unknown generated landmark");}
        if(!(kind==Landmark::Mount && no_name)) {
            if(kind==Landmark::Mount || kind==Landmark::Mesa || kind==Landmark::Ridge || kind==Landmark::Ruins || kind==Landmark::Dunes) {center[1]+=2;}
            Name(kind,center);
        }
    }
private:
    size_t Index(Coordinate p) const {return size_t(p[1])*size_t(g.width/2)+size_t(p[0]/2);}
    int Band(size_t i) const {return s.records[i][0]>>5;}
    std::optional<Coordinate> Offset(Coordinate p,size_t i) const {
        p[0]+=landmark_tables::OFFSETS[i][0];p[1]+=landmark_tables::OFFSETS[i][1];
        if(s.topology==0) {if(p[0]<0) {p[0]+=g.width;}else if(p[0]>=g.width) {p[0]-=g.width;}}
        if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height) {return std::nullopt;}return p;
    }
    std::vector<Cell> Cells(Coordinate p,size_t count) const {
        std::vector<Cell> result;result.reserve(count);
        for(size_t i=0;i<count;++i) {const auto n=Offset(p,i);if(n) {result.push_back({i,Index(*n),*n});}}return result;
    }
    void Observe(uint32_t off,unsigned size,uint32_t value) {if(t) {++t->stores;if(t->on_store) {t->on_store(off,size,value);}}}
    void Write(size_t i,size_t f,unsigned size,uint32_t value) {
        for(unsigned k=0;k<size;++k) {s.records[i][f+k]=uint8_t(value>>(8*k));}Observe(uint32_t(i*44+f),size,value);
    }
    void ObserveDraw(uint32_t before,uint32_t after,uint16_t value) {if(t) {++t->crt_draws;if(t->on_crt) {t->on_crt(before,after,value);}}}
    uint16_t Draw() {const auto before=s.random.Crt();const auto value=s.random.NextCrtRaw15();ObserveDraw(before,s.random.Crt(),value);return value;}
    void Item(size_t i,uint32_t mask,bool on=true) {Write(i,8,4,on?Read(s.records[i],8)|mask:Read(s.records[i],8)&~mask);}
    void Mark(size_t i,uint32_t mask) {Write(i,12,4,Read(s.records[i],12)|mask);}
    void Detail(size_t i,size_t detail) {Write(i,12,4,(Read(s.records[i],12)&0xffffffu)|(uint32_t(detail&255)<<24));s.dirty_flags|=4;}
    void Rock(size_t i,int rock) {Write(i,5,1,uint8_t((s.records[i][5]&63)|(rock<<6)));Mark(i,0x400000);s.dirty_flags|=1;}
    void Shape(Coordinate p,int category) {
        const ContinentRules rules{r[2],r[3],{{r[25],r[26],r[27],r[28],r[29]}},s.primary_rules};
        ShapeGeneratedTerrain(s,g,rules,p,category,&shaping);
    }
    void Name(Landmark kind,Coordinate p) {if(out.labels.size()<64) {out.labels.push_back({kind,p});}}
    void Rain() {
        smac_climate::World world{g.width,g.height,s.topology==0,{}};world.tiles.reserve(s.records.size());
        for(const auto& a:s.records) {world.tiles.push_back({a[0]>>5,a[0]&7,(a[0]>>3)&3,a[1],a[6],Read(a,8),Read(a,12),a[2]&15,a[5]>>6});}
        const smac_climate::Rules rule{uint32_t(s.resource_pattern),s.reset_words[1],s.categories[4],s.categories[5],r[10],r[11],r[12],r[13],r[15],r[16],r[17]};
        smac_climate::Rainfall(world,rule);s.dirty_flags|=world.dirty_flags;
        for(size_t i=0;i<s.records.size();++i) {
            const auto& a=world.tiles[i];s.records[i][0]=uint8_t((s.records[i][0]&~24)|(a.moisture<<3));s.records[i][6]=uint8_t(a.aux);
            for(unsigned k=0;k<4;++k) {s.records[i][12+k]=uint8_t(a.landmarks>>(8*k));}
        }
    }
    std::optional<Coordinate> Select(Landmark kind,Coordinate p) {
        if(p[0]>=0 && p[0]<g.width && p[1]>=0 && p[1]<g.height) {return p;}
        for(int attempt=1;attempt<=1000;++attempt) {
            if(kind==Landmark::Jungle) {p[1]=g.height/2+Draw()%4-2;p[0]=Draw()%g.width;}
            else if(kind==Landmark::Dunes) {const auto span=g.height/2;p[1]=(span>1?Draw()%span:0)+g.height/2-g.height/4;p[0]=Draw()%g.width;}
            else {const auto span=g.height-16;p[1]=(span>1?Draw()%span:0)+8;p[0]=Draw()%g.width;}
            p[0]^=(p[0]^p[1])&1;if(attempt>=1000) {return {};}
            const auto a=Offset(p,0);if(!a) {continue;}const auto index=Index(*a);
            const bool sea=kind==Landmark::Sargasso || kind==Landmark::Geothermal;
            if((Band(index)<3)!=sea) {continue;}
            if(kind==Landmark::Jungle) {
                bool water=false;int land=0;
                for(const auto& c:Cells(p,121)) {water=water || (c.offset && c.offset<21 && Band(c.index)<3);land+=Band(c.index)>=3;}
                if(!water || land<40 || (s.records[index][0]&24)<=8) {continue;}
            }
            if(kind==Landmark::Dunes && (s.records[index][0]&24)) {continue;}
            bool collision=false;for(const auto& c:Cells(p,289)) {if(s.records[c.index][15]) {collision=true;break;}}
            if(!collision) {return p;}
        }return {};
    }
    void Freshwater(Coordinate p) {
        int chosen=-1;
        if(p[0]>=0 && p[0]<g.width && p[1]>=0 && p[1]<g.height) {const auto i=Index(p);if(Band(i)>=3) {return;}chosen=s.records[i][3];}
        else {int best=0;for(size_t i=64;i<128;++i) {const auto count=regions.Area(i);if(count>=3 && count<=32 && count>=best) {best=count;chosen=int(i);}}}
        if(chosen<0) {return;}int first=-1;bool named=false;
        for(int y=g.height-1;y>=0;--y) {for(int x=y&1;x<g.width;x+=2) {
            const auto i=Index({{x,y}});
            if(s.records[i][3]==chosen) {if(first<0) {first=x;}Mark(i,128);}
            else if(!named && first>=0) {int mid=(x+first-2)/2;mid^=(mid^y)&1;Name(Landmark::Freshwater,{{mid,y}});named=true;}
        }}
    }
    State& s;Geometry g;const WorldRules& r;Regions& regions;Landmarks& out;InitialClimateTrace* t;ContinentTrace shaping;
};
}
void PlaceLandmark(State& s,Geometry g,const WorldRules& r,Regions& regions,Landmarks& out,Landmark kind,Coordinate center,uint32_t clock,bool suppress,InitialClimateTrace* t) {
    Builder(s,g,r,regions,out,t).Run(kind,center,clock,suppress);
}
const std::array<std::string,13>& StockLandmarkNames() {
    static const std::array<std::string,13> names{{"Monsoon Jungle","Garland Crater","Mount Planet","Sunny Mesa","Pholus Ridge","Uranium Flats","The Ruins","Manifold Nexus","Borehole Cluster","New Sargasso","Great Dunes","Freshwater Sea","Geothermal Shallows"}};return names;
}
std::string Landmarks::EncodeNames(const std::array<std::string,13>& names) const {
    if(labels.size()>64) {throw std::invalid_argument("Too many native landmark labels");}std::string result(labels.size()*40,'\0');
    for(size_t i=0;i<labels.size();++i) {
        const auto& label=labels[i];const auto& text=names.at(size_t(label.kind));
        if(text.size()>=32 || text.find('\0')!=std::string::npos) {throw std::invalid_argument("Native landmark name exceeds its slot");}
        for(unsigned coordinate=0;coordinate<2;++coordinate) {for(unsigned k=0;k<4;++k) {result[i*40+coordinate*4+k]=char(uint8_t(uint32_t(label.position[coordinate])>>(8*k)));}}
        std::copy(text.begin(),text.end(),result.begin()+i*40+8);
    }return result;
}
}
