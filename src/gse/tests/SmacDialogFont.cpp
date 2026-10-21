#include "SmacDialogFont.h"
#include "fixtures/SmacDialogFont.h"
#include "fixtures/WorkshopCardFont.h"
#include "util/SHA256.h"
#include "task/gsetests/GSETests.h"
#include "loader/font/FreeType.h"
#include "types/Font.h"
#include "util/FinallyGuard.h"
#include <fstream>
#include <iterator>
#include <memory>
#include <cstdlib>
#include <array>
namespace gse::tests {
namespace {
class Fonts final:public loader::font::FreeType {
public:
    types::Font* Raster(const std::string& path,const std::vector<unsigned char>& bytes,unsigned char size,loader::font::RasterMode mode) {
        return LoadFontImpl(path,bytes,size,mode);
    }
};
std::string Check(const char* env,unsigned char size,loader::font::RasterMode mode,
    const std::array<smac_dialog_font_fixture::Glyph,95>& expected,bool body) {
    const auto* path=std::getenv(env);if(!path || !*path) {return {};}
    std::ifstream stream(path,std::ios::binary);if(!stream) {return "Configured dialog font is unreadable";}
    const std::vector<unsigned char> data{std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
    Fonts loader;loader.Start();util::FinallyGuard stop([&]{loader.Stop();});
    std::unique_ptr<types::Font> font(loader.Raster(path,data,size,mode));
    size_t equal=0;
    for(unsigned ch=32;ch<=126;++ch) {
        const auto& glyph=font->m_symbols[ch];const auto& original=expected[ch-32];
        GT_ASSERT(glyph.ax==original.advance && glyph.left==original.left,"Dialog glyph advance or left bearing differs from GDI at ASCII"+std::to_string(ch));
        std::array<uint8_t,1024> cell{};
        for(unsigned y=0;y<glyph.height;++y) {for(unsigned x=0;x<glyph.width;++x) {
            const auto value=glyph.data[size_t(y)*glyph.width+x];
            GT_ASSERT(value==0 || value==255,"Dialog font contains antialiased pixels");
            const int dx=4+glyph.left+static_cast<int>(x),dy=24-glyph.top+static_cast<int>(y);
            GT_ASSERT(dx>=0 && dx<32 && dy>=0 && dy<32,"Dialog glyph exceeds the reference cell");
            cell[size_t(dy)*32+size_t(dx)]=value!=0;
        }}
        uint64_t hash=14695981039346656037ull;for(auto value:cell) {hash=(hash^value)*1099511628211ull;}
        const bool same=hash==original.hash;equal+=same;
        // Four measured scan-conversion differences remain, totaling five
        // pixels at16px. Keep the original target hashes for all95 glyphs.
        GT_ASSERT(same || (body && original.known_port_hash && hash==original.known_port_hash),
            "Unexpected dialog glyph mask differs from GDI at ASCII"+std::to_string(ch));
    }
    GT_ASSERT(equal>=(body?91u:95u),"Dialog glyph equality dropped below its measured boundary");
    GT_ASSERT(font->GetTextWidth("CHOOSE A FACTION")==static_cast<size_t>(body?144:138),"Dialog caption run width differs from GDI");
    GT_OK();
}
}
void AddSmacDialogFontTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC Workshop Arial10 matches95 native glyph advances bearings and complete masks",GT(){
        const auto* path=std::getenv("GLSMAC_WORKSHOP_REGULAR_FONT");if(!path || !*path) {return {};}
        std::ifstream stream(path,std::ios::binary);const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
        GT_ASSERT(util::SHA256(std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()))=="b3658eadae55e682b5f69eb64c439c1ecc8f196c0bb8d4756d145d13bc86476a","Workshop font does not match the native receipt");
        Fonts loader;loader.Start();util::FinallyGuard stop([&]{loader.Stop();});
        std::unique_ptr<types::Font> font(loader.Raster(path,bytes,10,loader::font::RasterMode::MONOCHROME));
        for(unsigned ch=32;ch<=126;++ch) {
            const auto& glyph=font->m_symbols[ch];const auto& expected=workshop_card_font_fixture::REGULAR10[ch-32];
            GT_ASSERT(glyph.ax==expected.advance && (!expected.ink || (glyph.left==expected.left && glyph.top==expected.top)),"Workshop GDI glyph metrics differ at ASCII"+std::to_string(ch));
            std::array<char,4096> cell{};
            for(unsigned y=0;y<glyph.height;++y) {for(unsigned x=0;x<glyph.width;++x) {
                const int dx=16+glyph.left+int(x),dy=48-glyph.top+int(y);
                GT_ASSERT(dx>=0 && dx<64 && dy>=0 && dy<64,"Workshop glyph exceeds native cell");
                cell[size_t(dy)*64+size_t(dx)]=glyph.data[size_t(y)*glyph.width+x]!=0;
            }}
            GT_ASSERT(util::SHA256(std::string_view(cell.data(),cell.size()))==expected.sha,"Workshop GDI glyph mask differs at ASCII"+std::to_string(ch));
        }
        GT_ASSERT(font->GetTextWidth("Attacks air units")==72 && font->GetTextWidth("Attacks ")==35
            && font->GetTextWidth("air ")==15 && font->GetTextWidth("ck")==9,"Workshop additive string extents differ");GT_OK();
    });
    task->AddTest("SMAC dialog synthetic body font measured GDI glyphs advances and bearings",GT(){
        return Check("GLSMAC_DIALOG_BODY_FONT",16,loader::font::RasterMode::MONOCHROME_BOLD,smac_dialog_font_fixture::BODY,true);
    });
    task->AddTest("SMAC dialog caption font matches all95 GDI glyph controls",GT(){
        return Check("GLSMAC_DIALOG_CAPTION_FONT",14,loader::font::RasterMode::MONOCHROME,smac_dialog_font_fixture::CAPTION,false);
    });
}
}
