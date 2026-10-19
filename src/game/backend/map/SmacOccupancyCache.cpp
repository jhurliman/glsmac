#include "SmacOccupancyCache.h"
#include <stdexcept>
namespace game::backend::map {
namespace {
void Require(bool valid,const char* message) {if(!valid) {throw std::invalid_argument(message);}}
size_t Count(int32_t width,int32_t height) {
    Require(width>=2 && width<=512 && !(width&1) && height>=1 && height<=256,"Occupancy requires a bounded doubled-coordinate map");
    return size_t(width/2)*height;
}
void Owner(int32_t owner) {Require(owner>=0 && owner<=7,"Occupancy requires an actual native or positive owner");}
void Write(std::string& out,size_t at,uint32_t value) {for(size_t i=0;i<4;++i) {out.at(at+i)=static_cast<char>(value>>(8*i));}}
uint32_t Read(const std::string& in,size_t at) {uint32_t value=0;for(size_t i=0;i<4;++i) {value|=uint32_t(static_cast<uint8_t>(in.at(at+i)))<<(8*i);}return value;}
}
SmacOccupancyCache SmacOccupancyCache::Create(int32_t width,int32_t height,bool wrap,const std::vector<Cell>& cells) {
    Require(cells.size()==Count(width,height),"Occupancy cell extent differs from geometry");
    for(const auto& cell:cells) {Require(cell.affiliation<=15 && cell.presence<=3,"Invalid occupancy cache cell");}
    SmacOccupancyCache result;result.m_width=width;result.m_height=height;result.m_wrap=wrap;result.m_cells=cells;return result;
}
SmacOccupancyCache SmacOccupancyCache::Blank(int32_t width,int32_t height,bool wrap) {return Create(width,height,wrap,std::vector<Cell>(Count(width,height),Cell{15,0}));}
size_t SmacOccupancyCache::Index(Coordinate c) const {
    Require(c[1]>=0 && c[1]<m_height && !((c[0]^c[1])&1),"Invalid occupancy coordinate");
    if(m_wrap) {c[0]%=m_width;if(c[0]<0) {c[0]+=m_width;}}
    Require(c[0]>=0 && c[0]<m_width,"Occupancy coordinate is outside flat map");return size_t(c[1])*(m_width/2)+c[0]/2;
}
const SmacOccupancyCache::Cell& SmacOccupancyCache::At(Coordinate coordinate) const {return m_cells.at(Index(coordinate));}
void SmacOccupancyCache::PlaceUnit(Coordinate coordinate,int32_t owner,bool first) {
    Owner(owner);auto& cell=m_cells.at(Index(coordinate));if(first && !(cell.presence&1u)) {cell.affiliation=static_cast<uint8_t>(owner);}cell.presence|=2u;
}
void SmacOccupancyCache::RemoveUnit(Coordinate coordinate,bool last) {auto& cell=m_cells.at(Index(coordinate));if(last) {cell.presence&=~2u;}}
void SmacOccupancyCache::FoundBase(Coordinate coordinate,int32_t owner) {Owner(owner);auto& cell=m_cells.at(Index(coordinate));cell.affiliation=static_cast<uint8_t>(owner);cell.presence|=1u;}
void SmacOccupancyCache::RemoveBase(Coordinate coordinate) {m_cells.at(Index(coordinate)).presence&=~1u;}
void SmacOccupancyCache::SetAffiliation(Coordinate coordinate,int32_t owner) {Owner(owner);m_cells.at(Index(coordinate)).affiliation=static_cast<uint8_t>(owner);}
void SmacOccupancyCache::Repair(const std::vector<Entity>& units,const std::vector<Entity>& bases) {
    // Authenticate the whole repair before changing the staged plane.
    for(const auto* rows:{&units,&bases}) {for(const auto& row:*rows) {Owner(row.owner);(void)Index(row.coordinate);}}
    for(auto& cell:m_cells) {cell.presence&=~2u;}
    for(const auto& unit:units) {auto& cell=m_cells.at(Index(unit.coordinate));PlaceUnit(unit.coordinate,unit.owner,!(cell.presence&2u));}
    for(auto& cell:m_cells) {cell.presence&=~1u;}
    for(const auto& base:bases) {auto& cell=m_cells.at(Index(base.coordinate));if(!(cell.presence&1u)) {FoundBase(base.coordinate,base.owner);}}
}
bool SmacOccupancyCache::operator==(const SmacOccupancyCache& other) const noexcept {return m_width==other.m_width && m_height==other.m_height && m_wrap==other.m_wrap && m_cells==other.m_cells;}
std::string SmacOccupancyCache::Serialize() const {
    std::string wire(17+m_cells.size(),'\0');wire.replace(0,4,"SOCC");Write(wire,4,1);Write(wire,8,m_width);Write(wire,12,m_height);wire[16]=m_wrap;
    for(size_t i=0;i<m_cells.size();++i) {wire[17+i]=static_cast<char>((m_cells[i].affiliation<<2)|m_cells[i].presence);}return wire;
}
SmacOccupancyCache SmacOccupancyCache::Deserialize(const std::string& wire) {
    Require(wire.size()>=17 && wire.size()<=MAX_WIRE_BYTES && wire.compare(0,4,"SOCC")==0 && Read(wire,4)==1,"Invalid occupancy record header");
    const auto width=Read(wire,8),height=Read(wire,12);Require(width<=512 && height<=256 && static_cast<uint8_t>(wire[16])<=1,"Invalid occupancy record geometry");
    const auto count=Count(width,height);Require(wire.size()==17+count,"Incomplete occupancy record or trailing bytes");
    std::vector<Cell> cells;cells.reserve(count);
    for(size_t i=0;i<count;++i) {const auto raw=static_cast<uint8_t>(wire[17+i]);Require(raw<64,"Invalid occupancy record cell");cells.push_back({static_cast<uint8_t>(raw>>2),static_cast<uint8_t>(raw&3u)});}
    return Create(width,height,wire[16]!=0,cells);
}
}
