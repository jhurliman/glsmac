#include "SmacTerritoryBoundary.h"
#include "fixtures/SmacTerritoryBoundary.h"
#include "game/frontend/NativeTerrainRenderer.h"
#include "game/backend/map/SmacTerrainGeometry.h"
#include "task/gsetests/GSETests.h"
#include <algorithm>
#include <stdexcept>
#include "Mesh.h"
#include "loader/texture/SDL2.h"
#include "util/FinallyGuard.h"
#include "util/UUID.h"
#include <filesystem>
#include <fstream>
#include <SDL.h>

namespace gse::tests {
namespace {
namespace boundary=game::frontend::smac_territory_boundary;
namespace raster=game::frontend::smac_terrain_raster;
uint64_t Digest(const std::vector<raster::Span>& spans) {
    uint64_t result=14695981039346656037ULL;
    for(const auto& s:spans) {for(const auto n:{s.y,s.left,s.right,s.u,s.v,s.du,s.dv,int(s.shade),int(s.dshade)}) {
        for(int shift=0;shift<32;shift+=8) {result=(result^static_cast<uint8_t>(static_cast<uint32_t>(n)>>shift))*1099511628211ULL;}
    }}return result;
}
}
void AddSmacTerritoryBoundaryTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC territory boundary actual SDL source owns custom faction lookup rows and cache assignments",GT(){
        namespace fs=std::filesystem;
        const auto directory=fs::temp_directory_path()/("glsmac-boundary-source-"+util::UUID::Generate());
        const auto data=directory/"default";fs::create_directory(directory);fs::create_directory(data);
        const std::array<std::string,8> names{{"texture.pcx","palette.pcx","rainfall.pcx","custom-a.pcx","custom-b.pcx","system.txt","small.pcx","truecolor.bmp"}};
        util::FinallyGuard remove([&]{std::error_code ignored;for(const auto& name:names) {fs::remove(data/name,ignored);}fs::remove(data,ignored);fs::remove(directory,ignored);});
        const auto write_pcx=[](const fs::path& path,int width,int height,const std::vector<uint8_t>& pixels) {
            std::vector<uint8_t> bytes(128,0);bytes[0]=10;bytes[1]=5;bytes[2]=1;bytes[3]=8;bytes[65]=1;
            const auto word=[&](size_t at,int value){bytes[at]=value&255;bytes[at+1]=(value>>8)&255;};
            word(8,width-1);word(10,height-1);word(66,width);word(68,1);
            for(const auto value:pixels) {if(value>=192) {bytes.push_back(193);}bytes.push_back(value);}
            bytes.push_back(12);for(int i=0;i<256;++i) {bytes.push_back(static_cast<uint8_t>(i));bytes.push_back(0);bytes.push_back(0);}
            std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            if(!file) {throw std::runtime_error("Cannot write boundary PCX fixture");}
        };
        write_pcx(data/names[0],1024,768,std::vector<uint8_t>(1024*768,20));
        std::vector<uint8_t> common(640*60,0);for(size_t owner=0;owner<8;++owner) {common[56*640+580+8*owner]=static_cast<uint8_t>(30+owner);}
        write_pcx(data/names[1],640,60,common);
        std::vector<uint8_t> rainfall(128*400,255);rainfall[254*128+2]=208;write_pcx(data/names[2],128,400,rainfall);
        std::vector<uint8_t> faction(162*758,255);faction[747*162+161]=120;faction[757*162+161]=255;write_pcx(data/names[3],162,758,faction);
        faction[747*162+161]=80;faction[757*162+161]=128;write_pcx(data/names[4],162,758,faction);
        write_pcx(data/names[6],2,2,{1,1,1,1});
        {const std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> rgb(SDL_CreateRGBSurfaceWithFormat(0,162,758,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
            GT_ASSERT(rgb && SDL_SaveBMP(rgb.get(),(data/names[7]).string().c_str())==0,"Cannot write true-color boundary fallback fixture");}
        {std::ofstream file(data/names[5]);file<<"#BOUNDARYPERCENTS\n";for(int i=0;i<8;++i) {file<<"0, default\n";}}
        return WithWidgetTextureResources(directory.string(),[&]()->std::string {
            std::shared_ptr<const game::frontend::native_terrain::Source> retained;
            {
                loader::texture::SDL2 loader;
                const auto defaults=loader.GetSmacTerrainSource();
                std::array<std::string,8> files{};files[3]=names[3];
                const auto custom=loader.GetSmacTerrainSource(files);
                GT_ASSERT(custom==loader.GetSmacTerrainSource(files) && custom!=defaults,"Native terrain source ignored actual faction cache assignment");
                GT_ASSERT(custom->faction_files==files && custom->loaded.pixels[0]==30 && custom->boundary_mask.pixels[0]==1 && custom->boundary_mask.pixels[1]==9,"Native terrain source did not own loaded indices/classification");
                GT_ASSERT(custom->palette[245][0]==235 && custom->palette[255][0]==255,"Native boundary source lost common Gamma100 or reserved display colors");
                const auto expected=boundary::Row(custom->palette,130,100);
                GT_ASSERT(custom->owner_clut[3]==expected && custom->owner_clut[3]!=defaults->owner_clut[3] && custom->owner_clut[5]==defaults->owner_clut[5],"Custom faction override was applied to a stock transport slot or omitted");
                files[3]=names[4];files[5]=names[3];const auto reassigned=loader.GetSmacTerrainSource(files);
                GT_ASSERT(reassigned!=custom && reassigned->owner_clut[3]==boundary::Row(custom->palette,90,50) && reassigned->owner_clut[5]==expected,"Native faction reassignment reused stale color/opacity rows");
                for(const auto& fallback:{names[6],names[7]}) {files[3]=fallback;const auto port_art=loader.GetSmacTerrainSource(files);
                    GT_ASSERT(port_art->owner_clut[3]==defaults->owner_clut[3] && port_art->owner_clut[5]==expected,"Non-native custom artwork did not retain the global owner row");}
                retained=custom;
            }
            GT_ASSERT(retained->loaded.pixels[0]==30 && retained->owner_clut[3][30]==130,"Native terrain source borrowed retired SDL surfaces or loader cache");GT_OK();
        });
    });

