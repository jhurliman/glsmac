#pragma once
#include "NativeTerrainView.h"
#include <functional>

namespace game::frontend::native_terrain {
enum class ZoomCommand {In=0x203,Out,Default,Overview,Maximum,Minimum};
inline int CommandZoom(int zoom,ZoomCommand command) {
    switch(command) {
        case ZoomCommand::In:return zoom<16?zoom+2:zoom;
        case ZoomCommand::Out:return zoom> -14?zoom-2:zoom;
        case ZoomCommand::Default:return 0;
        case ZoomCommand::Overview:return -6;
        case ZoomCommand::Maximum:return 16;
        case ZoomCommand::Minimum:return -14;
    }throw std::invalid_argument("Unknown native zoom command");
}
struct ZoomView {bool present=false,active=false;int zoom=0;Point camera{{0,0}};};
using ZoomViews=std::array<ZoomView,8>;
struct ZoomResult {ZoomViews views;int target;std::vector<unsigned> repaints;};
using ZoomPick=std::function<std::optional<Point>(unsigned,const ZoomView&)>;
using ZoomAdmission=std::function<bool(Point)>;
inline ZoomResult ApplyZoom(ZoomViews views,ZoomCommand command,int target,
    const ZoomPick& cursor={},std::optional<Point> overview={},const ZoomAdmission& allowed={}) {
    ZoomResult result{views,target,{}};
    const auto eligible=[&](unsigned i){return views[i].present && (i==0 || views[i].active);};
    if(target<0) {
        if(eligible(0) && overview && allowed && allowed(*overview)) {
            result.views[0].camera=*overview;target=0;
        }else if(cursor) {
            for(unsigned i=0;i<8;++i) {
                if(!eligible(i) || views[i].zoom>=0) {continue;}
                const auto tile=cursor(i,views[i]);
                if(tile && allowed && allowed(*tile)) {result.views[i].camera=*tile;target=int(i);break;}
            }
        }
    }
    result.target=target;
    for(unsigned i=0;i<8;++i) {
        if(eligible(i) && (target<0 || target==int(i))) {
            result.views[i].zoom=CommandZoom(views[i].zoom,command);result.repaints.push_back(i);
        }
    }return result;
}
}
