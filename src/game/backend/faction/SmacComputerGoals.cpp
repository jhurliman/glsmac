#include "SmacComputerGoals.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::faction {
namespace {
void Dimensions(int32_t width,int32_t height) {
    if(width<=0 || height<=0) {throw std::invalid_argument("Computer goal insertion requires positive map dimensions");}
}
template<size_t N>
size_t Insert(std::array<SmacComputerGoal,N>& records,int32_t type,int32_t priority,int32_t x,int32_t y,int32_t auxiliary,bool active) {
    // Native comparisons use the incoming DWORD; only the final store narrows.
    const auto word=[](int32_t value) {
        const auto low=uint32_t(value)&0xffffu;
        return static_cast<int16_t>(low<0x8000u ? int32_t(low):int32_t(low)-0x10000);
    };
    for(auto& record:records) {
        if(record.type==type && record.x==x && record.y==y) {
            record.priority=word(std::max(int32_t(record.priority),priority));
            return N; // duplicate does not refresh auxiliary or active storage
        }
    }
    size_t best=N;int32_t best_score=0;
    for(size_t i=0;i<N;++i) {
        const auto& record=records[i];int32_t score=0;
        if(record.type<0) {score=1000;}
        else if(record.priority<priority) {
            score=active && record.priority<=0 ? 100+int32_t(record.priority):20-int32_t(record.priority);
        }
        if(score>best_score) {best=i;best_score=score;}
    }
    if(best==N) {return N;}
    records[best]={word(type),word(priority),x,y,active ? auxiliary:records[best].auxiliary};return best;
}
}
bool SmacComputerGoal::operator==(const SmacComputerGoal& other) const {
    return type==other.type && priority==other.priority && x==other.x && y==other.y && auxiliary==other.auxiliary;
}
bool SmacComputerGoals::operator==(const SmacComputerGoals& other) const {return active==other.active && persistent==other.persistent;}
void SmacComputerGoals::InsertActive(int32_t type,int32_t priority,int32_t x,int32_t y,int32_t auxiliary,int32_t width,int32_t height) {
    Dimensions(width,height);
    if(x<0 || x>=width || y<0 || y>=height) {return;}
    Insert(active,type,priority,x,y,auxiliary,true);
}
void SmacComputerGoals::InsertPersistent(int32_t type,int32_t priority,int32_t x,int32_t y,int32_t width,int32_t height) {
    Dimensions(width,height);
    // Unlike active insertion, the native persistent helper has no bounds
    // rejection. Its parity diagnostic does not supply a gameplay predicate.
    const auto index=Insert(persistent,type,priority,x,y,-1,false);
    if(index<persistent.size()) {
        const auto& saved=persistent[index];
        InsertActive(saved.type,saved.priority,saved.x,saved.y,-1,width,height);
    }
}
void SmacComputerGoals::Renew(int32_t width,int32_t height) {
    Dimensions(width,height);
    for(auto& record:active) {
        if(record.priority<0) {record.type=-1;}
        else {record.priority=static_cast<int16_t>(-record.priority);}
    }
    for(const auto& record:persistent) {
        if(record.type>=0) {InsertActive(record.type,record.priority,record.x,record.y,-1,width,height);}
    }
}
}
