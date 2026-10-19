#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
namespace loader::texture::smac_effect {
using Palette=std::array<std::array<uint8_t,3>,256>;
struct Row {int color,opacity;};
using Rows=std::array<Row,16>;
using Bank=std::array<uint8_t,4096>;
Rows ReadRows(const std::string& system,int bank);
Bank Generate(const Palette& palette,const Rows& rows,const std::vector<std::pair<uint8_t,uint8_t>>& protected_ranges={});
// Unknown rows, including the original XF index252 out-of-bank read, reject.
// A graphical fallback must be an explicit caller policy, not a guessed CLUT.
uint8_t Composite(const Bank& bank,uint8_t source,uint8_t destination);
}
