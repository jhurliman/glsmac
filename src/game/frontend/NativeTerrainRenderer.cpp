#include "NativeTerrainRenderer.h"
#include "SmacWaterCoast.h"
#include "SmacRiver.h"
#include "SmacRoadTube.h"
#include "SmacDevelopedOverlay.h"
#include "game/backend/map/SmacTerrainGeometry.h"
#include "game/backend/map/SmacTerrainLighting.h"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace game::frontend::native_terrain {
namespace {
namespace raster=smac_terrain_raster;
namespace material=smac_land_material;
namespace terrain=backend::map::smac_terrain;
using World=::game::native_terrain::World;
const ::game::native_terrain::Tile* At(const World& world,int x,int y) {
    if(y<0 || y>=80 || (x-y)%2) {return nullptr;}
    if(world.horizontal_wrap) {x=(x%80+80)%80;}
    if(x<0 || x>=80) {return nullptr;}
    return &world.tiles[static_cast<size_t>(y)*40+static_cast<size_t>(x)/2];
}
}
std::vector<Placement> View(int x,int y,bool wrap) {
    if(x<0 || x>=80 || y<0 || y>=80) {throw std::invalid_argument("Native terrain view requires80x80 camera coordinates");}
    const int first_y=std::clamp(y-12,-2,60);
    int first_x=x-11;
    if(wrap) {if(first_x<0) {first_x+=80;}else if(first_x>=80) {first_x-=80;}}
    else {first_x=std::clamp(first_x,-4,62);}
    // Avoid a host-dependent signed bitwise parity repair at the flat edge.
    first_x=(first_x%2<0?first_x-1:first_x-first_x%2)+(first_y%2==0?0:1);
    std::vector<Placement> result;result.reserve(288);
    for(int row=0;row<25;++row) {for(int col=0;col<(row&1?11:12);++col) {
        const int unwrapped=first_x+(row&1)+2*col;
        int tx=unwrapped;const int ty=first_y+row;
        if(wrap) {if(tx<0) {tx+=80;}else if(tx>=80) {tx-=80;}}
        if(tx<0 || tx>=80 || ty<0 || ty>=80) {continue;}
        result.push_back({{{tx,ty}},{{48*(unwrapped-first_x),24*row}},static_cast<uint32_t>(44*(40*ty+tx/2))});
    }}
    return result;
}

