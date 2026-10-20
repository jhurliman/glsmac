#include "SmacClimate.h"
#include "SmacShapingOffsets.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace game::backend::map::smac_climate {
namespace {
void Require(bool condition,const char* message) {if(!condition) {throw std::invalid_argument(message);}}
int32_t Checked(int64_t value) {
    if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("Climate exceeds measured signed32 arithmetic");}
    return static_cast<int32_t>(value);
}
constexpr std::array<int32_t,9> thresholds{{0,20,40,60,80,100,120,140,160}};
constexpr std::array<int32_t,8> river_bounds{{0,20,40,60,80,100,120,140}};
constexpr std::array<Coordinate,4> directions{{{{1,-1}},{{1,1}},{{-1,1}},{{-1,-1}}}};
constexpr const auto& shaping=smac_shaping::OFFSETS;
constexpr const auto& ring_ends=smac_shaping::RING_ENDS;
template<class F> void Scan(World& world,const F& call) {
    for(int32_t y=0;y<world.height;++y) {for(int32_t x=y&1;x<world.width;x+=2) {call(x,y,*world.At(x,y));}}
}
void EnsureContour(World& world,int32_t x,int32_t y,int32_t category,Streams& random,const Context& context) {
    AssignSurfaceClass(world,x,y,category,random,context);auto& tile=*world.At(x,y);
    if(ClassifyContour(tile.contour,context.sea_offset)!=category) {
        const auto span=thresholds[category+1]-thresholds[category];
        const auto residue=span>1?random.Crt()%span:0;
        tile.contour=static_cast<uint8_t>(Checked(int64_t(thresholds[category])+context.sea_offset+residue));
    }
}
void ValidateRules(const World& world,const Rules& rules) {
    Require(rules.temperature>=0 && rules.temperature<=2 && rules.rainfall>=0 && rules.rainfall<=2,
        "Climate requires admitted temperature and rainfall categories");
    Require(rules.latitude_divisor>0 && rules.band_divisor>0 && rules.band_divisor<=world.height
        && rules.jitter_divisor>0 && rules.warming_divisor>0,"Climate requires nonzero admitted divisors and latitude band");
    Require(rules.mountain_shadow>=0 && rules.mountain_shadow<=64 && rules.hill_shadow>=0 && rules.hill_shadow<=64
        && rules.shadow_step>=0 && rules.shadow_step<=64,"Unmeasured climate shadow extent");
}
}
void World::Validate() const {
    Require(width>=16 && width<=512 && !(width&1) && height>=1 && height<=256
        && tiles.size()==size_t(width/2)*height,"Climate requires a complete bounded doubled-coordinate plane");
    for(const auto& t:tiles) {Require(t.elevation>=0 && t.elevation<=7 && t.temperature>=0 && t.temperature<=2
        && t.moisture>=0 && t.moisture<=2 && t.contour>=0 && t.contour<=255 && t.aux>=0 && t.aux<=255
        && t.cached_occupant>=0 && t.cached_occupant<=15 && t.rockiness>=0 && t.rockiness<=2,"Invalid retained climate tile");}
}
Tile* World::At(int32_t x,int32_t y) {
    if(horizontal_wrap && width>0) {x%=width;if(x<0) {x+=width;}}
    if(x<0 || x>=width || y<0 || y>=height) {return nullptr;}
    return &tiles.at(size_t(y)*(width/2)+x/2);
}
const Tile* World::At(int32_t x,int32_t y) const {return const_cast<World*>(this)->At(x,y);}
void Streams::Seed(uint32_t seed) {state.ExchangeShared(seed);if(seed_requests) {seed_requests->push_back(seed);}}
int32_t Streams::Shared(int32_t low,int32_t high) {
    if(low>high) {std::swap(low,high);}const auto span=int64_t(high)-low;
    const auto raw=state.NextSharedLow16();++shared_draws;
    return Checked(int64_t(low)+(int64_t(raw)*span>>16));
}
uint16_t Streams::Crt() {++crt_draws;const auto value=state.NextCrtRaw15();if(crt_values) {crt_values->push_back(value);}return value;}
int32_t ClassifyContour(int32_t contour,int32_t offset) {
    const auto value=Checked(int64_t(contour)-offset);
    for(int32_t category=6;category>=0;--category) {if(value>=thresholds[category]) {return category;}}return 0;
}
void AssignSurfaceClass(World& world,int32_t x,int32_t y,int32_t category,Streams& random,const Context& context) {
    Require(category>=0 && category<=7,"Invalid requested climate class");auto* t=world.At(x,y);Require(t,"Climate class center is outside world");
    const auto old=t->elevation;
    if(context.turn!=0) {
        if(old!=category && t->landmarks) {
            const auto prior=t->landmarks;t->landmarks&=~0x80u;const auto detail=static_cast<uint8_t>(t->landmarks>>24);
            if(detail<128) {
                if(context.invalidate_landmark) {context.invalidate_landmark(prior,detail);}
                t->landmarks=(t->landmarks&0xffffffu)|(uint32_t(static_cast<uint8_t>(~detail))<<24);world.dirty_flags|=4;
            }
        }
        if((old<3)!=(category<3)) {
            t->items&=0x7e1377a3u;t->remembered_items.fill(t->items);
            if(!context.terrain_only) {
                if(context.entity_change) {context.entity_change(world,x,y,old,category,random);}
                else {Require(!(t->items&3u) || t->cached_occupant>=8,"Occupied climate conversion requires actual Base/Unit coupling");}
            }
        } else if(category>=3) {
            const auto rock=t->rockiness;
            if(category>old && rock<2 && rock+1<=category-3) {
                if(random.Crt()%(rock+4)==0) {++t->rockiness;t->landmarks|=0x400000u;world.dirty_flags|=1;}
            } else if(category<old && rock>0 && rock>category-3) {
                if(random.Crt()%(rock+1)!=0) {--t->rockiness;t->landmarks|=0x400000u;world.dirty_flags|=1;}
            }
        }
    }
    t->elevation=category;world.dirty_flags|=4;
}
void ShapeHeight(World& world,int32_t x,int32_t y,int32_t category,Streams& random,const Context& context) {
    Require(category>=0 && category<=7,"Invalid shaping class");world.flood_counts.fill(0);world.affected_bases.fill(-1);
    if(world.At(x,y)) {EnsureContour(world,x,y,category,random,context);}
    for(const auto direction:{-1,1}) {const auto maximum=direction<0?category:8-category;
        for(int32_t radius=1;radius<maximum;++radius) {const auto target=category+direction*radius;bool changed=false;
            for(size_t i=ring_ends[radius-1];i<ring_ends[radius];++i) {
                const auto nx=x+shaping[i][0],ny=y+shaping[i][1];auto* tile=world.At(nx,ny);if(!tile) {continue;}
                if(direction<0?tile->elevation<target:tile->elevation>target) {
                    EnsureContour(world,nx,ny,target,random,context);
                    if(!(tile->items&3u) || tile->cached_occupant>=8) {
                        Require(bool(context.resource_bonus),"Shaping needs actual natural-resource authority");
                        if(!context.resource_bonus(world,nx,ny)) {tile->cached_occupant=15;}
                    }
                    changed=true;
                }
            }
            if(!changed) {break;}
        }
    }
}
void RecalculateAltitudes(World& world,Streams& random,const Context& context) {
    world.Validate();Scan(world,[&](int32_t x,int32_t y,Tile& tile){AssignSurfaceClass(world,x,y,ClassifyContour(tile.contour,context.sea_offset),random,context);});
    for(int32_t category=6;category>=2;--category) {
        Scan(world,[&](int32_t x,int32_t y,Tile& tile){if(tile.elevation==category) {ShapeHeight(world,x,y,category,random,context);}});
    }
}
void EcologyVolcano(World& world,int32_t x,int32_t y,Streams& random,const Context& context) {
    world.Validate();Require(world.At(x,y),"Volcano needs an admitted center");
    ShapeHeight(world,x,y,0,random,context);ShapeHeight(world,x,y,6,random,context);
    for(size_t i=0;i<9;++i) {auto* tile=world.At(x+shaping[i][0],y+shaping[i][1]);if(!tile) {continue;}
        tile->landmarks=((tile->landmarks|2u)&0xffffffu)|(uint32_t(i)<<24);world.dirty_flags|=4;
        tile->items&=0xee377e83u;if(i==0) {tile->items&=0xf3ffffffu;}
        tile->items|=0x100000u;tile->rockiness=2;tile->landmarks|=0x400000u;world.dirty_flags|=1;
    }
    world.mount_planet=Coordinate{x,y};
}
void Temperature(World& world,const Rules& rules,Streams& random) {
    world.Validate();ValidateRules(world,rules);const auto h=world.height,jitter=h/rules.jitter_divisor,category=rules.temperature;
    random.Seed(rules.pattern+17u);
    Scan(world,[&](int32_t x,int32_t y,Tile& tile) {
        const auto lm=tile.landmarks;if((lm&0x80000001u)==1u && (lm&0xff000000u)<0x15000000u) {return;}
        const auto r1=random.Shared(0,2*jitter),r2=random.Shared(0,category+1);
        const auto latitude=Checked(int64_t(std::abs(h/2-r1-y+jitter))-(category-1)*(h/rules.latitude_divisor)
            -Checked(int64_t(rules.extra)*(h/rules.warming_divisor)));
        const auto span=h/rules.band_divisor,index=Checked(int64_t(span/2)+2*int64_t(latitude))/span;
        auto value=index<=2?3:index<=9?2:1;const auto alt=tile.elevation;
        for(size_t i=0;i<9;++i) {const auto* n=world.At(x+shaping[i][0],y+shaping[i][1]);
            if(n && (n->items&0x1000000u) && (alt<=4 || n->elevation>=alt)) {++value;break;}}
        if(value>1) {
            if(alt>5) {if(category<2 || random.Shared(0,2)==0) {--value;}}
            else if(alt>4 && r2==0) {--value;}
        }
        tile.temperature=std::clamp(value-1,0,2);
    });
}
void Rivers(World& world,Streams& random) {
    world.Validate();for(auto& t:world.tiles) {t.items&=~0x280u;}
    Scan(world,[&](int32_t sx,int32_t sy,Tile& source) {
        if(!(source.items&0x100u) || source.elevation<3) {return;}
        for(auto& t:world.tiles) {t.landmarks&=~0x800000u;}
        random.Seed(static_cast<uint32_t>(47*sy+31*sx));auto direction=random.Shared(0,4),x=sx,y=sy,band=source.elevation,height=source.contour;
        source.landmarks|=0x800000u;bool finish=true;int32_t cycle_previous=0,steps=0,length=1,same_band=1;
        while(true) {
            if(world.At(x,y)->items&0x1000000u) {finish=false;break;}
            const auto preferred=(random.Shared(0,2)+direction-(length&1))&3,back=direction^2;
            struct Candidate {int32_t direction,weight,cycle;};std::vector<Candidate> candidates;int32_t minimum_uphill=9999,uphill_weight=8;bool terminated=false;
            for(int32_t d=0;d<4;++d) {
                if(d==back) {continue;}const auto nx=x+directions[d][0],ny=y+directions[d][1];auto* n=world.At(nx,ny);if(!n) {continue;}
                if(n->items&0x80u) {terminated=true;break;}
                if(n->items&0x100u) {int32_t crowd=0;for(const auto& delta:directions) {const auto* a=world.At(nx+delta[0],ny+delta[1]);if(a && (a->items&0x100u)) {++crowd;}}
                    if(crowd>1) {terminated=true;break;}}
                if(n->elevation>band) {continue;}const auto nh=n->contour;
                auto weight=std::clamp(height-nh+2,1,9999)*32+(d==preferred?31:0);
                if(nh>height) {if(nh>minimum_uphill) {continue;}minimum_uphill=nh;weight=uphill_weight++;}
                int32_t higher=0,cycle=0;bool reject=false;
                for(int32_t k=0;k<4;++k) {
                    if(k==(d^2)) {continue;}const auto* a=world.At(nx+directions[k][0],ny+directions[k][1]);if(!a) {continue;}
                    if(a->elevation<3 && n->elevation>=3 && same_band<5) {weight/=2;}
                    if(a->contour>nh) {++higher;if(a->items&0x80u) {reject=true;break;}}
                    if(a->landmarks&0x800000u) {if(cycle_previous || a->elevation<3) {reject=true;break;}weight/=8;cycle=1;}
                }
                if(reject || !weight) {continue;}if(higher==3) {weight=1;}candidates.push_back({d,weight,cycle});
            }
            if(terminated) {finish=false;break;}if(candidates.empty()) {break;}
            int32_t total=0;for(const auto& c:candidates) {total+=c.weight;}auto value=random.Shared(0,total);auto chosen=candidates.back();
            for(const auto& c:candidates) {if(value<c.weight) {chosen=c;break;}value-=c.weight;}
            direction=chosen.direction;cycle_previous=chosen.cycle;x+=directions[direction][0];y+=directions[direction][1];
            if(world.horizontal_wrap) {x=(x%world.width+world.width)%world.width;}
            auto& tile=*world.At(x,y);const auto next_band=tile.elevation;if(next_band!=band) {same_band=0;}band=next_band;
            auto nh=tile.contour;if(nh>height) {nh=std::max(height-1,river_bounds[band]);tile.contour=static_cast<uint8_t>(nh);}
            height=nh;if(++steps>=1000) {break;}tile.landmarks|=0x800000u;++length;++same_band;
        }
        auto& last=*world.At(x,y);
        if(band>=3 && !(last.items&0x80u) && finish && !((last.items&3u) && last.cached_occupant<8)) {last.items|=0x200u;}
        for(auto& tile:world.tiles) {if(tile.landmarks&0x800000u) {tile.items|=0x80u;}}
    });
    for(auto& tile:world.tiles) {tile.landmarks&=~0x800000u;}
}
void Rainfall(World& world,const Rules& rules) {
    world.Validate();ValidateRules(world,rules);const auto rain=rules.rainfall,shadow=rules.shadow_step;
    for(auto& tile:world.tiles) {tile.aux=0;}
    const auto mark=[&](int32_t x,int32_t y,int32_t mask){if(auto* tile=world.At(x,y)) {tile->aux|=mask;}};
    Scan(world,[&](int32_t x,int32_t y,Tile& tile) {
        const auto band=tile.elevation;
        if(tile.items&0x400000u) {for(size_t i=0;i<9;++i) {if(auto* n=world.At(x+shaping[i][0],y+shaping[i][1])) {n->aux|=(n->aux&1)?4:1;}}}
        if(band<3) {
            for(int32_t k=0;k<3;++k) {auto* a=world.At(x-2*(k+1),y);const auto* b=world.At(x-2*k,y);if(a && b && a->elevation>=b->elevation && k<=rain) {a->aux&=~16;}}
            mark(x+2,y,128);
            if(rain>0) {auto* n=world.At(x+4,y);if(n && (n->temperature>1 || rain>1)) {
                n->aux|=128;if(rain>1) {if(auto* a=world.At(x+6,y);a && a->temperature) {a->aux|=128;}}}}
        }
        if(band<=4) {if(band>=3 && (tile.items&0x200000u)) {for(const auto dy:{-1,0,1}) {mark(x+1,y+dy,64);}mark(x+2,y,64);}}
        else {const auto extent=band>5?rules.mountain_shadow:rules.hill_shadow,variation=(rain-1)*shadow;
            for(const auto direction:{-1,1}) {const auto limit=extent-direction*variation,mask=direction<0?32:16;
                for(int32_t distance=1;distance<=limit;++distance) {auto* n=world.At(x+direction*2*distance,y);if(!n) {continue;}
                    if(n->elevation<3 || n->elevation>=band) {break;}n->aux|=mask;}}}
    });
    int32_t wet=0,high=0,low=0;
    Scan(world,[&](int32_t x,int32_t y,Tile& tile) {
        const auto band=tile.elevation,temp=tile.temperature,f=tile.aux;const auto lm=tile.landmarks;int32_t result=0,adjust=0;
        if(y==0 || y==world.height-1) {tile.moisture=0;return;}
        if((lm&0x80000040u)==0x40u || (lm&0x80000100u)==0x100u) {wet=1;}
        if(f&32) {result=1;low=0;wet=1;}
        if(f&128) {wet=temp>1;high=0;low=0;if(rain>=1) {adjust=1;}}
        if(f&16) {if(wet) {--wet;}--adjust;}
        if((f&64) && band<=4) {high=0;low=0;adjust=std::min(0,adjust);}
        if(band>3) {++high;if(high>4*shadow*rain+1 && wet) {--wet;}}
        else if(band<3) {low=0;wet=1;}
        if(band==3 && wet && high==0) {if(++low>4*rain+2) {wet=0;low=0;}}
        if(band<=3 && high) {high-=1+rain;if(high<1) {high=0;low=0;if(rain!=0 || band<3) {wet=1;}}}
        if(tile.items&0x80u) {if((x/2)%3 && adjust<0) {adjust=0;}wet=1;low=0;}
        if(f&2) {--result;wet=0;}else if(wet) {adjust+=wet;}
        if(band<=4+int32_t(temp!=0)) {result+=adjust;}else if(rain-band+temp+5>=2 && adjust>0) {++result;}
        if((lm&0x80000004u)==4u && band>=3 && band<6) {
            const auto byte=static_cast<int32_t>(lm>>24);const auto detail=std::abs(byte<128?byte:byte-256);
            result+=(detail<49 || (detail<81 && band<5))?2:1;
        }
        result=(lm&0x80000040u)==0x40u?0:std::clamp(result,0,2);
        for(const auto flag:{1,4}) {if(f&flag) {wet=1;result=std::min(2,result+1);}}
        tile.moisture=result;
    });
    Scan(world,[&](int32_t x,int32_t y,Tile& tile) {
        if(tile.moisture) {return;}for(size_t i=1;i<9;++i) {const auto* n=world.At(x+shaping[i][0],y+shaping[i][1]);
            if(n && n->elevation>=3 && n->moisture==2) {tile.moisture=1;tile.landmarks|=0x400000u;world.dirty_flags|=1;break;}}
    });
}
void Recalculate(World& world,const Rules& rules,Streams& random,const Context& context) {
    world.Validate();ValidateRules(world,rules);RecalculateAltitudes(world,random,context);
    Temperature(world,rules,random);Rivers(world,random);Rainfall(world,rules);
}
}
