#include "SmacUnitStackPlacement.h"
#include <stdexcept>

namespace game::backend::unit {
namespace {
class Placement {
public:
    explicit Placement(SmacUnitPlacementWorld& world):w(world) {
        if(w.width<=0 || w.height<=0 || (w.width&1) || uint64_t(w.width/2)*uint64_t(w.height)!=w.tiles.size() || w.units.size()>2048) {
            throw std::invalid_argument("Invalid original Unit placement geometry/pool");
        }
    }
    SmacNativeUnitRecord& Unit(size_t i) const {
        if(i>=w.units.size() || w.units[i].id!=i) {throw std::invalid_argument("Unit placement requires dense original identities");}return w.units[i];
    }
    bool Valid(int32_t x,int32_t y) const {return x>=0 && x<w.width && y>=0 && y<w.height;}
    size_t Tile(int32_t x,int32_t y) const {return size_t(y)*size_t(w.width/2)+size_t(x/2);}
    uint32_t Items(size_t i) const {const auto& r=w.tiles[i];return uint32_t(r[8])|(uint32_t(r[9])<<8)|(uint32_t(r[10])<<16)|(uint32_t(r[11])<<24);}
    void Items(size_t i,uint32_t value) {for(unsigned k=0;k<4;++k) {w.tiles[i][8+k]=uint8_t(value>>(8*k));}}
    int32_t Head(int32_t x,int32_t y) const {
        if(Valid(x,y) && !(Items(Tile(x,y))&2)) {return -1;}
        size_t i=0;for(;i<w.units.size();++i) {if(w.units[i].x==x && w.units[i].y==y) {break;}}
        if(i==w.units.size()) {if(Valid(x,y)) {throw std::invalid_argument("Stale Unit occupancy requires native global repair");}return -1;}
        for(size_t steps=0;steps<w.units.size();++steps) {
            const auto& u=Unit(i);if(u.stack_previous==-1) {return int32_t(i);}
            if(u.stack_previous<0) {throw std::invalid_argument("Invalid predecessor sentinel");}
            const auto previous=size_t(u.stack_previous);const auto& p=Unit(previous);
            if(p.x!=x || p.y!=y || p.stack_next!=int32_t(i)) {throw std::invalid_argument("Inconsistent original Unit stack");}i=previous;
        }throw std::invalid_argument("Cyclic original Unit stack");
    }
    SmacUnitPlacementWorld& w;
};
int16_t Signed16(int32_t value) {const auto u=uint16_t(value);return u<=INT16_MAX?int16_t(u):int16_t(int32_t(u)-65536);}
}
void PlaceSmacUnit(SmacUnitPlacementWorld& world,size_t index,int32_t x,int32_t y,int32_t triad) {
    Placement p(world);auto& unit=p.Unit(index);
    if(triad<0 || triad>2 || unit.owner>7 || unit.stack_next!=-1 || unit.stack_previous!=-1) {throw std::invalid_argument("Insertion requires an unlinked admitted Unit");}
    const auto head=p.Head(x,y);if(head==int32_t(index)) {throw std::invalid_argument("Cannot insert a Unit onto itself");}
    unit.stack_next=int16_t(head);unit.stack_previous=-1;unit.x=Signed16(x);unit.y=Signed16(y);world.invalidation.unit=-1;
    if(head>=0) {p.Unit(size_t(head)).stack_previous=int16_t(index);}
    else if(p.Valid(x,y)) {const auto i=p.Tile(x,y);if(!(p.Items(i)&1)) {world.tiles[i][2]=uint8_t((world.tiles[i][2]&240)|(unit.owner&15));}}
    if(p.Valid(x,y)) {const auto i=p.Tile(x,y);p.Items(i,p.Items(i)|2u|(unit.owner && triad!=2?0x100000u:0u));}
}
void UnlinkSmacUnit(SmacUnitPlacementWorld& world,size_t index) {
    Placement p(world);auto& unit=p.Unit(index);const auto previous=unit.stack_previous,next=unit.stack_next,x=unit.x,y=unit.y;
    if(previous<-1 || next<-1 || previous==int32_t(index) || next==int32_t(index) || (previous>=0 && previous==next)) {
        throw std::invalid_argument("Invalid original stack links");
    }
    if(previous>=0) {const auto& u=p.Unit(size_t(previous));if(u.stack_next!=int32_t(index) || u.x!=x || u.y!=y) {throw std::invalid_argument("Inconsistent previous Unit link");}}
    if(next>=0) {const auto& u=p.Unit(size_t(next));if(u.stack_previous!=int32_t(index) || u.x!=x || u.y!=y) {throw std::invalid_argument("Inconsistent next Unit link");}}
    if(previous>=0) {p.Unit(size_t(previous)).stack_next=next;}
    if(next>=0) {p.Unit(size_t(next)).stack_previous=previous;}
    else if(previous<0 && p.Valid(x,y)) {const auto i=p.Tile(x,y);p.Items(i,p.Items(i)&~2u);}
    world.invalidation={int32_t(index),x,y};unit.x=unit.y=-1;unit.stack_next=unit.stack_previous=-1;
}
}