    task->AddTest("SMAC territory boundary reproduces31 native owner region and topology contexts",GT(){
        size_t count=0,commands=0;
        constexpr std::array<raster::Point,8> offsets{{{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}}};
        for(const auto& c:boundary_fixture::Contexts()) {
            const boundary::CellAt at=[&](int x,int y)->std::optional<boundary::Cell> {
                if(y<0 || y>=80) return {};
                if(c.wrap) x=(x%80+80)%80;
                if(x<0 || x>=80) return {};
                uint8_t band=c.band;int region=1,owner=c.owner;
                if(x==c.x && y==c.y) {band=c.center_band;}
                else {for(size_t i=0;i<offsets.size();++i) {
                    if(x==(c.x+offsets[i][0]+80)%80 && y==c.y+offsets[i][1] && c.different[i]) {region=c.other_region;owner=-1;}
                }}
                for(const auto& patch:c.patch) {if(x==(c.x+patch[0]+80)%80 && y==c.y+patch[1]) {band=static_cast<uint8_t>(patch[2]);}}
                return boundary::Cell{band,static_cast<uint8_t>(region),owner};
            };
            const auto view=game::frontend::native_terrain::View(c.x,c.y,c.wrap);
            const auto center=std::find_if(view.begin(),view.end(),[&](const auto& p){return p.tile==raster::Point{c.x,c.y};});
            GT_ASSERT(center!=view.end(),"Native boundary camera omitted center");
            const auto h=game::backend::map::smac_terrain::OrdinaryHeights(c.x,c.y,[&](int x,int y)->std::optional<uint8_t>{const auto cell=at(x,y);return cell?std::optional<uint8_t>{cell->band}:std::nullopt;});
            const auto x=center->flat_center[0],y=center->flat_center[1];
            const std::array<raster::Point,5> points{{{{x,y-h[0]}},{{x-48,y-h[1]}},{{x,y-24-h[2]}},{{x+48,y-h[3]}},{{x,y+24-h[4]}}}};
            boundary::Context context;context.viewer=3;context.relations.fill(c.relation);
            const auto actual=boundary::Select(c.x,c.y,points,at,context,c.flags);
            GT_ASSERT(actual.has_value()==!c.expected.empty(),"Native boundary admission differs: "+std::string(c.name));
            if(actual) {
                const auto& expected=c.expected.front();
                GT_ASSERT(actual->mapping==expected.mapping && actual->shade_limits==expected.shade_limits && actual->polygon.count==expected.polygon.count,"Boundary mapping differs: "+std::string(c.name));
                for(size_t i=0;i<4;++i) {GT_ASSERT(actual->polygon.vertices[i].screen==expected.polygon.vertices[i].screen && actual->polygon.vertices[i].uv==expected.polygon.vertices[i].uv && actual->polygon.vertices[i].light==0,"Boundary polygon differs: "+std::string(c.name));}
                ++commands;
            }++count;
        }
        GT_ASSERT(count==31 && commands==24,"Missing native territory contexts");GT_OK();
    });
    task->AddTest("SMAC territory boundary reproduces5024 native composed spans with half texel bias",GT(){
        size_t count=0;
        for(const auto& f:boundary_fixture::Frames()) {for(const auto& draw:f.draws) {
            const auto& p=draw.pass;
            const bool skip=p.shader==raster::Shader::WaterDepth16 && std::all_of(p.polygon.vertices.begin(),p.polygon.vertices.end(),[](const auto& v){return v.light>=16;});
            const auto spans=skip?std::vector<raster::Span>{}:raster::GeneratePolygonSpans(p.polygon,f.clip,p.shader!=raster::Shader::KeyedUnlit,p.shade_limits);
            GT_ASSERT(spans.size()==draw.spans && Digest(spans)==draw.span_hash,"Native boundary spans differ: "+std::string(f.name));count+=spans.size();
        }}GT_ASSERT(count==5024,"Missing native boundary spans");GT_OK();
    });
    task->AddTest("SMAC territory boundary quantizes common colors with stable ties and reserved identities",GT(){
        game::frontend::smac_land_material::Palette colors{};
        for(auto& color:colors) {color={100,0,0};}colors[10]={0,0,0};colors[11]={2,0,0};
        const auto tie=boundary::Row(colors,11,50),zero=boundary::Row(colors,11,0);
        GT_ASSERT(tie[10]==10 && zero[20]==12,"Boundary lost first nearest-color tie or duplicate-color quantization");
        for(size_t i=0;i<256;++i) {if(i<10 || i>245) GT_ASSERT(tie[i]==i && zero[i]==i,"Boundary remapped a reserved palette entry");}
        const auto percentages=boundary::ReadOpacities("#OTHER\n1,test\n#BOUNDARYPERCENTS\r\n0, unused\r\n50, gaian\n60, hive ;note\n50, university\n50, morgan\n60, spartan\n50, believer\n70, peace\n#END\n");
        GT_ASSERT((percentages==std::array<int,8>{{0,50,60,50,50,60,50,70}}),"Boundary opacities ignored system section");GT_OK();
    });
    task->AddTest("SMAC territory boundary enforces visibility relations and counts identity stores",GT(){
        boundary::Context context;context.viewer=3;
        GT_ASSERT(boundary::Owner(3,context)==3 && boundary::Owner(0,context)==-1 && boundary::Owner(2,context)==-1,"Boundary owner query leaked foreign or unclaimed information");
        context.relations[2]=0x08000008;GT_ASSERT(boundary::Owner(2,context)==2,"Boundary rejected admitted foreign owner");
        const boundary::CellAt at=[](int x,int y)->std::optional<boundary::Cell>{return boundary::Cell{3,1,(x==40 && y==40)?3:-1};};
        const std::array<raster::Point,5> points{{{{4,4}},{{0,4}},{{4,0}},{{8,4}},{{4,8}}}};
        context.visible=false;GT_ASSERT(!boundary::Select(40,40,points,at,context,0x10001),"Boundary ignored current visibility gate");
        context.visibility_bypass=true;GT_ASSERT(boundary::Select(40,40,points,at,context,0x10001),"Boundary ignored viewer capability bypass");
        context.override_mode=true;GT_ASSERT(!boundary::Select(40,40,points,at,context,0x10001),"Boundary ignored renderer override");
        raster::OwnerClut clut{};for(auto& row:clut) {for(size_t i=0;i<256;++i) {row[i]=static_cast<uint8_t>(i);}}
        raster::Bitmap initial{8,8,std::vector<uint8_t>(64,255)},classes{8,8,std::vector<uint8_t>(64,1)};
        raster::Command command;command.texture=&classes;command.shader=raster::Shader::OwnerClut;command.owner_clut=&clut;command.owner_mapping[1]=4;
        command.polygon={{{{{{0,0}},{{0,0}},0},{{{8,0}},{{7,0}},0},{{{8,8}},{{7,7}},0},{{{0,8}},{{0,7}},0}}},4};
        const auto painted=raster::DrawBatch(initial,{command},{{0,0,8,8}},true);
        GT_ASSERT(painted.writes==64 && painted.bitmap.pixels==initial.pixels && std::all_of(painted.coverage.begin(),painted.coverage.end(),[](auto n){return n!=0;}),"Reserved identity-valued boundary store lost coverage");GT_OK();
    });
}
}
