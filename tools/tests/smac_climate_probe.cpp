// Read-only detached climate probe. Input/output are semantic numeric fields;
// no original assets or executable bytes are distributed with this driver.
#include "game/backend/map/SmacClimate.h"
#include "game/backend/map/SmacPlanetMap.h"
#include <iostream>
#include <string>
using namespace game::backend;
namespace c=game::backend::map::smac_climate;
int main() {
    try {
        std::string mode;c::World world{};c::Rules r{};int32_t sea,turn,only,sx,sy,category;uint32_t shared,crt;
        if(!(std::cin>>mode>>world.width>>world.height>>world.horizontal_wrap>>r.pattern>>r.extra>>r.temperature>>r.rainfall
            >>r.latitude_divisor>>r.band_divisor>>r.jitter_divisor>>r.warming_divisor>>r.mountain_shadow>>r.hill_shadow>>r.shadow_step
            >>sea>>turn>>only>>shared>>crt>>sx>>sy>>category)) {throw std::runtime_error("Missing climate probe header");}
        if(world.width<16 || world.width>512 || world.height<1 || world.height>256) {throw std::runtime_error("Invalid probe geometry");}
        world.tiles.resize(size_t(world.width/2)*world.height);
        for(auto& t:world.tiles) {
            if(!(std::cin>>t.elevation>>t.temperature>>t.moisture>>t.contour>>t.aux>>t.items>>t.landmarks>>t.cached_occupant>>t.rockiness)) {throw std::runtime_error("Missing tile");}
            for(auto& word:t.remembered_items) {if(!(std::cin>>word)) {throw std::runtime_error("Missing memory");}}
        }
        world.Validate();SmacGameplayRandomState random(shared,crt);c::Streams streams{random};std::vector<uint32_t> seeds;std::vector<uint16_t> draws;
        streams.seed_requests=&seeds;streams.crt_values=&draws;
        c::Context context{sea,turn,bool(only),[&](const c::World& w,int32_t x,int32_t y) {
            // Declared no-automatic-land-bonus source profile used by these controls.
            const auto* t=w.At(x,y);const auto band=t->elevation;const auto items=t->items;const bool forced=items&0x400u;
            if(!forced && (r.pattern==0 || band>=3)) {return false;}
            const auto floor=[](int32_t v,int32_t d) {return v/d-(v%d<0);};
            const auto v=floor(x-y,2),u=floor(x+y,2),phase=(u&3)+4*(v&3);
            if(!forced && phase!=((-5*floor(u,4)-3*floor(v,4)+r.pattern)&15u)) {return false;}
            return band>=2; // every admitted band returns one nonzero resource type
        },{},{} };
        if(mode=="temperature") {c::Temperature(world,r,streams);}
        else if(mode=="rivers") {c::Rivers(world,streams);}
        else if(mode=="rainfall") {c::Rainfall(world,r);}
        else if(mode=="altitude") {c::RecalculateAltitudes(world,streams,context);}
        else if(mode=="shape") {c::ShapeHeight(world,sx,sy,category,streams,context);}
        else if(mode=="volcano") {c::EcologyVolcano(world,sx,sy,streams,context);}
        else if(mode=="complete") {c::Recalculate(world,r,streams,context);}
        else {throw std::runtime_error("Unknown climate probe mode");}
        std::cout<<random.Shared()<<' '<<random.Crt()<<' '<<streams.shared_draws<<' '<<streams.crt_draws<<' '<<world.dirty_flags<<'\n';
        std::cout<<seeds.size();for(const auto v:seeds) {std::cout<<' '<<v;}std::cout<<'\n';
        std::cout<<draws.size();for(const auto v:draws) {std::cout<<' '<<v;}std::cout<<'\n';
        // Exercise the production container publication rather than validating
        // only the probe's independent numeric encoder.
        std::string container(map::SmacPlanetMap::FILE_BYTES,'\0');container.replace(0,11,"TERRANMAP\0\x1a",11);
        const auto put=[&](size_t at,uint32_t value) {for(size_t i=0;i<4;++i) {container[at+i]=static_cast<char>(value>>(8*i));}};
        put(11,5);put(15,80);put(19,80);
        const auto stored=world.width==80 && world.height==80
            ? map::SmacPlanetMap::Decode(container).WithClimateTerrain(world,sea,r.extra).ExportClimateTerrain(world.horizontal_wrap):world;
        for(const auto& t:stored.tiles) {
            std::cout<<t.elevation<<' '<<t.temperature<<' '<<t.moisture<<' '<<t.contour<<' '<<t.aux<<' '<<t.items<<' '<<t.landmarks<<' '<<t.cached_occupant<<' '<<t.rockiness;
            for(const auto v:t.remembered_items) {std::cout<<' '<<v;}std::cout<<'\n';
        }
    }catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
