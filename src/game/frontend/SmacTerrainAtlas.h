#pragma once
#include "SmacTerrainRaster.h"
#include <algorithm>
#include <map>
#include <stdexcept>
namespace game::frontend::smac_terrain_raster {
// Synchronous command source owner. Keep this object and its immutable source
// alive until DrawBatch returns; cached bitmap addresses survive insertions.
class AtlasCrops {
public:
    explicit AtlasCrops(const Bitmap& source):m_source(source) {
        if(source.width<=0 || source.height<=0 || uint64_t(source.width)*static_cast<uint32_t>(source.height)!=source.pixels.size()) {
            throw std::invalid_argument("Invalid indexed terrain source");
        }
    }
    const Bitmap* Get(const std::array<int,4>& rect) {
        auto found=m_crops.find(rect);
        if(found==m_crops.end()) {
            const auto x=rect[0],y=rect[1],width=rect[2],height=rect[3];
            if(x<0 || y<0 || width<=0 || height<=0 || width>m_source.width || height>m_source.height ||
                x>m_source.width-width || y>m_source.height-height) {throw std::invalid_argument("Native terrain crop outside indexed atlas");}
            Bitmap pixels{width,height,std::vector<uint8_t>(static_cast<size_t>(width)*static_cast<size_t>(height))};
            for(int row=0;row<height;++row) {
                std::copy_n(m_source.pixels.begin()+static_cast<size_t>(y+row)*static_cast<size_t>(m_source.width)+static_cast<size_t>(x),width,
                    pixels.pixels.begin()+static_cast<size_t>(row)*static_cast<size_t>(width));
            }
            found=m_crops.emplace(rect,std::move(pixels)).first;
        }
        return &found->second;
    }
private:
    const Bitmap& m_source;
    std::map<std::array<int,4>,Bitmap> m_crops;
};
}
