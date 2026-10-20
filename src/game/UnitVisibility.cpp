#include "UnitVisibility.h"
#include <algorithm>
#include <set>

namespace game::actor_visibility {
namespace {
bool Disclosed(const Snapshot& s,const Unit& unit) {
    return (unit.owner==s.viewer && unit.flags && ((*unit.flags&0x440)!=0x440))
        || (unit.visible_to && (*unit.visible_to&(1u<<s.viewer)));
}
bool Selected(const Snapshot& s,const Unit& unit) {
    return unit.owner==s.viewer && unit.flags && ((*unit.flags&0x440)!=0x440)
        && unit.status && !(*unit.status&0x4200);
}
}
UnitPlan OrdinaryUnits(const Snapshot& s,int32_t x,int32_t y,
    std::optional<uint64_t> selected,bool control,bool pending,const std::vector<uint64_t>* tile_members) {
    if(!s.Known(x,y) || s.occupancy.size()!=s.explored_by.size()) {return {};}
    const auto cell=s.occupancy[size_t(y)*size_t(s.width/2)+size_t(x/2)];
    std::map<uint64_t,const Unit*> members;
    std::map<int16_t,uint64_t> source;
    const auto add=[&](uint64_t id,const Unit& unit) {
        if(unit.tile!=std::array<int32_t,2>{x,y} || !members.emplace(id,&unit).second) {return false;}
        return !unit.stack || source.emplace(unit.stack->id,id).second;
    };
    // The frontend supplies the complete tile index built from this owned
    // snapshot. Standalone/reference callers can use the full roster path.
    if(tile_members) {
        for(const auto id:*tile_members) {const auto row=s.units.find(id);if(row==s.units.end() || !add(id,row->second)) {return {};}}
    } else {
        for(const auto& row:s.units) {if(row.second.tile==std::array<int32_t,2>{x,y} && !add(row.first,row.second)) {return {};}}
    }
    if(members.empty()) {return {};}
    // A singleton needs no fabricated source-pool identity to establish order.
    std::vector<uint64_t> order;
    if(members.size()==1) {
        const auto& links=members.begin()->second->stack;
        if(links && (links->id<0 || links->id>=2048 || links->next!=-1 || links->previous!=-1)) {return {};}
        order.push_back(members.begin()->first);
    }
    else {
        if(source.size()!=members.size()) {return {};}
        std::optional<uint64_t> head;
        for(const auto& row:members) {if(row.second->stack->previous==-1) {if(head) {return {};}head=row.first;}}
        if(!head) {return {};}
        auto current=*head;int16_t previous=-1;std::set<uint64_t> visited;
        for(;;) {
            if(!visited.insert(current).second) {return {};}
            const auto& unit=*members.at(current);const auto& links=*unit.stack;
            if(links.id<0 || links.id>=2048 || links.next < -1 || links.next>=2048 || links.previous!=previous || !unit.domain || *unit.domain>2) {return {};}
            order.push_back(current);if(links.next==-1) {break;}
            const auto next=source.find(links.next);if(next==source.end()) {return {};}
            previous=links.id;current=next->second;
        }
        if(order.size()!=members.size()) {return {};}
    }
    auto lookup=order.front();
    const auto chosen=selected ? members.find(*selected):members.end();
    if(chosen!=members.end() && Selected(s,*chosen->second)) {lookup=chosen->first;}
    else if(!(cell.presence&2u)) {return {};}
    const auto& initial=*members.at(lookup);
    if(!Disclosed(s,initial)) {return {};}
    // The Base marker/tag gate precedes representative promotion too.
    if((cell.presence&1u) && cell.tag<8) {
        const bool order_admits=initial.working || (initial.order && *initial.order!=1 && *initial.order!=2 && *initial.order!=3);
        if(!control || !pending || !selected || *selected!=lookup || !Selected(s,initial) || !order_admits) {return {};}
    }
    auto body=lookup;int best=-1;
    for(const auto id:order) {
        const auto& candidate=*members.at(id);
        if(selected && id==*selected && candidate.owner==s.viewer && candidate.status
            && !(*candidate.status&0x4200) && control && pending) {body=id;break;}
        const int rank=int(candidate.domain.value_or(0))+(candidate.owner==s.viewer ? 4:0);
        if(rank>best) {best=rank;body=id;}
    }
    return {body,std::move(order)};
}
}
