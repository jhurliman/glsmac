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
