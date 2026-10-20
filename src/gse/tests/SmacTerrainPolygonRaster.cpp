#include "SmacTerrainPolygonRaster.h"
#include "fixtures/SmacTerrainPolygonRaster.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>

namespace gse::tests {
namespace {
namespace raster=game::frontend::smac_terrain_raster;
raster::Bitmap Synthetic() {
    raster::Bitmap result{56,56,std::vector<uint8_t>(56*56)};
    constexpr uint8_t values[]{9,10,255,1};
    for(size_t y=0;y<56;++y) {for(size_t x=0;x<56;++x) {result.pixels[y*56+x]=values[(x+y)&3];}}
    return result;
}
uint64_t Digest(const std::vector<uint8_t>& pixels) {
    uint64_t hash=14695981039346656037ull;
    for(auto byte:pixels) {hash=(hash^byte)*1099511628211ull;}return hash;
}
std::vector<raster::Command> Commands(const polygon_raster_fixture::Frame& frame,const raster::Bitmap& texture) {
    std::vector<raster::Command> commands;
    for(const auto& draw:frame.draws) {commands.push_back({draw.polygon,&texture,draw.shader,draw.clip});}
    return commands;
}
}
void AddSmacTerrainPolygonRasterTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC alternate terrain rows preserve clipped lag keys coast and water admission",GT(){
        raster::Bitmap texture{8,4,std::vector<uint8_t>(32)};
        for(int y=0;y<4;++y) {for(int x=0;x<8;++x) {texture.pixels[y*8+x]=static_cast<uint8_t>(20+16*y+x);}}
        raster::Command draw;draw.texture=&texture;draw.polygon.count=4;
        draw.polygon.vertices={{{{{0,0}},{{0,0}},2},{{{8,0}},{{7,0}},2},{{{8,4}},{{7,3}},2},{{{0,4}},{{0,3}},2}}};
        const raster::Bitmap initial{8,4,std::vector<uint8_t>(32,222)};
        const raster::Clip clip{{2,0,7,4}};
        const auto ordinary=raster::DrawBatch(initial,{draw},clip,true);
        draw.alternate_rows=true;const auto alternate=raster::DrawBatch(initial,{draw},clip,true);
        for(int y=0;y<4;++y) {for(int x=0;x<8;++x) {
            const auto at=size_t(y*8+x);
            const auto expected=x<2 || x>=7?222:!(y&1)?65:ordinary.bitmap.pixels[y*8+(x==2?2:x-1)];
            GT_ASSERT(alternate.bitmap.pixels[at]==expected,"Alternate opaque parity or clipped first-visible lag differs");
        }}
        for(size_t i=0;i<texture.pixels.size();i+=3) {texture.pixels[i]=9;}
        draw.shader=raster::Shader::KeyedShaded;draw.alternate_rows=false;
        const auto keyed=raster::DrawBatch(initial,{draw},clip,true);draw.alternate_rows=true;
        const auto keyed_alternate=raster::DrawBatch(initial,{draw},clip,true);
        for(size_t i=0;i<32;++i) {GT_ASSERT(keyed_alternate.bitmap.pixels[i]==
            (keyed.coverage[i] && !((i/8)&1)?65:keyed.bitmap.pixels[i]),"Black rows bypassed the native key test");}
        std::fill(texture.pixels.begin(),texture.pixels.end(),77);
        raster::Bitmap mask{8,4,std::vector<uint8_t>(32)};
        for(size_t i=0;i<32;++i) {mask.pixels[i]=static_cast<uint8_t>(i%3);}
        draw.shader=raster::Shader::CoastMapped;draw.mask=&mask;draw.mask_mapping[1]=1;draw.mask_mapping[2]=200;
        draw.shade_limits={{-1,1}};for(auto& vertex:draw.polygon.vertices) {vertex.light=12;}
        draw.alternate_rows=false;const auto coast=raster::DrawBatch(initial,{draw},clip,true);
        draw.alternate_rows=true;const auto coast_alternate=raster::DrawBatch(initial,{draw},clip,true);
        bool uncapped=false;
        for(size_t i=0;i<32;++i) {
            GT_ASSERT(coast_alternate.bitmap.pixels[i]==(coast.coverage[i] && !((i/8)&1)?65:coast.bitmap.pixels[i]),
                "Alternate coast changed mapped transparency, literal color or odd-row depth");
            if(coast.coverage[i] && coast.bitmap.pixels[i]!=200) {uncapped=uncapped || coast.bitmap.pixels[i]==89;}
        }
        GT_ASSERT(uncapped,"Coast discriminator had no shaded source stores");
        draw.shader=raster::Shader::WaterDepth16;draw.shade_limits={{-256,256}};
        draw.polygon.vertices[0].light=-4;draw.polygon.vertices[1].light=40;
        draw.polygon.vertices[2].light=40;draw.polygon.vertices[3].light=-4;
        const auto water=raster::DrawBatch(initial,{draw},clip,true);
        GT_ASSERT(water.writes>0,"Alternate water failed initial polygon admission");
        for(int x=2;x<7;++x) {GT_ASSERT(water.bitmap.pixels[x]==65 && water.bitmap.pixels[16+x]==65,
            "Alternate water applied per-pixel threshold to black rows");}
        for(auto& vertex:draw.polygon.vertices) {vertex.light=20;}
        GT_ASSERT(raster::DrawBatch(initial,{draw},clip,true).writes==0,"Alternate water bypassed whole-polygon rejection");
        draw.shader=raster::Shader::KeyedUnlit;bool rejected=false;
        try {(void)raster::DrawBatch(initial,{draw},clip);}catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected,"Unmeasured alternate shader was silently admitted");GT_OK();
    });
    task->AddTest("SMAC indexed polygon batch matches3332 native shaded and unlit spans",GT(){
        size_t count=0;
        for(const auto& frame:polygon_raster_fixture::Frames()) {for(const auto& draw:frame.draws) {
            const auto got=raster::GeneratePolygonSpans(draw.polygon,draw.clip,draw.shader!=raster::Shader::KeyedUnlit);
            GT_ASSERT(got==draw.spans,"Native polygon span mismatch: "+std::string(frame.name));
            count+=got.size();
        }}
        GT_ASSERT(count==3332,"Missing native keyed polygon spans");
        GT_OK();
    });
    task->AddTest("SMAC indexed polygon batch preserves native key shade order and owned output",GT(){
        const auto texture=Synthetic();
        const raster::Bitmap initial{1024,511,std::vector<uint8_t>(1024*511,250)};
        const auto before=initial.pixels;
        size_t count=0;
        const auto frames=polygon_raster_fixture::Frames();
        for(const auto& frame:frames) {if(frame.synthetic) {
            const auto result=raster::DrawBatch(initial,Commands(frame,texture),frame.clip);
            uint64_t attempts=0,writes=0;
            for(const auto& draw:frame.draws) {attempts+=draw.attempts;writes+=draw.writes;}
            GT_ASSERT(result.attempts==attempts && result.writes==writes && Digest(result.bitmap.pixels)==frame.fnv,
                "Native complete synthetic keyed frame mismatch: "+std::string(frame.name));
            ++count;
        }}
        GT_ASSERT(count==6 && initial.pixels==before,"Missing synthetic controls or mutated caller bitmap");
        auto commands=Commands(frames.front(),texture);
        auto invalid=commands.front();invalid.polygon.count=5;commands.push_back(invalid);
        const auto rejects=[](const auto& f) {try {f();}catch(const std::invalid_argument&) {return true;}return false;};
        GT_ASSERT(rejects([&]{raster::DrawBatch(initial,commands,frames.front().clip);}) && initial.pixels==before,"Invalid later polygon escaped partial output");
        commands.pop_back();commands.front().texture=nullptr;
        GT_ASSERT(rejects([&]{raster::DrawBatch(initial,commands,frames.front().clip);}),"Missing borrowed texture accepted");
        commands=Commands(frames.front(),texture);commands.front().shader=static_cast<raster::Shader>(99);
        GT_ASSERT(rejects([&]{raster::DrawBatch(initial,commands,frames.front().clip);}),"Invalid shader silently selected another path");
        raster::Result retained;
        {const auto temporary=Synthetic();retained=raster::DrawBatch(initial,Commands(frames[21],temporary),frames[21].clip);}
        GT_ASSERT(Digest(retained.bitmap.pixels)==frames[21].fnv,"Output retained a borrowed source after its lifetime");
        GT_OK();
    });
}
}
