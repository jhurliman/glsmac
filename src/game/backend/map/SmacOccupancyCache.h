#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace game::backend::map {
// Original cached affiliation and presence history, separate from territory,
// observer memory and the high site-rating nibble. No query recounts occupants.
class SmacOccupancyCache {
public:
    using Coordinate=std::array<int32_t,2>;
    struct Cell {uint8_t affiliation,presence;bool operator==(const Cell& other) const noexcept {return affiliation==other.affiliation && presence==other.presence;}};
    struct Entity {Coordinate coordinate;int32_t owner;};
    static constexpr size_t MAX_WIRE_BYTES=17+512*256/2;
    static SmacOccupancyCache Create(int32_t width,int32_t height,bool wrap,const std::vector<Cell>& cells);
    static SmacOccupancyCache Blank(int32_t width,int32_t height,bool wrap);
    const Cell& At(Coordinate coordinate) const;
    void PlaceUnit(Coordinate coordinate,int32_t owner,bool first);
    void RemoveUnit(Coordinate coordinate,bool last);
    void FoundBase(Coordinate coordinate,int32_t owner);
    void RemoveBase(Coordinate coordinate);
    void SetAffiliation(Coordinate coordinate,int32_t owner);
    // Original load order: clear/rebuild Units against incoming Base flags,
    // then clear/rebuild Bases. Empty cells retain the old affiliation.
    void Repair(const std::vector<Entity>& units,const std::vector<Entity>& bases);
    int32_t Width() const noexcept {return m_width;}
    int32_t Height() const noexcept {return m_height;}
    bool Wraps() const noexcept {return m_wrap;}
    bool operator==(const SmacOccupancyCache& other) const noexcept;
    size_t SerializedSize() const noexcept {return 17+m_cells.size();}
    std::string Serialize() const;
    static SmacOccupancyCache Deserialize(const std::string& wire);
private:
    SmacOccupancyCache()=default;
    size_t Index(Coordinate coordinate) const;
    int32_t m_width=0,m_height=0;bool m_wrap=false;
    std::vector<Cell> m_cells;
};
}
