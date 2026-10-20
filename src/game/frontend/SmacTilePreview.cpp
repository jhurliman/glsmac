#include "SmacTilePreview.h"
#include "SmacTerrainAtlas.h"
#include "SmacDevelopedOverlay.h"
#include "SmacRoadTube.h"
#include "SmacResourceArt.h"
#include <map>

namespace game::frontend::smac_tile_preview {
namespace raster=smac_terrain_raster;
namespace material=smac_land_material;
namespace {
// The native retained-sprite walker at85/100: strict-positive midpoint ties.
raster::Bitmap Scale(const raster::Bitmap& input) {
    const int w=input.width*85/100,h=input.height*85/100;
    raster::Bitmap out{w,h,std::vector<uint8_t>(size_t(w)*size_t(h),9)};
    const auto positions=[](int count) {
        std::vector<int> result;result.reserve(count);int position=0,error=15-170+100;
        for(int i=0;i<count;++i) {
            result.push_back(position);error+=30;int step=1;
            if(error>0) {++step;error-=170;}position+=step;
        }
        return result;
    };
    const auto xs=positions(w),ys=positions(h);
    for(int y=0;y<h;++y) {for(int x=0;x<w;++x) {
        out.pixels[size_t(y)*size_t(w)+size_t(x)]=input.pixels.at(size_t(ys[y])*size_t(input.width)+size_t(xs[x]));
    }}
    return out;
}
}
std::optional<native_terrain::Frame> Paint(const ::game::native_terrain::World& world,
    const native_terrain::Source& source,int x,int y,uint16_t base,
    const raster::Bitmap& backing,raster::Point anchor,raster::Clip clip) {
    if(!world.ValidDimensions()) {return {};}
    for(const auto value:anchor) {if(value < -32000 || value > 32000) {throw std::invalid_argument("Unsupported preview anchor");}}
    const auto at=[&](int tx,int ty)->const ::game::native_terrain::Tile* {
        if(ty<0 || ty>=world.height || ((tx^ty)&1)) {return nullptr;}
        if(world.horizontal_wrap) {tx=(tx%world.width+world.width)%world.width;}
        if(tx<0 || tx>=world.width) {return nullptr;}return &world.tiles[size_t(ty)*size_t(world.width/2)+size_t(tx)/2];
    };
    if(x<0 || x>=world.width || y<0 || y>=world.height || ((x^y)&1)) {return {};}
    const auto& own=*at(x,y);
    // Current original controls close bare/rocky/moist land, remembered Road
    // and Fungus, and live resource sprites. Other feature painters need their
    // own preview-scale receipts; preserve the existing working fallback.
    constexpr uint32_t admitted_items=0x4u|0x20u|0x400u|0x20000000u;
    // Upper-byte detail and0x400000 dirty bookkeeping are present even on
    // ordinary Planet tiles. They do not select independent landmark art.
    constexpr uint32_t semantic_memberships=~0xff400000u;
    if(!own.selected_known || own.band<3 || (own.landmarks&semantic_memberships) ||
        (own.selected_items&~admitted_items) || !own.resource_ready) {return {};}
    const material::TileAt materials=[&](int tx,int ty)->std::optional<material::Tile> {
        const auto* cell=at(tx,ty);if(!cell) {return {};}
        // Center admission/memory is separate from the original selectors'
        // live neighbor climate and Fungus connectivity reads.
        return material::Tile{cell->band,cell->moisture,cell->rockiness,cell->items};
    };
    const auto passes=material::Select({{x,y}},materials,static_cast<uint16_t>(base+44*(40*y+x/2)),own.selected_items);
    const std::array<raster::Point,5> points{{{{anchor[0]+40,anchor[1]+20}},
        {{anchor[0],anchor[1]+20}},{{anchor[0]+40,anchor[1]}},
        {{anchor[0]+80,anchor[1]+20}},{{anchor[0]+40,anchor[1]+40}}}};
    raster::AtlasCrops crops(source.loaded);
    std::vector<raster::Command> commands;
    for(const auto& pass:passes) {
        raster::Command command;command.texture=crops.Get(pass.crop);command.polygon.count=pass.vertices;
        command.shader=pass.role==material::Role::Base?raster::Shader::OpaqueShaded:
            pass.role==material::Role::Fungus?raster::Shader::KeyedUnlit:raster::Shader::KeyedShaded;
        for(size_t v=0;v<pass.vertices;++v) {
            const auto index=pass.fan<0?v+1:v==0?0:1+(size_t(pass.fan)+v-1)%4;
            command.polygon.vertices[v]={points[index],pass.uv[v],0};
        }
        commands.push_back(command);
    }
    bool complete=true; // Road connectivity keeps its separate remembered-item authority.
    const smac_road_tube::CellAt roads=[&](int tx,int ty)->std::optional<smac_road_tube::Cell> {
        const auto* cell=at(tx,ty);if(!cell) {return {};}
        if(!cell->selected_known) {complete=false;return {};}
        return smac_road_tube::Cell{cell->band,cell->selected_items,cell->admitted_base};
    };
    for(const auto& pass:smac_road_tube::Select(x,y,points,{},roads,0x621)) {
        raster::Command command;command.texture=crops.Get(pass.crop);command.polygon=pass.polygon;
        command.shader=pass.shader;command.shade_limits=pass.shade_limits;commands.push_back(command);
    }
    if(!complete) {return {};}
    std::map<material::Crop,raster::Bitmap> sprites;
    for(const auto& pick:smac_resource_art::Select(x,y,own.band,own.resource_items,world.resource_pattern,world.automatic_land_resources,0x621)) {
        if(source.ter1.pixels.empty()) {return {};}
        const auto decoded=smac_developed_overlay::Decode(source.ter1,pick.crop);
        if(decoded.bitmap.pixels.empty()) {continue;}
        const auto sprite=sprites.emplace(pick.crop,Scale(decoded.bitmap)).first;
        if(sprite->second.pixels.empty()) {continue;} // native floor-to-zero extent emits no stores
        raster::Command command;command.texture=&sprite->second;
        command.sprite_origin={{anchor[0]+decoded.offset[0]*85/100,anchor[1]+decoded.offset[1]*85/100}};
        command.shader=pick.shadow?raster::Shader::SpriteShadow:raster::Shader::Sprite;
        command.shadow_lookup=&source.shadow_lookup;commands.push_back(command);
    }
    auto result=raster::DrawBatch(backing,commands,clip,true);
    native_terrain::Frame frame;frame.dimensions={{backing.width,backing.height}};frame.world={{world.width,world.height}};
    frame.developed_tiles.assign(world.tiles.size(),false);frame.landmark_tiles.assign(world.tiles.size(),false);frame.resource_tiles.assign(world.tiles.size(),false);
    frame.indexed=std::move(result.bitmap.pixels);frame.coverage=std::move(result.coverage);
    frame.rgba=material::ResolveRgba(frame.indexed,source.palette);
    for(size_t i=0;i<frame.coverage.size();++i) {frame.rgba[4*i+3]=frame.coverage[i]?255:0;}
    frame.attempts=result.attempts;frame.writes=result.writes;frame.tiles=1;return frame;
}
}