Frame Render(const World& world,const Source& source,const std::vector<Placement>& placements,uint16_t base,Point offset,uint32_t display_flags) {
    if(source.loaded.width<=0 || source.loaded.height<=0 ||
        uint64_t(source.loaded.width)*static_cast<uint32_t>(source.loaded.height)!=source.loaded.pixels.size()) {
        throw std::invalid_argument("Invalid indexed terrain source");
    }
    if(offset[0]<-4096 || offset[0]>4096 || offset[1]<-4096 || offset[1]>4096 || placements.size()>3200) {
        throw std::invalid_argument("Unsupported native terrain view offset or size");
    }
    const terrain::BandAt bands=[&](int x,int y)->std::optional<uint8_t> {
        const auto* tile=At(world,x,y);return tile?std::optional<uint8_t>{tile->band}:std::nullopt;
    };
    const material::TileAt materials=[&](int x,int y)->std::optional<material::Tile> {
        const auto* tile=At(world,x,y);if(!tile) {return {};}
        return material::Tile{tile->band,tile->moisture,tile->rockiness,tile->selected_known?tile->selected_items:tile->items};
    };
    const smac_water_coast::CellAt cells=[&](int x,int y)->std::optional<smac_water_coast::Cell> {
        const auto* tile=At(world,x,y);return tile?std::optional<smac_water_coast::Cell>{{tile->band,tile->contour,tile->items}}:std::nullopt;
    };
    const smac_territory_boundary::CellAt territories=[&](int x,int y)->std::optional<smac_territory_boundary::Cell> {
        const auto* tile=At(world,x,y);return tile?std::optional<smac_territory_boundary::Cell>{{tile->band,tile->region,tile->owner}}:std::nullopt;
    };
    const smac_road_tube::CellAt roads=[&](int x,int y)->std::optional<smac_road_tube::Cell> {
        const auto* tile=At(world,x,y);return tile?std::optional<smac_road_tube::Cell>{{tile->band,tile->selected_items,tile->admitted_base}}:std::nullopt;
    };
    // This opt-in inspector reveals terrain; ordinary game fog is not inferred
    // from explored memory. Owner relations still use the actual local viewer.
    const smac_territory_boundary::Context boundary_context{world.viewer,world.relations};
    std::map<material::Crop,raster::Bitmap> crops;
    const auto crop=[&](const material::Crop& rect)->const raster::Bitmap* {
        auto found=crops.find(rect);
        if(found==crops.end()) {
            const auto x=rect[0],y=rect[1],width=rect[2],height=rect[3];
            if(x<0 || y<0 || width<=0 || height<=0 || width>source.loaded.width || height>source.loaded.height ||
                x>source.loaded.width-width || y>source.loaded.height-height) {throw std::invalid_argument("Native terrain crop outside indexed atlas");}
            raster::Bitmap pixels{width,height,std::vector<uint8_t>(static_cast<size_t>(width)*static_cast<size_t>(height))};
            for(int row=0;row<height;++row) {
                std::copy_n(source.loaded.pixels.begin()+static_cast<size_t>(y+row)*static_cast<size_t>(source.loaded.width)+static_cast<size_t>(x),width,
                    pixels.pixels.begin()+static_cast<size_t>(row)*static_cast<size_t>(width));
            }
            found=crops.emplace(rect,std::move(pixels)).first;
        }
        return &found->second;
    };
    Frame frame;
    std::map<material::Crop,smac_developed_overlay::Sprite> sprites;
    std::vector<raster::Command> commands;commands.reserve(placements.size()*14);
    const auto append_water=[&](int x,int y,const std::array<Point,5>& points) {
        for(const auto& pass:smac_water_coast::Select(x,y,points,cells)) {
            raster::Command command;command.texture=crop(pass.crop);command.polygon=pass.polygon;
            command.shader=pass.shader;command.shade_limits=pass.shade_limits;
            if(pass.shader==raster::Shader::CoastMapped) {command.mask=&source.coast_mask;command.mask_mapping=pass.mapping;}
            commands.push_back(command);
        }
    };
    const auto append_boundary=[&](int x,int y,const std::array<Point,5>& points) {
        if(world.viewer<=0) {return;}
        const auto pass=smac_territory_boundary::Select(x,y,points,territories,boundary_context,display_flags);
        if(!pass) {return;}
        raster::Command command;command.texture=&source.boundary_mask;command.polygon=pass->polygon;
        command.shader=raster::Shader::OwnerClut;command.shade_limits=pass->shade_limits;
        command.owner_clut=&source.owner_clut;command.owner_mapping=pass->mapping;commands.push_back(command);
    };
    const auto append_developed=[&](const ::game::native_terrain::Tile& tile,const Point& anchor,bool kelp) {
        if(!tile.developed_ready || source.ter1.pixels.empty()) {return;}
        for(const auto& pick:smac_developed_overlay::Select(tile.selected_items,tile.band,tile.nutrients,tile.landmarks,display_flags)) {
            const bool is_kelp=tile.band<3 && pick.crop==material::Crop{{607,190,100,62}};
            if(is_kelp!=kelp) {continue;}
            auto found=sprites.find(pick.crop);
            if(found==sprites.end()) {found=sprites.emplace(pick.crop,smac_developed_overlay::Decode(source.ter1,pick.crop)).first;}
            const auto& sprite=found->second;if(sprite.bitmap.pixels.empty()) {continue;}
            raster::Command command;command.texture=&sprite.bitmap;
            command.sprite_origin={anchor[0]+sprite.offset[0],anchor[1]+sprite.offset[1]};
            command.shader=pick.shadow?raster::Shader::SpriteShadow:raster::Shader::Sprite;
            command.shadow_lookup=&source.shadow_lookup;commands.push_back(command);
        }
    };
    for(const auto& placement:placements) {
        const auto x=placement.tile[0],y=placement.tile[1];
        if(x<0 || x>=80 || y<0 || y>=80 || ((x^y)&1) ||
            placement.record_offset!=static_cast<uint32_t>(44*(40*y+x/2)) ||
            placement.flat_center[0]<-4096 || placement.flat_center[0]>4096 ||
            placement.flat_center[1]<-4096 || placement.flat_center[1]>4096) {throw std::invalid_argument("Invalid native terrain placement");}
        const auto& tile=*At(world,x,y);
        if(!tile.ordinary) {continue;}
        const auto fx=placement.flat_center[0]+offset[0],fy=placement.flat_center[1]+offset[1];
        if(tile.band<3) {
            const std::array<Point,5> points{{{{fx,fy}},{{fx-48,fy}},{{fx,fy-24}},{{fx+48,fy}},{{fx,fy+24}}}};
            append_water(x,y,points);append_boundary(x,y,points);
            append_developed(tile,{{fx-48,fy-24}},true);append_developed(tile,{{fx-48,fy-24}},false);
            frame.developed_tiles[static_cast<size_t>(y)*40+x/2]=tile.developed_ready && !source.ter1.pixels.empty();
            ++frame.tiles;continue;
        }
        // Neighbor landmark deformation has not been translated. Do not feed
        // an ordinary lighting/height model through an unsupported stencil.
        bool ordinary=true;
        for(int dy=-4;dy<=4;++dy) {for(int dx=-4;dx<=4;++dx) {if((dx-dy)%2) {continue;}
            const auto* neighbor=At(world,x+dx,y+dy);
            if(neighbor && neighbor->band>=3 && neighbor->deformed) {ordinary=false;}
        }}
        if(!ordinary) {continue;}
        const auto light=terrain::OrdinaryLighting(x,y,bands);if(!light) {continue;}
        const auto heights=terrain::OrdinaryHeights(x,y,bands);
        const std::array<Point,5> points{{{{fx,fy-heights[0]}},{{fx-48,fy-heights[1]}},
            {{fx,fy-24-heights[2]}},{{fx+48,fy-heights[3]}},{{fx,fy+24-heights[4]}}}};
        const auto seed=static_cast<uint16_t>(base+placement.record_offset);
        for(const auto& pass:material::Select({{x,y}},materials,seed)) {
            raster::Command command;command.texture=crop(pass.crop);command.polygon.count=pass.vertices;
            command.shader=pass.role==material::Role::Base?raster::Shader::OpaqueShaded:
                pass.role==material::Role::Fungus?raster::Shader::KeyedUnlit:raster::Shader::KeyedShaded;
            for(size_t v=0;v<pass.vertices;++v) {
                const auto index=pass.fan<0?v+1:v==0?0:1+(static_cast<size_t>(pass.fan)+v-1)%4;
                command.polygon.vertices[v]={points[index],pass.uv[v],(*light)[index]};
            }
            commands.push_back(command);
        }
        append_boundary(x,y,points);
        append_water(x,y,points);
        for(const auto& pass:smac_river::Select(x,y,points,*light,cells,display_flags)) {
            raster::Command command;command.texture=crop(pass.crop);command.polygon=pass.polygon;
            command.shader=pass.shader;command.shade_limits=pass.shade_limits;commands.push_back(command);
        }
        for(const auto& pass:smac_road_tube::Select(x,y,points,*light,roads,display_flags)) {
            raster::Command command;command.texture=crop(pass.crop);command.polygon=pass.polygon;
            command.shader=pass.shader;command.shade_limits=pass.shade_limits;commands.push_back(command);
        }
        const Point anchor{{fx-48,fy-24-(heights[1]+heights[2]+heights[3]+heights[4])/4}};
        append_developed(tile,anchor,false);
        frame.developed_tiles[static_cast<size_t>(y)*40+x/2]=tile.developed_ready && !source.ter1.pixels.empty();
        ++frame.tiles;
    }
    const raster::Bitmap background{1024,511,std::vector<uint8_t>(1024*511)};
    const auto drawn=raster::DrawBatch(background,commands,{{0,0,1024,511}},true);
    frame.rgba=material::ResolveRgba(drawn.bitmap.pixels,source.palette);
    for(size_t i=0;i<drawn.coverage.size();++i) {frame.rgba[4*i+3]=drawn.coverage[i]?255:0;}
    frame.attempts=drawn.attempts;frame.writes=drawn.writes;
    return frame;
}
}
