#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace graphics::opengl {

// GPU-only pages: CPU pixels, normalized UVs, save files and update rectangles
// retain the original logical texture layout.
struct TexturePaging {
    size_t width=0,height=0,page_width=0,page_height=0,columns=1,rows=1;
    bool Paged() const { return columns>1 || rows>1; }
    size_t Layers() const { return columns*rows; }
    static TexturePaging Make(size_t width,size_t height,size_t limit,size_t max_layers) {
        if(!limit || !max_layers || width>INT32_MAX || height>INT32_MAX) {throw std::runtime_error("Invalid GPU texture dimensions/limits");}
        TexturePaging result{width,height,width,height,1,1};
        if(!width || !height) {return result;}
        result.columns=1+(width-1)/limit;result.rows=1+(height-1)/limit;
        if(result.columns>max_layers/result.rows) {throw std::runtime_error("Texture exceeds GPU array layer limit");}
        result.page_width=1+(width-1)/result.columns;result.page_height=1+(height-1)/result.rows;
        return result;
    }
    struct Upload {
        size_t left,top,right,bottom,x,y,layer;
    };
    std::vector<Upload> Updates(size_t left,size_t top,size_t right,size_t bottom) const {
        if(left>right || top>bottom || right>width || bottom>height) {throw std::runtime_error("Texture update exceeds logical pixels");}
        std::vector<Upload> result;
        if(left==right || top==bottom) {return result;}
        for(size_t row=top/page_height;row<=(bottom-1)/page_height;++row) {
            for(size_t col=left/page_width;col<=(right-1)/page_width;++col) {
                const auto x=col*page_width,y=row*page_height;
                result.push_back({std::max(left,x),std::max(top,y),std::min(right,x+page_width),std::min(bottom,y+page_height),
                    std::max(left,x)-x,std::max(top,y)-y,row*columns+col});
            }
        }
        return result;
    }
};
}
