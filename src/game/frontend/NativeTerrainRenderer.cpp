#include "NativeTerrainRenderer.h"
#include "SmacTerrainAtlas.h"
#include "SmacWaterCoast.h"
#include "SmacRiver.h"
#include "SmacRoadTube.h"
#include "SmacDevelopedOverlay.h"
#include "SmacLandmarkArt.h"
#include "SmacResourceArt.h"
#include "game/backend/map/SmacTerrainGeometry.h"
#include "game/backend/map/SmacLandmarkGeometry.h"
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
    if(y<0 || y>=world.height || (x-y)%2) {return nullptr;}
    if(world.horizontal_wrap) {x=(x%world.width+world.width)%world.width;}
    if(x<0 || x>=world.width) {return nullptr;}
    return &world.tiles[static_cast<size_t>(y)*size_t(world.width/2)+static_cast<size_t>(x)/2];
}
}
std::vector<Placement> View(int x,int y,bool wrap) {
    return SelectView(Layout(x,y,0,wrap));
}

static Frame RenderImpl(const World& world,const Source& source,const std::vector<Placement>& placements,uint16_t base,Point offset,uint32_t display_flags,uint8_t initial_index,const std::vector<ViewGeometry::Terrain>* prepared,const Frame* previous=nullptr,Point scroll={{0,0}},raster::Clip clip={{0,0,1024,511}},bool opaque_background=false,Point dimensions={{1024,511}},int unit=12,int zoom=0) {
    if(!world.ValidDimensions() || dimensions[0]<=0 || dimensions[1]<=0 || dimensions[0]>32767 || dimensions[1]>32767) {throw std::invalid_argument("Invalid native bitmap dimensions");}
    const int width=dimensions[0],height=dimensions[1],half_width=4*unit,half_height=2*unit;
    const size_t pixels=size_t(width)*size_t(height);
    raster::AtlasCrops crops(source.loaded);
    if(offset[0]<-4096 || offset[0]>4096 || offset[1]<-4096 || offset[1]>4096 || placements.size()>world.tiles.size()) {
        throw std::invalid_argument("Unsupported native terrain view offset or size");
    }
    const terrain::BandAt bands=[&](int x,int y)->std::optional<uint8_t> {
        const auto* tile=At(world,x,y);return tile?std::optional<uint8_t>{tile->band}:std::nullopt;
    };
    const material::TileAt materials=[&](int x,int y)->std::optional<material::Tile> {
        const auto* tile=At(world,x,y);if(!tile) {return {};}
        return material::Tile{tile->band,tile->moisture,tile->rockiness,tile->items};
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
    const auto crop=[&](const material::Crop& rect) {return crops.Get(rect);};
    Frame frame;frame.unit=unit;frame.zoom=zoom;frame.dimensions=dimensions;frame.world={{world.width,world.height}};
    frame.developed_tiles.assign(world.tiles.size(),false);frame.landmark_tiles.assign(world.tiles.size(),false);frame.resource_tiles.assign(world.tiles.size(),false);
    std::map<material::Crop,smac_developed_overlay::Sprite> sprites;
    std::vector<raster::Command> commands;commands.reserve(placements.size()*14);
    const auto append_marker=[&](const std::optional<material::Crop>& pick,const Point& anchor,bool shadow=false) {
        if(!pick || source.ter1.pixels.empty()) {return;}
        auto found=sprites.find(*pick);
        if(found==sprites.end()) {found=sprites.emplace(*pick,smac_developed_overlay::Decode(source.ter1,*pick)).first;}
        const auto& sprite=found->second;if(sprite.bitmap.pixels.empty()) {return;}
        raster::Command command;command.texture=&sprite.bitmap;command.shader=shadow?raster::Shader::SpriteShadow:raster::Shader::Sprite;
        command.shadow_lookup=&source.shadow_lookup;
        command.sprite_origin=anchor;command.sprite_offset=sprite.offset;command.sprite_scale={{zoom+16,16}};commands.push_back(command);
    };
    const auto append_quads=[&](int x,int y,const std::array<Point,5>& points,const std::array<int,5>& light) {
        const auto& tile=*At(world,x,y);
        for(const auto& pass:smac_landmark_art::Quads(tile.landmarks,tile.band>=3,points,light)) {
            raster::Command command;command.texture=crop(pass.crop);command.polygon=pass.polygon;
            command.shader=pass.shader;command.shade_limits=pass.shade_limits;commands.push_back(command);
        }
        // Outer screen-Y offsets sum to zero. Ceil(mean projected Y)
        // equals flatCenterY-floor(mean height), including negative heights.
        int sum=0;for(unsigned v=1;v<5;++v) {sum+=points[v][1];}
        append_marker(smac_landmark_art::EarlyMarker(tile.landmarks,display_flags),
            {{points[0][0]-half_width,-terrain::LandmarkGeometry::FloorQuarter(-sum)-half_height}});
        std::array<std::optional<smac_landmark_art::Neighbor>,8> neighbors{};
        for(unsigned n=0;n<8;++n) {
            const auto& offset=smac_land_material::tables::NEIGHBORS[n];
            if(const auto* cell=At(world,x+offset[0],y+offset[1])) {neighbors[n]={{cell->band,cell->landmarks}};}
        }
        if(const auto pass=smac_landmark_art::Jungle(tile.landmarks,tile.band>=3,points,neighbors)) {
            raster::Command command;command.texture=crop(pass->crop);command.polygon=pass->polygon;
            command.shader=pass->shader;command.shade_limits=pass->shade_limits;commands.push_back(command);
        }
    };
    const auto append_water=[&](int x,int y,const std::array<Point,5>& points,bool survey=false) {
        const bool sea=At(world,x,y)->band<3;bool quads=false;
        const auto& tile=*At(world,x,y);
        for(const auto& pass:smac_water_coast::Select(x,y,points,cells,
            survey?std::optional<uint32_t>{tile.selected_items&~0x20u}:
            tile.selected_known?std::optional<uint32_t>{tile.selected_items}:std::nullopt)) {
            if(!survey && sea && !quads && pass.shader==raster::Shader::KeyedUnlit) {
                append_quads(x,y,points,{});quads=true;
            }
            raster::Command command;command.texture=crop(pass.crop);command.polygon=pass.polygon;
            command.shader=pass.shader;command.shade_limits=pass.shade_limits;command.alternate_rows=survey;
            if(pass.shader==raster::Shader::CoastMapped) {command.mask=&source.coast_mask;command.mask_mapping=pass.mapping;}
            commands.push_back(command);
        }
        if(!survey && sea && !quads) {append_quads(x,y,points,{});}
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
        for(const auto& pick:smac_developed_overlay::Select(tile.selected_items,tile.band,tile.nutrients,tile.landmarks,display_flags,zoom)) {
            const bool is_kelp=tile.band<3 && pick.crop==material::Crop{{607,190,100,62}};
            if(is_kelp!=kelp) {continue;}
            auto found=sprites.find(pick.crop);
            if(found==sprites.end()) {found=sprites.emplace(pick.crop,smac_developed_overlay::Decode(source.ter1,pick.crop)).first;}
            const auto& sprite=found->second;if(sprite.bitmap.pixels.empty()) {continue;}
            raster::Command command;command.texture=&sprite.bitmap;
            command.sprite_origin=anchor;command.sprite_offset=sprite.offset;command.sprite_scale={{zoom+16,16}};
            command.shader=pick.shadow?raster::Shader::SpriteShadow:raster::Shader::Sprite;
            command.shadow_lookup=&source.shadow_lookup;commands.push_back(command);
        }
    };
    size_t placement_index=0;
    for(const auto& placement:placements) {
        const auto* submission=prepared?&prepared->at(placement_index):nullptr;++placement_index;
        const auto x=placement.tile[0],y=placement.tile[1];
        if(x<0 || x>=world.width || y<0 || y>=world.height || ((x^y)&1) ||
            placement.record_offset!=static_cast<uint32_t>(44u*static_cast<uint32_t>((world.width/2)*y+x/2)) ||
            placement.flat_center[0]<-4096 || placement.flat_center[0]>4096 ||
            placement.flat_center[1]<-4096 || placement.flat_center[1]>4096) {throw std::invalid_argument("Invalid native terrain placement");}
        const auto& tile=*At(world,x,y);
        if(submission && submission->unknown) {
            for(unsigned edge=0;edge<4;++edge) {
                const auto& a=submission->outline[edge];const auto& b=submission->outline[(edge+1)%4];
                raster::Command command;command.shader=raster::Shader::IndexedLine;command.line_color=194;
                command.line={{a[0]+offset[0],a[1]+offset[1],b[0]+offset[0],b[1]+offset[1]}};commands.push_back(command);
            }++frame.tiles;continue;
        }
        const bool survey=submission && submission->survey;
        const bool admitted=survey || (submission && world.retained_geometry?tile.material_admitted:tile.ordinary);
        if(!admitted) {continue;}
        const auto center=submission?submission->flat_center:placement.flat_center;
        const auto fx=center[0]+offset[0],fy=center[1]+offset[1];
        const auto resources=!survey && tile.resource_ready && !source.ter1.pixels.empty()
            ?smac_resource_art::Select(x,y,tile.band,tile.resource_items,world.resource_pattern,world.automatic_land_resources,display_flags,zoom)
            :std::vector<smac_resource_art::Pick>{};
        const auto append_resources=[&](const Point& anchor,bool late) {
            for(const auto& pick:resources) {
                if((pick.stage!=smac_resource_art::Stage::Resource)==late) {append_marker(pick.crop,anchor,pick.shadow);}
            }
        };
        if(tile.band<3) {
            frame.landmark_tiles[static_cast<size_t>(y)*size_t(world.width/2)+x/2]=!source.ter1.pixels.empty();
            const std::array<Point,5> points{{{{fx,fy}},{{fx-half_width,fy}},{{fx,fy-half_height}},{{fx+half_width,fy}},{{fx,fy+half_height}}}};
            append_water(x,y,points,survey);
            if(survey) {++frame.tiles;continue;}
            append_boundary(x,y,points);
            const Point anchor{{fx-half_width,fy-half_height}};
            append_developed(tile,anchor,true);append_resources(anchor,false);append_developed(tile,anchor,false);
            append_marker(smac_landmark_art::Nexus(tile.landmarks,display_flags),anchor);append_resources(anchor,true);
            frame.resource_tiles[static_cast<size_t>(y)*size_t(world.width/2)+x/2]=tile.resource_ready && !source.ter1.pixels.empty();
            frame.developed_tiles[static_cast<size_t>(y)*size_t(world.width/2)+x/2]=tile.developed_ready && !source.ter1.pixels.empty();
            ++frame.tiles;continue;
        }
        // Owned backend geometry already authenticates the exact height and
        // lighting inputs. Only the legacy ordinary fallback needs this broad
        // guard against unimplemented neighboring profile stencils.
        if(!submission && (!tile.heights || !tile.lighting)) {
            bool ordinary=true;
            for(int dy=-4;dy<=4;++dy) {for(int dx=-4;dx<=4;++dx) {if((dx-dy)%2) {continue;}
                const auto* neighbor=At(world,x+dx,y+dy);
                if(neighbor && neighbor->band>=3 && neighbor->deformed) {ordinary=false;}
            }}
            if(!ordinary) {continue;}
        }
        const auto light=submission?std::optional<std::array<int,5>>{submission->lighting}:tile.lighting?tile.lighting:terrain::OrdinaryLighting(x,y,bands);if(!light) {continue;}
        const auto heights=submission?submission->heights:tile.heights?*tile.heights:terrain::OrdinaryHeights(x,y,bands);
        frame.landmark_tiles[static_cast<size_t>(y)*size_t(world.width/2)+x/2]=!source.ter1.pixels.empty();
        const std::array<Point,5> points{{{{fx,fy-heights[0]}},{{fx-half_width,fy-heights[1]}},
            {{fx,fy-half_height-heights[2]}},{{fx+half_width,fy-heights[3]}},{{fx,fy+half_height-heights[4]}}}};
        const auto seed=static_cast<uint16_t>(base+placement.record_offset);
        bool quads=false;
        const material::TileAt center_materials=survey?material::TileAt{[&materials,x,y](int tx,int ty) {
            auto cell=materials(tx,ty);
            if(cell && tx==x && ty==y) {cell->moisture=0;cell->rockiness=0;}
            return cell;
        }}:materials;
        const auto center_items=survey?std::optional<uint32_t>{tile.selected_items&~0x20u}:
            tile.selected_known?std::optional<uint32_t>{tile.selected_items}:std::nullopt;
        for(const auto& pass:material::Select({{x,y}},center_materials,seed,center_items)) {
            if(survey && (pass.role==material::Role::Forest || pass.role==material::Role::Fungus)) {continue;}
            if(!quads && (pass.role==material::Role::Forest || pass.role==material::Role::Fungus)) {
                append_quads(x,y,points,*light);quads=true;
            }
            const auto dunes=survey?std::optional<material::Crop>{}:smac_landmark_art::Dunes(tile.landmarks,true,tile.selected_known?tile.selected_items:tile.items,display_flags);
            raster::Command command;command.texture=crop(pass.role==material::Role::Base && dunes?*dunes:pass.crop);command.polygon.count=pass.vertices;command.alternate_rows=survey;
            command.shader=pass.role==material::Role::Base?raster::Shader::OpaqueShaded:
                pass.role==material::Role::Fungus?raster::Shader::KeyedUnlit:raster::Shader::KeyedShaded;
            for(size_t v=0;v<pass.vertices;++v) {
                const auto index=pass.fan<0?v+1:v==0?0:1+(static_cast<size_t>(pass.fan)+v-1)%4;
                command.polygon.vertices[v]={points[index],pass.uv[v],(*light)[index]};
            }
            commands.push_back(command);
        }
        if(!survey && !quads) {append_quads(x,y,points,*light);}
        if(!survey) {append_boundary(x,y,points);}
        append_water(x,y,points,survey);
        for(const auto& pass:smac_river::Select(x,y,points,*light,cells,display_flags)) {
            raster::Command command;command.texture=crop(pass.crop);command.polygon=pass.polygon;
            command.shader=pass.shader;command.shade_limits=pass.shade_limits;command.alternate_rows=survey;commands.push_back(command);
        }
        if(survey) {++frame.tiles;continue;}
        for(const auto& pass:smac_road_tube::Select(x,y,points,*light,roads,display_flags)) {
            raster::Command command;command.texture=crop(pass.crop);command.polygon=pass.polygon;
            command.shader=pass.shader;command.shade_limits=pass.shade_limits;commands.push_back(command);
        }
        const Point anchor=submission?Point{{submission->projector[0]+offset[0],submission->projector[1]+offset[1]}}
            :Point{{fx-half_width,fy-half_height-terrain::LandmarkGeometry::FloorQuarter(heights[1]+heights[2]+heights[3]+heights[4])}};
        append_resources(anchor,false);append_developed(tile,anchor,false);
        append_marker(smac_landmark_art::Nexus(tile.landmarks,display_flags),anchor);append_resources(anchor,true);
        frame.resource_tiles[static_cast<size_t>(y)*size_t(world.width/2)+x/2]=tile.resource_ready && !source.ter1.pixels.empty();
        frame.developed_tiles[static_cast<size_t>(y)*size_t(world.width/2)+x/2]=tile.developed_ready && !source.ter1.pixels.empty();
        ++frame.tiles;
    }
    raster::Bitmap background{width,height,std::vector<uint8_t>(pixels,opaque_background?(previous?0:65):initial_index)};
    std::vector<uint8_t> retained_coverage(pixels,0);
    if(previous) {
        if(previous->opaque_background!=opaque_background || previous->unit!=unit || previous->zoom!=zoom || previous->dimensions!=dimensions || previous->world!=frame.world || previous->indexed.size()!=pixels || previous->coverage.size()!=pixels || previous->developed_tiles.size()!=world.tiles.size() || previous->landmark_tiles.size()!=world.tiles.size() || previous->resource_tiles.size()!=world.tiles.size() || scroll[1]!=0 ||
            (scroll[0]!=-8*unit && scroll[0]!=8*unit)) {throw std::invalid_argument("Invalid native horizontal bitmap history");}
        for(int y=0;y<height;++y) {for(int x=0;x<width;++x) {
            const int sx=x-scroll[0];if(sx<0 || sx>=width) {continue;}
            const size_t dest=size_t(y)*size_t(width)+x,src=size_t(y)*size_t(width)+sx;
            background.pixels[dest]=previous->indexed[src];retained_coverage[dest]=previous->coverage[src];
        }}
        for(size_t i=0;i<world.tiles.size();++i) {
            frame.developed_tiles[i]=frame.developed_tiles[i] || previous->developed_tiles[i];
            frame.landmark_tiles[i]=frame.landmark_tiles[i] || previous->landmark_tiles[i];
            frame.resource_tiles[i]=frame.resource_tiles[i] || previous->resource_tiles[i];
        }
    }
    auto drawn=raster::DrawBatch(background,commands,clip,true);
    for(size_t i=0;i<drawn.coverage.size();++i) {drawn.coverage[i]|=retained_coverage[i];}
    if(opaque_background) {std::fill(drawn.coverage.begin(),drawn.coverage.end(),1);}
    frame.opaque_background=opaque_background;
    frame.rgba=material::ResolveRgba(drawn.bitmap.pixels,source.palette);
    for(size_t i=0;i<drawn.coverage.size();++i) {frame.rgba[4*i+3]=drawn.coverage[i]?255:0;}
    frame.indexed=drawn.bitmap.pixels;frame.coverage=drawn.coverage;
    frame.attempts=drawn.attempts;frame.writes=drawn.writes;
    return frame;
}
Frame Render(const World& world,const Source& source,const std::vector<Placement>& placements,uint16_t base,Point offset,uint32_t flags,uint8_t initial_index) {
    return RenderImpl(world,source,placements,base,offset,flags,initial_index,nullptr);
}
Frame RenderPrepared(const World& world,const Source& source,const ViewGeometry::Plan& plan,uint16_t base,uint32_t flags,uint8_t initial_index,const Frame* previous) {
    if(plan.world!=Point{{world.width,world.height}} || plan.zoom< -14 || plan.zoom>16 ||
        plan.unit!=::game::native_zoom::Unit(plan.zoom)) {
        throw std::invalid_argument("Native material plan has inconsistent zoom geometry");
    }
    if(bool(plan.regions)!=bool(previous)) {throw std::invalid_argument("Native dirty plan requires matching retained bitmap history");}
    std::vector<Placement> placements;placements.reserve(plan.terrain.size());
    for(const auto& tile:plan.terrain) {placements.push_back(tile.placement);}
    return RenderImpl(world,source,placements,base,{{0,0}},flags,initial_index,&plan.terrain,previous,plan.scroll,plan.clip,plan.opaque_background,plan.viewport,plan.unit,plan.zoom);
}
}
